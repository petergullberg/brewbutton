brewbutton
============
The nespresso expert is a piece of shit and the app even more. So this project aims to replace the mobile app, and allow you to brew your own stuff. The only reason I started banging my head on this BLE protocol, was that the default "lungo" on the front dial added too much water, making it undrinkable, and to overcome the crappy App and the Crappy default settings on the machine.

The brewbutton, is a simple piece of code that connects to an nespresso machine, and when triggered by the IO PIN, it will send a simple brew command. Right now, it's hard-coded to a 130ml coffee recipe brew. 
My idea is to have a small brew button that can brew my morning coffe the way I want it.

I have contributed a little to the reverse engineered of the protocol, see below. 

HOW TO START
============

HW and environment setup
------------

The brew button is based on a simple ESP32 example, where I only added a debounced PIN, and  stuff needed to program the nespresso machine. There might be alternatives here, but I put it here as a reference.
* Arduino IDE 1.8.10 
* NodeMCU ESP-32s(v1.1) with Bluetooth Low Energy support. https://hitechchain.se/iot/esp32-wemos-mini-d1-wifi-bluetooth
* ESP32 BLE Arduino v1.0.1 library
* GPIO2 <= with INTERNAL PULL_UP - hook up a button to ground..
* To setup the ESP32 environment in Arduino IDE, I followed this one: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/


Code Changes from default
-------------------------
From the original setup I had to make a few changes
* I based the work on ESP32 BLE_Client
* I hardcoded the #define CONFIG_BLE_SMP_ENABLE, in BLEDEvice.cpp, and BLEClient.cpp to ensure security is enabled (to be honest I haven't spent enough time, understanding how to makefiles works in Arduino
* FIX a bug in BleRemoteService.cpp on line 175: uint16_t count = 1;
* Add the line: BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT); in onConnect

On top of this, I also discvered a bug on my windows installation, that prevented me from compiling, but that was solved by a simple mklink....

RETRIEVE AUTH-KEY
-----------------

In order to start this, you need to retrieve the AUTH-KEY. For this, I find this a fairly simple step-by-step instruction.

* Using Android (I used Samsung Galaxy Note 4)
* Go to developer options in settings, enable BLE HCI snoop
* Start the Nespresso App (you need to have registered this and connected to your machine already)
* Brew a cup of coffee
* Stop BLE HCI snoop.
* Connect the mobile to USB/PC and copy/email the file (On Samsung, the snoop is here: \\Galaxy Note4\Phone\Android\data\btsnoop_hci.log)
* Install wireshark (wireshark.org), learn a little about it: https://medium.com/@urish/reverse-engineering-a-bluetooth-lightbulb-56580fcb7546
* Load the file in wire-shark
* Look for Write Request to 0x0014 (Service 06aa3a41-f22a-11e3-9daa-0002a5d5c51b), the data part is the AUTH-key
* Extract the AUTH-KEY (8bytes), mine is "879608e27cb1f96e"

TEST AUTH-KEY
-------------
* Downloaded the GATTBrowser from Renesas, you have it on both iPhone/Android
* Find and connect to the nespresso device
* Find auth service characteristics 06aa3a41-f22a-11e3-9daa-0002a5d5c51b
* Write AUTH-KEY to this register
* Find service characteristics 06aa3a42-f22a-11e3-9daa-0002a5d5c51b
* Write 03050704000000000105
* If you managed to do these steps above correctly  you should be brewing an Americano


PROTOCOL ANALYSIS
==================
There exist some already done reverse engineering. I have tried to gather what was missing, in order to build what I wanted.
I used Wireshark and GATTBrowser from Renesas, both where quite handy. And you could also analyze status, while brewing manually.

STANDARD BREWS
--------------
Standard brews, are those that are on the front dial. PLease note that the Americano contains way too much water, and is not a good choice.
,,,
0305070400000000 00 00 medium ristretto
0305070400000000 01 01 low espresso
0305070400000000 02 02 high lungo
0305070400000000 01 04 low hot water
0305070400000000 01 05 low americano
0305070400000000 01 07 Recipe brew
03060102 would stop the brewing (not always)
,,,
As you can see, the last byte is the type of brew, and the second last is the temperature

RECIPE BREW (07)
-----------
Recipe brew, is where you can select how much coffe you want, the amount of water, and how hot the water should be.
This comes as two separate write requests to the "command service" 06aa3a42-f22a-11e3-9daa-0002a5d5c51b (0x0024)

```
To make a recipe brew, you send two separate commands to the "command service" characteristic 06aa3a42-f22a-11e3-9daa-0002a5d5c51b

 Prepare command:
 +---------------------------------------------+
 |  01 10 08 00 00 {01 00 61} {02 00 24}       |
 +---------------------------------------------+
 | - 01 = coffe, 0x0061 = 97 ml                |
 | - 02 = water, 0x0024 = 37 ml                |
 | - It's possible to reverse these two,       |
 |   if you want water first.                  |
 +---------------------------------------------+

Brew command:
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

 +------+-----------+-------------------------+
 | Byte |    Bit    | Description             |
 +------+-----------+-------------------------+
 |  B0  | xxxx xxx1 | Water is empty          |
 +------+-----------+-------------------------+
 |  B1  | 1xxx xxxx | Capsule engaged         |
 |      | xxxx x1xx | Water engaged           |
 |      | xxxx xx1x | Awake, ok               |
 |      | x1xx xxxx | Door open / Sensor full |
 |      | xxxx 1xxx | Sleeping                |
 |      | xxxx xxx1 | Water temperature low   |
 +------+-----------+-------------------------+
 |  B2  | ???? ???? | tbc                     |
 |      |           | 80                      |
 +------+-----------+-------------------------+
 |  B3  | ???? ???? | tbc                     |
 |      |           | 05, seen 06             |
 +------+-----------+-------------------------+
 |  Bn  | ???? ???? | tbc                     |
 |      |           |                         |
 +------+-----------+-------------------------+

What I noticed was that when water ran out, "water engaged" was still active, as it hadn't reached it's volume.
While brewing coffee, both capsule engage and water engaged are active.

```
Examples:
- Idle:	       "40 02 01 E0 40 00 FF FF"
- Coffe:  	    "40 84 01 E0 40 00 FF FF"
- Water:	      "40 04 01 E0 40 00 FF FF"
- Empty Water: "41 84 01 E0 40 00 FF FF" (capsule still locked in)
- Bucket full: tbc 


Second status - tbd
------------------
Not really sure what this is, yet...
But it appears that the app reads this after writing the recipe brew.
0x0026 (R)

What I noticed after performing prepare recipe reading the value was
 - 811001200000.... (20B)
 - and after brew command the value was:
 - 830501200000.... (20B)


TODO's
======
Here is a list of things I would like to progress on, in case I have that time somewhere in the future.

TODO'S on the Code:
-------------------
- Add status on the machine,
- Add a display and some knob, allowing me to configre my different options
- Since the BLE has too little reach, I'm thinking of making a web-service, suggestions are of course welcome
- There is absolutely no error handling, or even checks before trying to write
- Stability should be in place here. I haven't tested this yet, AT ALL...
- Next step would be to "arm" the brewbutton, so I by mistake don't break it

Other protocol details I plan to investigate:
---------------------------------------
- It would be great to be able to understand how the AUTH_KEY is generated/retrieved, alternatively add a server that emulates the nespresso machine, and stores the AUTH_KEY from the App. 
- Can I query the status of the lid
- Can I query if the lid has been opened since last cycle?
- Would like to investigate the scheduling option
- Can I program the dials?
- Planning on reversing a little more (status 0x0026) and also try to understand time program
- What other data can I capture (sleep, etc?)
