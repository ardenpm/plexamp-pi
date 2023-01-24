![CAD Assembly](https://github.com/ardenpm/plexamp-pi/raw/main/images/onshape_assembly.png)

# Plexamp Pi Streamer

I wanted a self-contained streamer for [Plexamp](https://plexamp.com/) which would let me navigate and select my music, control tracks with a rotary encoder and output to a reasonably good headphone amplifier, this project is the result. It includes a 3D printable enclosure and some software components which can be combined, with the off the shelf hardware to build the unit.

This project is not affiliated with Plex or Plexamp in anyway.

## Hardware

### 3D Printed Enclosure

![Finished Enclosure](https://github.com/ardenpm/plexamp-pi/raw/main/images/photo_finished_2.jpg)

The enclosure to holds a Raspberry Pi 4, IQudio DAC+, the Raspberry Pi Touchscreen and a rotary encoder. A full CAD model is available in [Onshape](https://onshape.com) here:

[Plexamp Pi Enclosure CAD](https://cad.onshape.com/documents/df243c2479e3554b8c02ccae/w/d451762f250ff1bc25f3b904/e/bc04dc9fd1fc2764ce90258b)

Exported STL versions of these files can be found in the `stl` directory of this repository. They are all in the correct orientation for printing. This model is provided under the [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) license. This CAD model includes third-party models subject to their own licenses for aligning the components. These models can be found here:

- [Raspberry Pi 4b](https://grabcad.com/library/raspberry-pi-4-b-1)
- [Official Touchscreen](https://www.printables.com/model/202234-official-raspberry-pi-7-touch-screen-reference-mod)

Please consult the above links for full details on these models. They are used for alignment reference only and are not present in the exported STL files. All of my models were printed in [eSUN PLA-ST](https://www.esun3d.com/epla-st-product/) however any standard material like PETG, ASA or ABS should work fine. You will however need to have your printer fairly dialed in or otherwise go into the Onshape model and adjust some of the components.

The IO cover and USB cover parts which slid into the back cover are quite thin and slide into a narrow slot. I have found that even different filaments can make a difference between this fitting and not. The covers are of course cosmetic and could be left off but I also found sliding them in and out of the back cover a few times can help if they are a little tight initially.

Everything except the frame bezel can be printed without supports. If your bridging is very reliable you could also print the bezel without supports, if not I would recommend supporting the rectangular cutout for the rotary encoder (and using support blockers elsewhere).

Two heatset threaded inserts are used to fix the stand to the bezel. These can be inserted with a soldering iron. The back cover is screwed to the topmost standoffs which secures it against the bezel, locking the IO and USB covers into place. The fan should also have threaded inserts placed in its mounting holes however these do not need to be heatset and can just be pulled in from the otherside with an M3 screw. The fan should be mounted to the rear cover before placing the rear cover over the stack of LCD controller, Raspberry Pi and DAC+ hat.

Aligning the IO and USB covers while simultaneously lowering the back cover and keeping all of the wires clear of pinching can be a bit tricky and might take a few tries and different hand placements before you get it.

### BOM

#### Fasteners

- 2 x M3x8mm button head screw (for fixing the stand to the bezel)
- 4 x M3x10mm countersink screw (for the case fan)
- 4 x M3x4mm countersink screw (for fixing the display to the bezel)
- 4 x M2.5x4mm countersink screw (for fixing the rear cover to the top-most standoff)
- 4 x M2.5x23mm standoff (top-most standoff)
- 8 x M2.5x12mm standoff (between the LCD and driver board and driver board and Pi)
- [6 x M3x5.7mm Theaded insert](https://cnckitchen.store/products/gewindeeinsatz-threaded-insert-m3-standard-100-stk-pcs)

#### Electronics

- [Noctua 5V 40mm PWM Fan](https://noctua.at/en/products/fan/nf-a4x10-5v-pwm)
- [Raspberry Pi 4 Model B](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)
- [Raspberry Pi DAC+](https://www.raspberrypi.com/products/dac-plus/)
- [Raspberry Pi Touch Display](https://www.raspberrypi.com/products/raspberry-pi-touch-display/)
- KY-040 Rotary Encoder
- Dupont connectors and hookup wire

My build used an IQAudio DAC+ hat however this company has since been acquired by the Raspberry Pi Foundation so the above listed Raspberry Pi DAC+ hat is the equivilent model. The layout of the headers may have changed so if you are using that model you might need to make adjustments to the IO cover. The new official Raspberry Pi DAC+ hat also omits the rotary encoder and mute headers, however it includes passthrough of all of the GPIO pins (if the header is soldered to the board) so they can be directly connected there.

### Connections

![Stack Wiring](https://github.com/ardenpm/plexamp-pi/raw/main/images/photo_stack_1.jpg)

The following connections were used for the initial build. Of course alternative configurations are possible but these have been tested. Check these actually line up with what you have if using this as a guide. No responsibility if you fry your Pi. Of course the wire colours are just what I chose or happened to be already connected to the devices being used.

- Display power + (red) → pin 2 (5V)
- Display power - (black) → pin 6 (GND)
- Fan Power + (yellow) → pin 4 (5V)
- Fan Power - (black) → pin 9 (GND)
- Fan PWM (blue) → pin 8 (GPIO14)
- Encoder Power + (red) → pin 1 (3.3V)
- Encoder SW (green) → pin 13 (GPIO27)
- Encoder CLK (A) (blue) → pin 16 (GPIO23)
- Encoder DT (B) (yellow) → pin 18 (GPIO24)
- Encoder GND (black) → pin 14 (GND)

While hardware PWM is generally preferable for fan control, the Raspberry Pi hardware PWM circuitry is shared with the headphone jack. While we are using a dedicated headphone applifier the headphone jack has not been covered so software PWM is used instead. See [this article](https://blog.driftking.tw/en/2019/11/Using-Raspberry-Pi-to-Control-a-PWM-Fan-and-Monitor-its-Speed/) for further details. Some other articles which may be relevant can be found here:

- [Connecting a PWM Fan to a Raspberry Pi](https://www.the-diy-life.com/connecting-a-pwm-fan-to-a-raspberry-pi/)
- [Noctua Fan Pinouts](https://faqs.noctua.at/support/solutions/articles/101000081757)

The Noctua model PWM fan is highly recommended despite costing a bit more as it is significantly quieter than other models on the market. It also has more complete documentation.

## Software

For this build the Raspberry Pi was running Raspberry Pi OS (64-bit). If you want to use a different distribution the instructions may need to be adapted.

### Plexamp

Download and install [Plexamp](https://plexamp.com/). You need the headless version. Instructions for installing this and its dependencies are linked on the Plexamp website. These can change from time to time so are not included here.

### Plexamp UI

On startup we want the browser to start full screen and open the Plexamp page. First add a shell script to handle loading Chromium full screen and deal with files left by unclean shutdowns. Create the following file in `~/.local/bin/plexamp-ui` in a text editor:

```
#!/bin/bash
if [ -f /home/$USER/.config/chromium/Default/Preferences ]; then
  sed -i 's/"exited_cleanly":false/"exited_cleanly":true/' /home/$USER/.config/chromium/Default/Preferences
fi
if [ -f /home/$USER/.config/chromium/Default/Preferences ]; then
  sed -i 's/"exit_type":"Crashed"/"exit_type":"Normal"/' /home/$USER/.config/chromium/Default/Preferences
fi
xset s noblank
xset s off
xset -dpms
/usr/bin/chromium-browser --noerrdialogs --disable-infobars --kiosk http://127.0.0.1:32500/
```

You can omit the `xset` lines if you want to leave your display sleep enabled. Now we need to add this as a service so it runs on startup. Create a file `/lib/systemd/system/plexamp-ui.service` and add the following text:

```
[Unit]
Description=Plexamp Browser Interface

[Service]
Environment=DISPLAY=:0
Environment=XAUTHORITY=/home/pi/.Xauthority
ExecStart=/home/pi/.local/bin/plexamp-ui
Restart=always
RestartSec=10s
TimeoutSec=infinity
User=pi

[Install]
WantedBy=graphical.target
```

Test the service works and brings up the browser interface on your screen:

```
sudo systemctl start plexamp-ui.service
```

If so you can then enable the service:

```
sudo systemctl enable plexamp-ui.service
```

You'll likely also want to disable the pointer display so the cursor doesn't appear on the touch screen. To do this edit `/etc/lightdm/lightdm.conf` and uncomment the line:

```
#xserver-command=X
```

Then change it to:

```
xserver-command=X -nocursor
```

### PWM Fan Control Service

The Noctua PWM fan does not need to run at full speed all the time. We want it to be tied to the CPU temperature. The default Raspberry Pi OS fan configuration only allows for turning the fan on and off rather than controlling its speed. For that we need to use a separate script. Do not enable the Raspberry Pi OS fan configuration as it will interfere with this script.

First get the PWM fan control script from Michael Klements repository. While hardware PWM is generally preferable for fan control, the Raspberry Pi hardware PWM circuitry is shared with the headphone jack. While we are using a dedicated headphone applifier the headphone jack has not been covered so software PWM is used instead. See [this article](https://blog.driftking.tw/en/2019/11/Using-Raspberry-Pi-to-Control-a-PWM-Fan-and-Monitor-its-Speed/) for a more detailed description of controlling a 4-pin PC fan using Raspberry Pi PWM features.

```
cd ~
git clone https://github.com/mklements/PWMFanControl.git
cp PWMFanControl/FanProportional.py ~/.local/bin/pwm-fan
chmod +x ~/.local/bin/pwm-fan
```

Change the PWM frequency in the script and add a shebang.

```
sed -i -e '1s/^/\#\!\/usr\/bin\/env python3\n/;s/fan = IO.PWM(14,100)/fan = IO.PWM(14,25)/' ~/.local/bin/FanProportional.py
```

Add the following to /lib/systemd/system/pwm-fan.service

```
[Unit]
Description=PWM Fan Control
After=network.target

[Service]
ExecStart=/usr/bin/python3 /home/pi/.local/bin/FanProportional.py
Restart=always
User=pi

[Install]
WantedBy=multi-user.target
```

Test the services works and if so enable it.

```
sudo systemctl start pwm-fan.service
sudo systemctl enable pwm-fan.service
```

The service file and modified script are included in this repository. Use the above instructions if you prefer to ensure you have the latest version of the script Michael Klements authored.

### Rotary Encoder Control

This repository also contains software to utilise a rotary encoder and button to control Plexamp. It can be found in the `control` directory. After connecting your encoder you will need to add the following lines to `config.txt` to enable the relevant event devices:

```
dtoverlay=rotary-encoder,pin_a=23,pin_b=24,relative_axis=1
dtoverlay=gpio-key,gpio=27,keycode=42,label="Left Shift"
```

Obviously change the pins are required for your configuration. We assign the key press to the Left Shift here so that it will not cause any undesired actions. To build the control software you will need some libraries:

```
sudo apt update
sudo apt install libxml2-dev libcurl4-openssl-dev
```

You can then build the software control software and install it as a service.

```
cd control
make
mkdir -p ~/.local/bin
cp plexamp-control ~/.local/bin
sudo cp plexamp-control.service /lib/systemd/system/
sudo systemctl enable plexamp-control
```

This program makes HTTP requests to your local running Plexamp instance based on those requests it was observed to be making through its webpage. Currently these do not require authentication (at least when requesting from the same host). This is however not a published API from Plex so it could change at anytime. If you need to modify the URLs you can change them in the `plexamp-control.c` file and recompile.
