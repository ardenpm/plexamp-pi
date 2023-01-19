/*
 * Copyright (c) 2022-2023 Paul Arden
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <linux/input.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>

#include <curl/curl.h>
#include <libxml/parser.h>

#define EVENT_DEV_PATH "/dev/input"
#define EVENT_DEV_NAME "event"
#define BUTTON_PREFIX "button@"
#define ROTARY_PREFIX "rotary@"
#define ROTARY_MULTIPLIER 1000
#define MAX_EVENTS 5
#define MAX_STRING 4096
#define MAX_MULTI_PRESS_DELAY_MS 150

static volatile int running = 1;

void sig_handler(int val) {
  fprintf(stderr, "Ctrl-C recieved.\n");
  running = 0;
}

unsigned long previous_time_usec;
int press_queue = 0;
bool debug = false;

struct http_response_body {
  char *data;
  size_t length;
};

void http_response_body_init(struct http_response_body *body) {
  body->length = 0;
  body->data = malloc(body->length + 1);
  if (body->data == NULL) {
    fprintf(stderr, "Unable to allocate memory for HTTP response.\n");
    exit(EXIT_FAILURE);
  }
  body->data[0] = '\0';
}

size_t http_response_append(void *ptr, size_t size, size_t nmemb, struct http_response_body *body)
{
  size_t new_length = body->length + size * nmemb;
  body->data = realloc(body->data, new_length + 1);
  if (body->data == NULL) {
    fprintf(stderr, "Unable to reallocate memory for HTTP response.\n");
    exit(EXIT_FAILURE);
  }
  memcpy(body->data + body->length, ptr, size * nmemb);
  body->data[new_length] = '\0';
  body->length = new_length;

  return size*nmemb;
}

struct plexamp_status_struct {
  bool playing;
  long time;
  long duration;
} plexamp_status;

void plexamp_get_status_from_xml(xmlNode *a_node) {
  xmlNode *current = NULL;

  for (current = a_node; current; current = current->next) {
    if (current->type == XML_ELEMENT_NODE && strcmp(current->name, "Timeline") == 0) {
      xmlChar* type = xmlGetProp(current, "type");
      if (strcmp(type, "music") == 0) {
        xmlChar* s_state = xmlGetProp(current, "state");
        xmlChar* s_time = xmlGetProp(current, "time");
        xmlChar* s_duration = xmlGetProp(current, "duration");
        char *ptr;
        if (strcmp(s_state, "paused") == 0) {
          plexamp_status.playing = false;
        } else if (strcmp(s_state, "playing") == 0) {
          plexamp_status.playing = true;
        }
        plexamp_status.time = strtol(s_time, &ptr, 10);
        plexamp_status.duration = strtol(s_duration, &ptr, 10);
        xmlFree(s_state);
        xmlFree(s_time);
        xmlFree(s_duration);
      }
      xmlFree(type);
    }
    plexamp_get_status_from_xml(current->children);
  }
}

void plexamp_get_status(CURL *curl) {
  struct http_response_body body;
  CURLcode result;
  xmlDocPtr doc = NULL;
  xmlNodePtr root = NULL;

  http_response_body_init(&body);
  curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:32500/player/timeline/poll?type=music&commandID=1");
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "Error: cURL did not return OK.\n");
    exit(EXIT_FAILURE);
  }

  doc = xmlReadMemory(body.data, strlen(body.data) * sizeof(char), "poll.xml", NULL, XML_PARSE_NOWARNING);
  if (doc == NULL) {
    fprintf(stderr, "Error: Failed to parse XML response.\n");
    exit(EXIT_FAILURE);
  }

  root = xmlDocGetRootElement(doc);
  plexamp_get_status_from_xml(root);

  xmlFreeDoc(doc);
  free(body.data);
}

void handle_encoder_event(int rel, CURL *curl) {
  plexamp_get_status(curl);

  struct http_response_body body;
  CURLcode result;
  char url[MAX_STRING];
  long offset = plexamp_status.time + (rel * 10000);
  
  http_response_body_init(&body);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

  sprintf(url, "http://127.0.0.1:32500/player/playback/seekTo?type=music&offset=%ld&commandID=1", offset);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  if (debug) printf("Encoder: Scrub to offset %ld\n", offset);

  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "Error: cURL did not return OK.\n");
    exit(EXIT_FAILURE);
  }

  free(body.data);
}

void handle_generic_event(struct input_event *ie, CURL *curl)
{
  unsigned long new_time_usec, delay;
  switch (ie->type) {
	case EV_REL:
    handle_encoder_event(ie->value, curl);
		break;
	case EV_KEY:
    new_time_usec = 1000000 * ie->time.tv_sec + ie->time.tv_usec;
    delay = new_time_usec - previous_time_usec;
    if (ie->value == 1) {
      if (delay < MAX_MULTI_PRESS_DELAY_MS * ROTARY_MULTIPLIER) {
        press_queue++;;
      } else {
        press_queue = 1;
      }
    }
    previous_time_usec = new_time_usec;
		break;
	default:
		break;
	}
}

void handle_single_press(CURL *curl) {
  plexamp_get_status(curl);

  struct http_response_body body;
  CURLcode result;
  
  http_response_body_init(&body);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

  if (plexamp_status.playing) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:32500/player/playback/pause?type=music&commandID=1");
    if (debug) printf("Single press: Pause\n");
  } else {
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:32500/player/playback/play?type=music&commandID=1");
    if (debug) printf("Single press: Play\n");
  }

  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "Error: cURL did not return OK.\n");
    exit(EXIT_FAILURE);
  }

  free(body.data);
}

void handle_double_press(CURL *curl) {
  struct http_response_body body;
  CURLcode result;
  
  http_response_body_init(&body);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

  curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:32500/player/playback/skipNext?type=music&commandID=1");
  if (debug) printf("Double press: Next track\n");

  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "Error: cURL did not return OK.\n");
    exit(EXIT_FAILURE);
  }

  free(body.data);
}

void handle_triple_press(CURL *curl) {
  struct http_response_body body;
  CURLcode result;
  
  http_response_body_init(&body);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

  curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:32500/player/playback/skipPrevious?type=music&commandID=1");
  if (debug) printf("Triple press: Previous track\n");

  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "Error: cURL did not return OK.\n");
    exit(EXIT_FAILURE);
  }

  free(body.data);
}

static int is_event_device(const struct dirent *dir) {
	return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

int main(int argc, char *argv[]) {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  if (argc == 2) {
    if (strcmp(argv[1], "--debug") == 0 || strcmp(argv[1], "-d") == 0) {
      debug = true;
    }
  }

  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  previous_time_usec = 1000000 * current_time.tv_sec + current_time.tv_usec;

  struct dirent **paths;
  int device_count;
  int fd, fd_rotary, fd_button;
  bool button_found = false;
  bool rotary_found = false;

  device_count = scandir(EVENT_DEV_PATH, &paths, is_event_device, alphasort);
  if (device_count < 2) {
    fprintf(stderr, "Error: Not enough devices to scan.\n");
    exit(EXIT_FAILURE);
  }

  printf("Searching %d event devices for a rotary encoder and button.\n", device_count);

  for (int i = 0; i < device_count; i++) {
    char path[MAX_STRING];
    char name[MAX_STRING];
    snprintf(path, sizeof(path), "%s/%s", EVENT_DEV_PATH, paths[i]->d_name);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
      continue;
    }
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    if (strstr(name, BUTTON_PREFIX) != NULL) {
      printf("Button %s found at %s\n", name, path);
      button_found = true;
      fd_button = fd;
    } else if (strstr(name, ROTARY_PREFIX) != NULL) {
      printf("Rotary %s encoder found at %s\n", name, path);
      rotary_found = true;
      fd_rotary = fd;
    } else {
      close(fd);
    }
    if (button_found && rotary_found) {
      break;
    }
  }

  if (!button_found || !rotary_found) {
    fprintf(stderr, "Error: button or rotary encoder not found.\n");
    if (rotary_found && !button_found) {
      close(fd_rotary);
    }
    if (button_found && !rotary_found) {
      close(fd_button);
    }
    exit(EXIT_FAILURE);
  }

  struct epoll_event event, events[MAX_EVENTS];
  int epoll_fd = epoll_create(2);
  if (epoll_fd == -1) {
    fprintf(stderr, "Failed to create epoll file descriptor\n");
    close(fd_rotary);
    close(fd_button);
    exit(EXIT_FAILURE);
  }

  struct input_event ie;

  event.events = EPOLLIN;
  event.data.fd = fd_rotary;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_rotary, &event)) {
    fprintf(stderr, "Failed to add rotary encoder to epoll\n");
    close(epoll_fd);
    close(fd_rotary);
    close(fd_button);
    exit(EXIT_FAILURE);
  }
  event.data.fd = fd_button;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_button, &event)) {
    fprintf(stderr, "Failed to add button to epoll\n");
    close(epoll_fd);
    close(fd_rotary);
    close(fd_button);
    exit(EXIT_FAILURE);
  }

  CURL *curl;
  CURLcode res;
  
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Unable to initialise cURL.\n");
    close(epoll_fd);
    close(fd_rotary);
    close(fd_button);
    exit(EXIT_FAILURE);
  }

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_response_append);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  int epoll_timeout = -1;

  while (running) {
    int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, epoll_timeout);
    if (event_count == 0) {
      switch (press_queue) {
      case 1:
        handle_single_press(curl);
        break;
      case 2:
        handle_double_press(curl);
        break;
      case 3:
        handle_triple_press(curl);
        break;      
      default:
        break;
      }
      press_queue = 0;
    }
    for (int i = 0; i < event_count; i++) {
      ssize_t bytes_read = read(events[i].data.fd, &ie, sizeof(struct input_event));
      handle_generic_event(&ie, curl);
    }
    epoll_timeout = press_queue > 0 ? MAX_MULTI_PRESS_DELAY_MS : -1;
  }

  close(epoll_fd);
  close(fd_rotary);
  close(fd_button);

  xmlCleanupParser();
  curl_easy_cleanup(curl);

  printf("Polling exited.\n");

  exit(EXIT_SUCCESS);
}
