/**
 * NESPRESSO BLE INTERFACE
 * In the shitty NESPRESSO coffee maker, it is not possible to program
 * the amount of water that goes through the capsule from the outside.
 * Therefore this setup was needed. Was thinking of throwing it out.
 * But realized I neede the BLE Client skills for a number of other devices at home
 *  * 
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 * 
 * https://gist.github.com/kolontsov/352f51859d15ee4e66368122bbf4d7c5
 * https://github.com/nkolban/ESP32_BLE_Arduino
 * 
 */

/*
Nespresso Expert machine Hack (Bluetooth)
Nespresso Expert machine has the Bluetooth ability which officially can be used only by Nespresso mobile app and it does not offer any API for 3rd party applications and services. Moreover, the Bluetooth services and characteristics are not documented and easy to use by the other Bluetooth libraries plus there is no documentation for the Bluetooth packets payload that need to be sent or received.
However, after searching a lot and sniffing the packets for a couple of days, I've been able to hack the machine and write the small nodejs application using noble and express to control and monitor the machine with Rest API exposed by express through Bluetooth connection. As I did this application for my ex-company and they are still using it for their demo I cannot share the code but I'm going to explain how it works.

Thanks to this repo: https://github.com/fsalomon/nespresso-expert-ble and also this nice medium post https://medium.com/@urish/reverse-engineering-a-bluetooth-lightbulb-56580fcb7546 that basically helped me to understand how I need to sniff the packets.

Sniffing
first I have installed the Nespresso app on my mobile and I connected to the machine and used the application. Then, using the post I mentioned above, I sniffed the packets and opened them in the Wireshark to analyze them.

Authentication
I notice that everytime on the app we start communicating with the machine (brew or monitor or just oppening machine tab) it starts the conversation with a packet that has "8" at the start of its value and has 16 characters (mine was 85c55bc324a4170b) and it is writting on the 4th characteristic of the service 06aa1910f22a11e39daa0002a5d5c51b. note: only one mobile can connect to the expert machine and everytime that new mobile connects (which needs the device reset) the authentication packet will change.

I have used noble library for ble communication. I mention some hints ;) Authentication will be done by writing to 4th characteristic of the first service Buffer.from("85c55bc324a4170b", "hex"). after writing when you read the 5th characteristic, the data should be "2" which means authenticaton was successful.

Communication
I don't know why, and dont ask me please but before sending the brew command you need to write this Buffer.from("01100800000200c8000000", "hex") on the 4th charac of the second service ("06aa1920f22a11e39daa0002a5d5c51b") and then inside the callback, you write this Buffer.from("03050704000000000101", "hex") which is low temp espresso. Now you machine should start brewing... yayyyy

below I leave all of my notes in case it can be useful

Brewing Types
Here are my observations:

0305070400000000 00 00 medium ristretto
0305070400000000 01 01 low espresso
0305070400000000 02 02 high lungo
0305070400000000 01 04 low hot water
0305070400000000 01 05 low americano
0305070400000000 01 07 low probably cleaning mode (all lights turning on together)
03060102 would stop the brewing (not always)

Machine Alarms (Errors)
On the second service, on 0th charac read normally is like 0:64 1:9 2:13 3:64 4:128 5:0 6:255 7:255.

0: 64 is ok, 65: no water
1: 64 with 132 is brewing or busy , 64 with 2 ready , 64 with 66/67/7x :full disposal or no disposal on second service, on 4th charac read normally is like: 0:129 1:16 2:1 3:32 4:0 5:0 6:0 7:0 8:0 9:0 10:0 11:0 12:0 13:0 14:0 15:0 16:0 17:0 18:0 19:0
For understanding slide error, read should be done after command of brew

1: 195 slider error, 129 ok, 131 busy brewing
*/
/*
TODO'S:
* Implement onan ESP32 Wroom
* Add a hardware button
* Add status on the machine, and potentially a display
*/
#define CONFIG_BLE_SMP_ENABLE 1
#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "BLEDevice.h"
//#include "BLEScan.h"

// Water temperature
#define TEMP_LOW 	"01"
#define TEMP_MEDIUM	"00"
#define TEMP_HIGH	"02"

// Type of coffee
#define RISTRETTO	"00"
#define ESPRESSO	"01"
#define LUNGO	    "02"
#define AMERICANO	"04"
#define HOTWATER	"05"
#define RECIPE		"07"
char cmd[128];    // UGLY hack, which re-uses a dedicated buffer
int _MY_LOG( char *tag, int rc)
{
    Serial.print(tag);
}


/*
#define uint_16   unsigned int
unit_16 adjust_range(uint_16 x, uint_16 y, uint_16 z) 
{
  if (x<y)
    return y;
  if (x>z)
    return z;
  return x;
}

#define MAX_COFFEE_SIZE 140
#define MIN_COFFEE_SIZE 15
#define MAX_WATER_SIZE  300
#define MIN_WATER_SIZE  25
// size in ml.
char *brew_recipe(uint_16 coffee_size, uint_16 water_size, byte reversed )
{
  cmd[0]=0x00;
  coffee_size = adjust_range(coffee_size, MIN_COFFEE_SIZE, MAX_COFFEE_SIZE);
  water_size  = adjust_range(water_size, MIN_WATER_SIZE,MAX_WATER_SIZE);
  if ( reversed ) {
    strcat("02");

  }
  else {
    strcat( cmd, "01" );
    strcat( cmd, itoa(coffe_size));
    strcat( cmd, "02" );
    strcat( cmd, itoa(coffe_size));

  }
  return cmd;  
}
*/
char *brew_cmd(char *vol, char *temp )
{
	strcpy(cmd, "0305070400000000");
	strcat(cmd, temp);
	strcat(cmd, vol);
	return cmd;
}

/*********************** END BREW COMMAND STUFF ***********************/

// The remote service we wish to connect to.
// NESPRESSO DEVICE UUID

        // Auth Characteristics


// The characteristic of the remote service we are interested in.
// CMD characteristics


static BLEUUID deviceServiceUUID( "06aa1910-f22a-11e3-9daa-0002a5d5c51b" );

//static BLEUUID serviceUUID( "06aa1910-f22a-11e3-9daa-0002a5d5c51b" );
static BLEUUID authServiceUUID( "06aa1910-f22a-11e3-9daa-0002a5d5c51b" );
static BLEUUID cmdStatusServiceUUID( "06aa1920-f22a-11e3-9daa-0002a5d5c51b" );

static BLEUUID authCharUUID( "06aa3a41-f22a-11e3-9daa-0002a5d5c51b" );
static BLEUUID cmdCharUUID( "06aa3a42-f22a-11e3-9daa-0002a5d5c51b" );
static BLEUUID statusCharUUID( "06aa3a12-f22a-11e3-9daa-0002a5d5c51b" );

// Characteristics
static BLERemoteCharacteristic* pRemoteCharacteristicAuth;
static BLERemoteCharacteristic* pRemoteCharacteristicCommand;
static BLERemoteCharacteristic* pRemoteCharacteristicStatus;



static String nespressoDeviceAuth = "879608e27cb1f96e"; // replace with your auth (from sniffer)

//static String nespressoDeviceId[] = "D0:09:E2:58:54:B8"; // replace with your device id
// Verify it simply with e.g. RENESAS GATT BROWSER, and write auth and then CMD



static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};


bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");


    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);   //

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
 
    BLERemoteService* pRemoteService = pClient->getService(authServiceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(authServiceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    
    Serial.println(" - Found our service");
    Serial.println(authServiceUUID.toString().c_str());

	  // FIRST NESPRESSO AUTH
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristicAuth = pRemoteService->getCharacteristic(authCharUUID);

    if (pRemoteCharacteristicAuth == nullptr) {
      Serial.print("Failed to find our AUTH characteristic UUID: ");
      Serial.println(authCharUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(authCharUUID.toString().c_str());
    Serial.println(" - Found AUTH characteristic");

    if (pRemoteCharacteristicAuth->canWrite()) {
        Serial.println("The AUTH characteristic can be written to");
    }


    pRemoteService = pClient->getService(cmdStatusServiceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(cmdStatusServiceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    
    Serial.println(cmdStatusServiceUUID.toString().c_str());
    Serial.println(" - Found our CMD service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristicCommand = pRemoteService->getCharacteristic(cmdCharUUID);

    if (pRemoteCharacteristicCommand == nullptr) {
      Serial.print("Failed to find our cnd characteristic UUID: ");
      Serial.println(cmdCharUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found CMD characteristic");
    Serial.println(cmdCharUUID.toString().c_str());

     if (pRemoteCharacteristicCommand->canWrite()) {
        Serial.print("The CMD characteristic can be written to");
    }


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristicStatus = pRemoteService->getCharacteristic(statusCharUUID);

    if (pRemoteCharacteristicStatus == nullptr) {
      Serial.print("Failed to find our cnd characteristic UUID: ");
      Serial.println(statusCharUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found STATUS characteristic");
    Serial.println(statusCharUUID.toString().c_str());

     if (pRemoteCharacteristicStatus->canRead()) {
        Serial.println("The characteristic value can be read");
        // Read characteristics, is it will force bonding - security
        pRemoteCharacteristicStatus->readValue();

     }
 
//    if(pRemoteCharacteristicAuth->canNotify())
//      pRemoteCharacteristicAuth->registerForNotify(notifyCallback);

    connected = true;

}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(deviceServiceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


// constants won't change. They're used here to set pin numbers:
const int buttonPin = 2;    // the number of the pushbutton pin, should have pullupp/down
const int ledPin = 13;      // the number of the LED pin

// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
// Will have an issue if more than 49 days

void initButton( void )
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  buttonState = LOW;    // Initalize button state
  // set initial LED state
  digitalWrite(ledPin, ledState);
}

// This returns true once if it detects a keypress
int detectPress(void) 
{
  // read the state of the switch into a local variable:
  int fDetectPress = false;
  int inputValue = digitalRead(buttonPin);


  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (inputValue != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (inputValue != buttonState) {
      buttonState = inputValue;
      if ( buttonState )
        fDetectPress = true;    // Only detec on rising edge, debounced
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = inputValue;
  return fDetectPress;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  Serial.println("NESPRESSO BLE STUFF...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  // IO pin set
  initButton();

  Serial.println(uxTaskGetStackHighWaterMark(NULL));
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  
} // End of setup.

int cnt = 0;
// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    
    if ( 0 == cnt ) {    // Just to slow down serial printing
      String newValue = "Time since boot: " + String(millis()/1000);
      Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    }
    cnt++;
    if ( cnt > 100 ) {
      cnt=0;
    }

    if ( detectPress() ) {
      // Time to brew!
      Serial.println("===================");
      Serial.println("Keypress detected: ");
      uint8_t perfectRecipe[] = {0x01,0x10,0x08,0x00,0x00,0x01,0x00,0x82,0x00,0x00,0x00};
      uint8_t perfectRecipeBrew[] = { 0x03, 0x05, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07 };
      uint8_t auth[] = { 0x87, 0x96, 0x08, 0xe2, 0x7c, 0xb1,0xf9,0x6e};
      
      Serial.println("Writing to characteristics\n");
      pRemoteCharacteristicAuth->writeValue( auth, sizeof(auth), true );
      delay(1000);

      pRemoteCharacteristicCommand->writeValue( perfectRecipe, sizeof(perfectRecipe), true );
      delay(100);
      pRemoteCharacteristicCommand->writeValue( perfectRecipeBrew, sizeof(perfectRecipeBrew), true );
      delay(1000);

      delay(100);
      
      Serial.println("The perfect brew is about to be delivered");
      Serial.println("===================");

    }

  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(100); //  a second between loops.
} // End of loop
