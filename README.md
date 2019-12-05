brewbutton
============
The nespresso expert is a piece of shit and the app even more. So this project aims to replace the mobile app, and allow you to brew my own stuff. The only reason I started banging my head on this BLE protocol, was that the default "lungo" on the front dial added too much water, making it undrinkable, and to overcome the crappy App and the Crappy default settings on the machine.

The brewbutton, is a simple piece of code that connects to an nespresso machine, and when triggered on the IO PIN, it will send a simple brew command. Right now, it's hard-coded to a 130ml coffee recipe brew. 
My idea is to have a small brew button that can brew my morning coffe the way I want it.

I have contributed a little to the reverse engineered of the protocol, see below. 

HOW TO START
============

HW and environment setup
------------

The brew button is based on a simple ESP32 example, where I only added a debounced PIN, and  stuff needed to program the nespresso machine.
* NodeMCU ESP-32s(v1.1) with Bluetooth Low Energy support
* I used Arduino IDE 1.8.10
* ESP32 BLE Arduino v1.0.1
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

TODO'S on the Code:

* Add status on the machine, and potentially a display
* Since the BLE has too little reach, I'm thinking of making a web-service, suggestions are of course welcome
* There is absolutely no error handling, or even checks before trying to write


RETRIEVE AUTH-KEY
-----------------

In order to start this, you need to retrieve the AUTH-KEY. For this, I find this a fairly simple step-by-step instruction.

* Using Samsung Galaxy Note 4
* Go to developer options in settings, enable BLE HCI snoop
* Start the Nespresso App (it should already be registered)
* Stop BLE HCI snoop.
* Connect the mobile to USB-PC, snoop file can be found here: \\Galaxy Note4\Phone\Android\data
* Install wireshark (wireshark.org)
* Load the file in wire-shark
* Look for AUTH-key Write Request to 0x0014 (Service 06aa3a41-f22a-11e3-9daa-0002a5d5c51b)
* Extract the AUTH-KEY (8bytes), mine is "879608e27cb1f96e"

TEST AUTH-KEY
-------------
* Downloaded on  GATTBrowser (Renesas)
* Find and connect to device
* Find auth service characteristics 06aa3a41-f22a-11e3-9daa-0002a5d5c51b
* Write AUTH-KEY to this register
* Find service characteristics 06aa3a42-f22a-11e3-9daa-0002a5d5c51b
* Write 03050704000000000105
* If you managed to do these steps above correctly  you should be brewing an Americano


PROTOCOL ANALYSIS
==================
There exist some already done reverse engineering. I have tried to gather what was missing, in order to build what I wanted.

RECIPE BREW
-----------
To brew a recipe coffee, allows you to specify the amount of coffee, the amount of water, and how hot the water should be.
This comes as two separate write requests to the same service 06aa3a42-f22a-11e3-9daa-0002a5d5c51b (0x0024)

#1 - Prepare Command : 


```
In investigated the status service (06aa3a12-f22a-11e3-9daa-0002a5d5c51b) (0x001C)
 
 Prepare command:
 +---------------------------------------------+
 |  01 10 08 00 00 {01 00 61} {02 00 24}       |
 +---------------------------------------------+
 | - 01 = coffe, 00 61 = 97 ml                 |
 | - 02 = water, 00 24 = 37 ml                 |
 | - It's possible to reverse these two,       |
 |   if you want water first.                  |
 +---------------------------------------------+

Execute command:
 +---------------------------------------------+
 |  03 05 07 04 00 00 00 00 {00} {07}          |
 +---------------------------------------------+
 | - 00 = Hot                                  |
 | - 07 = Recipe                               |
 +---------------------------------------------+

What I noticed was that when water ran out, "water engaged" was still active, as it hadn't reached it's volume
```




STATUS
------

```
In investigated the status service (06aa3a12-f22a-11e3-9daa-0002a5d5c51b) (0x001C)
 +------+-----------+------------------+
 | Byte |    Bit    | Description      |
 +------+-----------+------------------+
 |  B0  | xxxx xxx1 | Empty water      |
 +------+-----------+------------------+
 |  B1  | 1xxx xxxx | Capsule engaged  |
 |      | xxxx x1xx | Water engaged    |
 |      | xxxx xx1x | Idle             |
 +------+-----------+------------------+

What I noticed was that when water ran out, "water engaged" was still active, as it hadn't reached it's volume
```
Examples:
- Idle:	       "40 02 01 E0 40 00 FF FF"
- Coffe:  	    "40 84 01 E0 40 00 FF FF"
- Water:	      "40 04 01 E0 40 00 FF FF"
- Empty Water: "41 84 01 E0 40 00 FF FF" (capsule still locked in)
- Bucket full: tbc 


Other status - tbd
------------------
0x26 (R)


Other protocol details I plan to investigate:
---------------------------------------
- How is the auth-key generated by the App, through cloudAPI? 
- Can I query the status of the lid
- CanI query if the lid has been opened since last cycle?
- Will investigate the scheduling option
- Can I program the dials?
- Planning on reversing a little more (status 0x0026) and also try to understand time program
- What other data can I capture (sleep, etc?)
