# brewbutton
nespresso brewbutton
The nespresso expert is a piece of shit and the app even more. What I needed was a small brew button that can brew my morning coffe the way I want it. Therefore I reverse engineered the stuff. 
https://gist.github.com/petergullberg/db0aada23240181b79858d20e0578cab/edit

The brew button is based on a simple ESP32 example, where I only added a debounced PIN, and the stuff needed to program the nespresso machine.

Stuff used
---------------
* NodeMCU ESP-32s(v1.1)
* Arduino IDE 1.8.10
* ESP32 BLE Arduino v1.0.1


Changes from default
--------------------
From the original setup I had to make a few changes
* I based the work on ESP32 BLE_Client
* I hardcoded the #define CONFIG_BLE_SMP_ENABLE, in BLEDEvice.cpp, and BLEClient.cpp to ensure security is enabled (to be honest I haven't spent enough time, understanding how to makefiles works in Arduino
* FIX a bug in BleRemoteService.cpp on line 175: uint16_t count = 1;
* Add the line: BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT); in onConnect

On top of this, I also discvered a bug on my windows installation, that prevented me from compiling 

