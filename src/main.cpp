/******************************************************************************/
/** Programm zur Übertragung der Messdaten der Sensoren auf dem Board                                  **/
/******************************************************************************/
/****                         Version vom 03.05.2022                       ****/
/******************************************************************************/
/*Das ist der Code der sensorboard clients, die im Zug/Bus verteilt sind. Wir müssen jedeglich das define OwnNuber am anfang ändern. **/
 #define OwnNuber 0//siehe comsSetup.h für die dokumentation der Kommunikation

/******************************************************************************/
// here comes the stuff we need to get the actual data fromt he sensors
#include <Arduino.h>
#include <printmatrix.h>
#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>
#include <vl5helper.h>
// The documentations is obove every function.
// hover over the function to get the info written above
vl5helper test;
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData; // Result data class structure, 1356 byes of RAM

int imageResolution = 0; // Used to pretty print output
int imageWidth = 0;      // Used to pretty print output
int array[64] = {0};

// here comes the stuff we need to communicate to the serverSensorboard
/* #include <coms.h> */

/*** Setup-Routine ************************************************************/

void comsSetup(int IndividualclientNumber);
void comsLoop(int Ein, int Aus);
typedef struct TxStruct
{
 int Einsteigeer;//wir müssen eigenlich nur das delta zurückgeben aber aus analythischen Grüden ist es sinvoll Einsteiger und Ausseiger zu übergeben.
 int Aussteiger;
  
}TxStruct;
TxStruct sentData;

/*** Struct-Variable in der die empfangenen Daten enthalten sind *************/  
typedef struct RxStruct
{
 int doorstate=1;

}RxStruct;
RxStruct receivedData;


void setupSensory();
int doorstate=1;
void setup()
{
  Serial.begin(115200);
  Serial.printf("this is the %dth client \n",OwnNuber);
  comsSetup(OwnNuber);
  setupSensory();
  
}
/******************************************************************************/
void loopSensor();

unsigned long int timerg;
bool temploop = false;
/**** Loop-Funktion ***********************************************************/
void loop()
{
  timerg=millis();
  while(receivedData.doorstate==0){//receivedData.doorstate should be there but now there is a true for 5 seconds
    loopSensor();
temploop = true;
  }
  if (temploop)
  {
    temploop = false;
  
    comsLoop(test.Einsteiger,test.Aussteiger);//send the data once

  }
  //Serial.println(receivedData.doorstate); // Empfangen Daten anzeigen

}


//this is to set up the sensor e.g. the vl53l5cx
void setupSensory()
{
  
   //Wire.begin(21,22,1000000); //SDA 21   SCL is 22  This is for a correct module                     // This resets to 100kHz I2C
  Wire.begin(22, 21, 1000000); // nope SDA is 22 and SCL is 21 wrong pins so we can't use Wire.begin();
  // Wire.setClock(1000000);             // Run sensor out of spec but it's working
  myImager.setWireMaxPacketSize(128); // Increase default from 32 bytes to 128 - not supported on all platforms but it is sopported on esp32 so we are going to use it in our people counting application
  Serial.println("Initializing sensor board. This can take up to 10s. Please wait.");
  if (myImager.begin() == false)
  {
    Serial.println(F("Sensor not found - check your wiring. Freezing"));
    while (1)
      ;
  }
  myImager.setResolution(8 * 8);              // Enable all 64 pads
  imageResolution = myImager.getResolution(); // Query sensor for current resolution - either 4x4 or 8x8
  imageWidth = sqrt(imageResolution);         // Calculate printing width
  myImager.setRangingFrequency(15);

  // additional setting are not improtant but take a look into the example
  myImager.startRanging();
  test.resetEEPROM(); // for resetting the Einsteiger und Aussteiger varialbles to 0
}
void loopSensor(){
   if (myImager.isDataReady() == true)
    {
      if (myImager.getRangingData(&measurementData)) // Read distance data into array
      {
        // The ST library returns the data transposed from zone mapping shown in datasheet
        // Pretty-print data with increasing y, decreasing x to reflect reality
        for (int y = 0; y <= imageWidth * (imageWidth - 1); y += imageWidth)
        {
          for (int x = imageWidth - 1; x >= 0; x--)
          {
            array[x + y] = measurementData.distance_mm[x + y];
            // Serial.print("\t");
            // Serial.print(measurementData.distance_mm[x + y]);
          }
          // Serial.println();
        }
        // Serial.println();
      }
    }
    // recommended around 70-300ms
    delay(70); // should not go lower than delay(70); milliseconds

    test.run(&array[0], 1000); // will return -1 or 0 or 1
    Serial.print("time: ");
    Serial.println(millis());
    Serial.print("\nAusseiger ");
    Serial.print(test.Aussteiger);
    Serial.print(" Einsteiger ");
    Serial.println(test.Einsteiger);
    
}



 #include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
//#define OwnNumber 5//DAs ist Sehr WICHTIG!!!!!!!!!!!!!!! das zeigt die position des sensorboards im Zug/Bus an.!!!!!!
//obere line sollte in der main stehen für easy access und sichtbarkeit



//here comes the stuff we need to communicate to the serverSensorboard   






/*** Struct-Variable in der die zu sendenden Daten geschrieben werden *********/  


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status); //callback function



uint8_t RxMACaddress[] = {0xC8, 0xC9, 0xA3, 0xCC, 0x1B, 0x6C};
//wird für den setub gebraucht
//@param IndividualclientNumber Das ist Die wichtige nummerierung des Clients, welche in die Mac addresse einfliest
void comsSetup(int IndividualclientNumber){ 

WiFi.mode(WIFI_STA);  // Aktivieren des WiFi im Station-Mode


//this is the default mac address
uint8_t templateMac[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x60};
//the next address is +1
//this means the second is 0x61 and the thirsd is 0x62 ....
/***** MAC-Adresse des Empfaengers festlegen **********************************/


  templateMac[5]=templateMac[5]+IndividualclientNumber;//Here happens the magic and the nodes get the right mac address assigned
esp_wifi_set_mac(WIFI_IF_STA, &templateMac[0]);
  /*** Aktivieren der ESP-Now Funktion *****************************************/
  if(esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  /*** Sendefunktion aufrufen **************************************************/
  esp_now_register_send_cb(OnDataSent);
  
  /*** Client der angegebenen MAC-Adresse ausfindig machen *********************/
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, RxMACaddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  /*** Status des Peering-Prozesses ueberprüfen ********************************/
  if(esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }

  /*** Funktion OnDataRecv aufrufen, sobald neue Daten einlaufen ***************/
  esp_now_register_recv_cb(OnDataRecv);
}
    /***** MAC-Adresse des Empfaengers festlegen **********************************/
//Sendet die Übergebenen Werte zum server, wessen Mac bekannt ist.
//@param Ein int Einsteiger
//@param Aus int Aussteiger
void comsLoop(int Ein, int Aus){
sentData.Aussteiger=Aus;
sentData.Einsteigeer=Ein;

     esp_err_t result = esp_now_send(RxMACaddress, (uint8_t *) &sentData, sizeof(sentData));
}
/*** Funktion die aufgerufen werden, wenn Daten gesendet werden ***************/
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) //callback function
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
/******************************************************************************/
/*** Funktion die aufgerufen werden, wenn Daten empfangen werden ***************/
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  Serial.print(receivedData.doorstate);
}
/*******************************************************************************/