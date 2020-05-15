Brewbutton (Android (ESP32) & Browser Web Bluetooth)
====================================================
The nespresso expert is a piece of shit and the app even more. So this project aims to replace the mobile app, and allow you to brew your own stuff. The only reason I started banging my head on this BLE protocol, was that the default "lungo" on the front dial added too much water, making it undrinkable, and to overcome the crappy App and the Crappy default settings on the machine.

I have contributed a little to the reverse engineered of the protocol, see below.
Part of this insights was based on: https://gist.github.com/farminf/94f681eaca2760212f457ac59da99f23

The brewbutton Android
----------------------
, is a simple piece of code that connects to an nespresso machine, and when triggered by the IO PIN, it will send a simple brew command. Right now, it's hard-coded to a 130ml coffee recipe brew.
My idea is to have a small brew button that can brew my morning coffe the way I want it.

Brewbutton in Chrome
--------------------
**Update: 2020-05-10:**
I also added similar functionality using Browser Web Bluetooth. If you managed to get your Auth-key, you can most probably use this straight away, if we you have Chrome & W10 or MAC, see below. You should be able to test it right away, directly in browser. https://rawgit.com/petergullberg/brewbutton/master/brewbutton.html

![QR-code](qr_example.jpg)
bluefy://open?url=https://rawgit.com/petergullberg/brewbutton/master/brewbutton.html


HOW TO START
============

HW and environment setup
------------------------

The brew button is based on a simple ESP32 example, where I only added a debounced PIN, and  stuff needed to program the nespresso machine. There might be alternatives here, but I put it here as a reference.
* Arduino IDE 1.8.10
* NodeMCU ESP-32s(v1.1) with Bluetooth Low Energy support. https://hitechchain.se/iot/esp32-wemos-mini-d1-wifi-bluetooth
* ESP32 BLE Arduino v1.0.1 library
* Install I2C LCD - http://www.esp32learning.com/code/esp32-and-i2c-lcd-example.php
* GPIO2 <= with INTERNAL PULL_UP - hook up a button to ground..
* To setup the ESP32 environment in Arduino IDE, I followed this one: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/


Code Changes from default
-------------------------
From the original setup I had to make a few changes
* I based the work on ESP32 BLE_Client
* I hardcoded the #define CONFIG_BLE_SMP_ENABLE, in BLEDEvice.cpp, and BLEClient.cpp to ensure security is enabled (to be honest I haven't spent enough time, understanding how to makefiles works in Arduino
* FIX a bug in BleRemoteService.cpp on line 175: uint16_t count = 1;
* Add the line: BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT); in onConnect
* FIX a bug in BLERemoteCharacteristic.h, https://github.com/nkolban/esp32-snippets/blob/91e1571dd5bd433e107f92e1588780ee22fed42e/cpp_utils/BLERemoteCharacteristic.h#L77

On top of this, I also discvered a bug on my windows installation, that prevented me from compiling, but that was solved by a simple mklink....

RETRIEVE AUTH-KEY
-----------------

In order to start this, you need to retrieve the AUTH-KEY. For this, I find this a fairly simple step-by-step instruction.

* Using Android (I used Samsung Galaxy Note 4)
* Go to developer options in settings, enable BLE HCI snoop
* Start the Nespresso App (you need to have registered this and connected to your machine already)
* Brew a cup of coffee
* Stop BLE HCI snoop.
* Connect the mobile to USB/PC and copy or email the file (On Samsung, the snoop is here: \\Galaxy Note4\Phone\Android\data\btsnoop_hci.log)
* If you don't want to mess with WireShark, but uses Linux, you can try this command (at least it works for me)
```
hexdump -v -e '/1 "%02X "' btsnoop_hci.log | grep -o '0B 00 04 00 12 14 00 \<.. .. .. .. .. .. .. ..\>'
```
* Install wireshark (wireshark.org), learn a little about it: https://medium.com/@urish/reverse-engineering-a-bluetooth-lightbulb-56580fcb7546
* Load the file in wire-shark
* Look for Write Request to 0x0014 (Service 06aa3a41-f22a-11e3-9daa-0002a5d5c51b), the data part is the AUTH-key
* Extract the AUTH-KEY (8bytes), mine is "879608e27cb1f96e"

TEST AUTH-KEY
-------------
* A simple way to test the AuthKey is to use the Web Bluetooth web-page that is part of this project
https://rawgit.com/petergullberg/brewbutton/master/brewbutton.html
Test on Chrome on Mac and Win10
* Another way is to download the GATTBrowser from Renesas, you have it on both iPhone/Android
* Make sure your Nespresso app is not running, as the machine only accepts one connection
* Find and connect to the nespresso device
* Find auth service characteristics 06aa3a41-f22a-11e3-9daa-0002a5d5c51b
* Write AUTH-KEY to this register
* Find service characteristics 06aa3a42-f22a-11e3-9daa-0002a5d5c51b
* Write 03050704000000000105
* If you managed to do these steps above correctly you should be brewing an Americano
* Now you only need to enter the AUTH-KEY in your source code file compile and run

PROTOCOL ANALYSIS
==================
There exist some already done reverse engineering. I have tried to gather what was missing, in order to build what I wanted.
I used Wireshark and GATTBrowser from Renesas, both where quite handy. And you could also analyze status, while brewing manually.

STANDARD BREWS
--------------
Standard brews, are those that are on the front dial. Please note that the Americano contains way too much water, and is not a good choice.
```
0305070400000000 00 00 medium ristretto
0305070400000000 01 01 low espresso
0305070400000000 02 02 high lungo
0305070400000000 01 04 low hot water
0305070400000000 01 05 low americano
0305070400000000 01 07 Recipe brew      <= only throught BLE
03060102 would stop the brewing (not always)
```
As you can see, the last byte is the type of brew, and the second last is the temperature

RECIPE BREW (07)
-----------
Recipe brew, is where you can select how much coffe you want, the amount of water, and how hot the water should be.
This comes as two separate write requests to the "command service" 06aa3a42-f22a-11e3-9daa-0002a5d5c51b (0x0024)

```
To make a recipe brew, you send two separate commands to the "command service" characteristic 06aa3a42-f22a-11e3-9daa-0002a5d5c51b

 Prepare command (0x0024):
 +---------------------------------------------+
 |  01 10 08 00 00 {01 00 61} {02 00 24}       |
 +---------------------------------------------+
 | - {010061} 01=coffe, 0061h=97ml             |
 | - {020024} 02=water, 0024h=37ml             |
 | - It's possible to reverse these two,       |
 |   if you want water first.                  |
 +---------------------------------------------+

Brew command (0x0024):
 +---------------------------------------------+
 |  03 05 07 04 00 00 00 00 {02} {07}          |
 +---------------------------------------------+
 | - 02 = Hot, 01 = x, 00 = x                  |
 | - 07 = Recipe                               |
 +---------------------------------------------+

To brew a recipe coffe, you write preparation command and then the brew command. When doing the analysis, the app sends
What I noticed was that when water ran out, "water engaged" was still active, as it hadn't reached it's volume

```

**Flow when the App brews coffee**

```
Prepare command    ------->   Write char (06aa3a12-f22a-11e3-9daa-0002a5d5c51b) (0x001C) 10B
Read Char          <-------   Read char (0x0026) 20B
Brew command       ------->   Write char (06aa3a12-f22a-11e3-9daa-0002a5d5c51b) (0x001C) 10B
Read Char          <-------   Read (0x0026) 20B
Read STATUS        <-------   Read 0x001C 8B
```

STATUS
------

```
I investigated the status service (06aa3a12-f22a-11e3-9daa-0002a5d5c51b) (0x001C)
It's default state in idle mode is: "40 02 01 E0 40 00 FF FF"

 +------+-----------+---------------------------------------------------+
 | Byte |    Bit    | Description                                       |
 +------+-----------+---------------------------------------------------+
 |  B0  | x1xx xxxx | Always 1                                          |
 |      | xxx1 xxx0 | Capsule mechanism jammed                          |
 |      | xxxx xxx1 | Water is empty                                    |
 +------+-----------+---------------------------------------------------+
 |  B1  | 1xxx xxxx | Capsule engaged                                   |
 |      | x1xx xxxx | Tray open / tray sensor full. Dx when sensor trips|
 |      |           | It also seeems to be indicator of descaling       |
 |      | xxx1 xxxx | Tray sensor tripped during brewing?               |
 |      | xxxx 1xxx | Sleeping                                          |
 |      | xxxx x1xx | Water pump engaged                                |
 |      | xxxx xx1x | Awake, ok                                         |
 |      | xxxx xxx1 | Water temperature low / set while sleeping        |
 +------+-----------+---------------------------------------------------+
 |  B2  | ???? ???? | tbc                                               |
 |      |           | Typical value 0x80                                |
 +------+-----------+---------------------------------------------------+
 |  B3  | ???? ???? | tbc                                               |
 |      |           | 05h, seen 06h                                     |
 +------+-----------+---------------------------------------------------+
 |  B4  | xx.. .... | Appears to be an error counter. It's incremented  |
 |      |           | each time an error occurs. 00h,40h,80h,d0h,00h    |
 |      |           | Maybe used to detect error (notifications)        |
 +------+-----------+---------------------------------------------------+
 |  B5  | ???? ???? | tbc                                               |
 |      |           |                                                   |
 +------+-----------+---------------------------------------------------+
 | B6B7 |   XX XX   | [2019-12-21] Updated                              |
 |      |           | Appears to be a count-down that is used to        |
 |      |           | signal when descaling it needed when it reaches   |
 |      |           | 000h, probably starting from FFFFh                |
 |      |           | When it reached 0000h, it set B1.6.               |
 |      |           | Before descaling counter starts, the B6-B7 are    |
 |      |           | not returned.                                     |
 |      |           | Unclear what the values actually represent        |
 |      |           | but they tend to go slower as longer we wait      |
 +------+-----------+---------------------------------------------------+

What I noticed was that when water ran out, "water engaged" was still active, as it hadn't reached it's volume.
While brewing coffee, both capsule engage and water engaged are active.
Quirks found:
* Sometimes when device is reobooted status is 40 00 ..
* If tray sensor trips during brewing the value becomes 40 Dx
* After sending the brew command, you may get warming, water engaged and them coffee brew, seems to be some latency in status

Examples:
- Idle:        "40 02 01 E0 40 00 FF FF"
- Coffee:      "40 84 01 E0 40 00 FF FF"
- Water:       "40 04 01 E0 40 00 FF FF"
- Empty Water: "41 84 01 E0 40 00 FF FF" (capsule still locked in)
- Tray full:   "40 4x
```

CMD_RESPONSE_STATUS
-------------------
It appears that the characteristic 06aa3a52-f22a-11e3-9daa-0002a5d5c51b is the RESPONSE from COMMANDS sent over Bluetooth (06aa3a42-f22a-11e3-9daa-0002a5d5c51b).
This need to be clarified more in detail, but this is what I have found so far:
```
The command structure in both ways appears to be:
{2B_CMD}{L}{DATA}  =>
{2B_RESP}{L}{DATA} <=
B0.7 represent 0=CMD, 1=RESP
B0.6 represent ERROR

 +-------------------------------------+-------------------------------------------------------------------------+
 | Sequence                            | Interpretation                                                          |
 +-------------------------------------+-------------------------------------------------------------------------+
 | CMD : 03050704000000000200          | Send normal brew command                                                |
 | RESP: 83 05 01 20                   | Success (8x) on last command (Brew=x3 05), respcode:len(01),data(20)    |
 |                                     |                                                                         |
 | CMD : 03050704000000000200          | Send normal brew command, but did not cycle lid (no new pod)            |
 | RESP: c3 05 02 24 12                | Failure (cx) on last command (brew=x3 05), respCode:len(02),data(24 12) |
 |                                     | Reason(24 12) Lid not cycled                                            |
 |                                     |                                                                         |
 | CMD : 03050704000000000500          | Send incorrect brew command                                             |
 | RESP: c3 05 02 36 03                | Failure (cx) on last command (brew=x3 05), respCode:len(02),data(36 03) |
 |                                     | Reason(36 03) Wrong command                                             |
 |                                     |                                                                         |
 | CMD : 0110080000010061020024        | Send prepare brew command, while not having cycled lid                  |
 | RESP: c1 10 02 23 60                | Failure (cx), cmd x110 ok status=2360                                   |
 |                                     |                                                                         |
 | CMD : 0110080000010061020024        | Send prepare brew command, after cyclcing lid                           |
 | RESP: 81 10 01 20                   | Success (8x) last cmd x110 ok Reason=20                                 |
 |                                     |                                                                         |
 | CMD : 03060102                      | Send abort command (while not brewing)                                  |
 | RESP: c3 06 01 21                   | Failure (cx) on last command (cmd=x3 06), respCode:len(01),data(21)     |
 |                                     | Reason(21)                                                              |
 +-------------------------------------+-------------------------------------------------------------------------+
Other errors: 
 * 0x2403 => Trays full

After reboot, the characteristics is empty.
```


Slider status
-------------
The capsule slider status is on characteristic 06aa3a22-f22a-11e3-9daa-0002a5d5c51b

```
Slider status
 +------+-----------+---------------------------------------------------+
 | Byte |    Value  | Description                                       |
 +------+-----------+---------------------------------------------------+
 |  B0  |    0x00   | Slider is open                                    |
 |      |    0x02   | Slider is closed                                  |
 +------+-----------+---------------------------------------------------+
```

Second status - tbd
------------------
Not really sure what this is, yet...
But it appears that the app reads this after writing the recipe brew.
0x0026 (R)

What I noticed after performing prepare recipe in the Nespresso App, reading the value was
 - 811001200000.... (20B)
 - and after brew command the value was:
 - 830501200000.... (20B)


TODO's
======
Here is a list of things I would like to progress on, in case I have that time somewhere in the future.

TODO's on the Code:
-------------------
- Add status on the machine,
- Add a display and some knob, allowing me to configre my different options
- Since the BLE has too little reach, I'm thinking of making a web-service, suggestions are of course welcome.
- There is absolutely no error handling, or even checks before trying to write
- Stability should be in place here. I haven't tested this yet, AT ALL...
- Next step would be to "arm" the brewbutton, so I by mistake don't break it

Other protocol details I plan to investigate:
---------------------------------------------
- It would be great to be able to understand how the AUTH_KEY is generated/retrieved, alternatively add a server that emulates the nespresso machine, and stores the AUTH_KEY from the App.
- Can I query the status of the lid (RESOLVED)
- Can I query if the lid has been opened since last cycle? (RESOLVED - CMD_RESP_STATUS)
- Would like to investigate the scheduling option
- Can I program the dials?
- Planning on reversing a little more (status 0x0026) and also try to understand time program
- What other data can I capture

Tested machines
---------------
* Tested on nespresso expert
* Tested on nespresso prodigio&milk by @tikismoke


Comments from @tikismoke (thanks!)
=======================
Nespresso prodigio&milk
Descaling ("detartrage" in french) so i have different value i hope to find what byte (or value changed) when i'll done it.
For example the for value i get:

```
06aa3a12-f22a-11e3-9daa-0002a5d5c51b
44 09 80 0C C0 00 00
44 09 80 2D C0 00 00
44 09 07 2D C0 00 00
44 09 07 1D 0C C0 00 00 00
```
Caps remaining
-------------
06aa3a15-f22a-11e3-9daa-0002a5d5c51b is the caps remaining in stock in hex format (if you have add them in the apps)
Value got from 00:00 to 03:E8 (from 0 caps to 1000).
```
06aa3a12-f22a-11e3-9daa-0002a5d5c51b
44 09 80 0C C0 00 00
44 09 80 2D C0 00 00
44 09 07 2D C0 00 00
44 09 07 1D 0C C0 00 00 00
```
But it can also contain from 03:E9 to FF:FF and in apps it appears a '+' instead of the caps value and ask you to follow caps purchase in apps (as if it was deactivate)
The apps can notify you when value is low.

Water hardness
--------------
byte 3.0 (04 or 03) is the "water hardness" ("dureté de l'eau" in french) set in the apps (from 0 to 4)
Try to send 5 or more and the value change to 4 automatically (seems that the cofee machine maintain it too a correct value)
```
06aa3a44-f22a-11e3-9daa-0002a5d5c51b
07 08 04 00
07 08 03 00
```

Other notes:
When a pod wasn't punctured (and hence no coffee flow) the error was 41840018000

Nespresso w/ WebBluetooth
=========================
Recently I have been working on learning Web-Bluetooth and JavaScript, and what is better
than to have an  actual device to work on.
The current implementation is very experimental (as always), "brewbutton.html".
I made an simple implementation of the protocol in Web Bluetooth/JavaScript. There are quite many things left to manage, but it's a start.

Some caveats here:
* Unless you use local host, you need to have SSL (e.g. letsencrypt & Apache)
* You may need to have experimental settings in the browser (To be verified)
* You need Windows 10, BLE does not work on W7
* Chrome / (new) Edge works fine, but NOT Safari (and no plans)
* Android - works with Chrome
* iOS - tested with BlueFy
* iOS is not really working today, and currently no plans
* Have tried WebBluetooth w/ Chrome on W10 and MacBook Pro from 2012, and works fine.
* On Windows 10, you MUST pair the Nespresso machine from Windows control panel.

Issues with this:
* Same issue at the arduino version, we do not check that hte lid have been cycled.

Specification and examples:
* https://webbluetoothcg.github.io/web-bluetooth/
* https://github.com/WebBluetoothCG/
* https://googlechrome.github.io/samples/web-bluetooth/ - A bunch of good examples
* https://www.smashingmagazine.com/2019/02/introduction-to-webbluetooth/ - Good intro to Web Bluetooth


ISSUE TRACKING ARDUINO VERSION
==============================
There are some outstanding issues, will be further investigated.
* Need to resolve this: when descaler is needed, there seem to be a discrepancy between B1.6 (Expert) or B0.2 (prodigio&milk). One or the other or both. This needs to be weeded out
* ...
