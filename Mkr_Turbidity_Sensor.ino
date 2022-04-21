
#include <Wire.h>
#include <DFRobot_ADS1115.h>
#include <RTCZero.h>            //Managing Real Time Clock of the MKR and sleepmode
#include <MKRGSM.h>             //usefull to work with an MKR board
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>                 //to manage files on SD Card
#include <WDTZero.h>            //Watch! The dog will bite you if you sleep too much!!! https://github.com/javos65/WDTZero
#include <NTPClient.h>


#define WATER_TEMP_SENSOR 1
#define AMBIENT_TEMP_SENSOR 0
#define SD_CHIP_SELECT    4       //Communication PIN SD Card
#define LOG_FILE "TURBDAT.csv"
#define GPRS_APN      "mdata.net.au" // replace your GPRS APN  --> "mdata.net.au" for ALDI
#define GPRS_LOGIN        ""      //no login required for the carrier for ALDI
#define GPRS_PASSWORD     ""      //no password required for the carrier for ALDI

WDTZero watchDog;
File myFile;                        //specific class to use files
RTCZero rtc;              //Real Time clock
GSM gsmAccess(false);            //access to the network, true if you want to have all messages from the GSM chip
GSMUDP timeUDP;
GPRS gprs;
NTPClient timeClient(timeUDP,"pool.ntp.org");

// Setup a oneWire instance to communicate withf any OeWire device
OneWire oneWireWater(WATER_TEMP_SENSOR);
DallasTemperature sensorInWater(&oneWireWater);

OneWire oneWireAmbient(AMBIENT_TEMP_SENSOR);
DallasTemperature sensorInAmbient(&oneWireAmbient);

// Pass oneWire reference to DallasTemperature library

DFRobot_ADS1115 ads(&Wire);

unsigned long epochTime;
String dateAndTime;
unsigned long counter;
float aTemp, wTemp;
int16_t adc0, adc1, adc2, adc3;
void setup(void)
{
  Serial.begin(9600);
  watchDog.setup(WDT_HARDCYCLE16S);
  timeClient.begin();
  gsmConnect(); //Enable once sim is registered
  isCardMounted();
  counter = readFile("counter").toInt();
  rtc.begin();
  sensorInWater.begin();
  sensorInAmbient.begin();
  ads.setAddr_ADS1115(ADS1115_IIC_ADDRESS0);   // 0x48
  ads.setGain(eGAIN_TWOTHIRDS);   // 2/3x gain
  ads.setMode(eMODE_CONTIN);       // continuous mode for less noise
  ads.setRate(eRATE_8);          // 8 SPS for less noise
  ads.setOSMode(eOSMODE_SINGLE);   // Set to start a single-conversion
  ads.init();
}


void loop(void) {
  watchDog.clear();
  epochTime = timeClient.getEpochTime() + 36000;
  rtc.setEpoch(epochTime);
  getRtcTime();
  sensorInWater.requestTemperatures();
  sensorInAmbient.requestTemperatures();
  aTemp = sensorInAmbient.getTempCByIndex(0);
  wTemp = sensorInWater.getTempCByIndex(0);
  {
    if (ads.checkADS1115())
    {
      adc0 = ads.readVoltage(0);
      adc1 = ads.readVoltage(1);
      adc2 = ads.readVoltage(2);
      adc3 = ads.readVoltage(3);

    }
    else
    {
      Serial.println("ADS1115 Disconnected!");
    }
        if (counter % 10 == 0) {
    
    //print the temperature in Celsius
    Serial.print("Ambient Temp: ");
    Serial.print(aTemp);
    Serial.print("C  |  ");
    //print the temperature in Celsius
    Serial.print("Water Temp: ");
    Serial.print(wTemp);
    Serial.println("C  |  ");

    Serial.print("Sensor0:");
    Serial.print(adc0);
    Serial.print("mV,  ");

    Serial.print("Sensor1:");
    Serial.print(adc1);
    Serial.print("mV,  ");

    Serial.print("Sensor2:");
    Serial.print(adc2);
    Serial.print("mV,  ");

    Serial.print("Sensor3:");
    Serial.print(adc3);
    Serial.println("mV");}
    isCardMounted();
    SaveData();
    counter += 1;
    ChangeParameter("counter", String(counter)); // Used to remember what count is next
    //if (counter % 4000 == 0) {
     // gsmConnect();
    //}
  }

}
void isCardMounted() {
  if (SD.begin(SD_CHIP_SELECT)) {
    return;
  }
  Serial.println("Checking SD card..");
  while (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    delay(1000);
  }
  Serial.println("SD Card Mounted");
}

void gsmConnect() {
  // connection state
  boolean notConnected = true;

  // Start GSM shield
  // If your SIM has PIN, pass it as a parameter of begin() in quotes
  while (notConnected)
  {
    if (gsmAccess.begin() == GSM_READY && gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY) {
      notConnected = false;
      Serial.println("Connected to network");
      timeClient.update();
    }
    else
    {
      Serial.println("Not connected");
      delay(1000);
    }
  }
}

void SaveData() { //save all data on the SD card
  String csvObject = "";
  if (!SD.exists(LOG_FILE)) {
    csvObject = "DateTime [DDMMYY HH:MM:SS],Timestamp[s],Measure_number[-],Ambient_temp[c],Water_temp[c],Sensor0_voltage[mV],Sensor1_voltage[mV],Sensor2_voltage[mV],Sensor3_voltage[mV],\n";
  }
  myFile = SD.open(LOG_FILE, FILE_WRITE);
  csvObject = csvObject + dateAndTime + "," + String(epochTime) + "," + String(counter) + "," + String(aTemp) + "," + String(wTemp) + "," + String(adc0) + "," + String(adc1) + "," + String(adc2) + "," + String(adc3);
  if (myFile) {
    myFile.println(csvObject);
    myFile.close();
  }
}

void getRtcTime() {
  dateAndTime = print2digits(rtc.getDay()) + "/" + print2digits(rtc.getMonth()) + "/" + print2digits(rtc.getYear()) + " " + print2digits(rtc.getHours()) + ":" + print2digits(rtc.getMinutes()) + ":" + print2digits(rtc.getSeconds());
}

String print2digits(int number) { //print correctly dates
  String numback = "";
  if (number < 10) {
    numback = "0"; // print a 0 before if the number is < than 10
  }
  numback = numback + String(number);
  return numback;
}

String readFile(String nameFile) { //read the content of a file from the SD card
  String msg = "'--Reading file " + nameFile + ".txt: ";
  String result;
  nameFile = nameFile + ".txt";
  if (SD.exists(nameFile)) {
    //    msg=msg+"file exists! ";
    myFile = SD.open(nameFile);
    result = "";
    while (myFile.available()) {
      char c = myFile.read();
      if ((c != '\n') && (c != '\r')) {
        result += c;
        delay(5);
      }
    }
  }
  else {
    result = "0";
    myFile = SD.open(nameFile, FILE_WRITE); {
      myFile.println(result);
      myFile.close();
    }
  }
  return result;
}

void ChangeParameter (String nameparameter , String value) { //all values should be given as string
  String name = nameparameter + ".txt";
  if (SD.exists(name)) {
    SD.remove(name);
  }
  myFile = SD.open(name, FILE_WRITE);
  if (myFile) {
    myFile.println(value);
    myFile.close();
  }
}
