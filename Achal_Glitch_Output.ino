#include <WiFiNINA.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include <ArduinoJson.h>
#include "Wire.h"
#include "Adafruit_MPR121.h"

//mpr stuff

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
uint16_t currtouched = 0;
uint16_t lasttouched = 0;

//**Details of your local Wifi Network

//Name of your access point
char ssid[] = "WIFI NAME";
//Wifi password
char pass[] = "PASSWORD";

int status = WL_IDLE_STATUS;       // the Wifi radio's status

// pubnub keys
char pubkey[] = "pub-c-6d8b57e4-4006-4dab-89a1-219957311232";
char subkey[] = "sub-c-fc64ea22-2a16-11eb-a9aa-e23bcc63a965";

// channel and ID data

const char* myID = "Achal"; // place your name here, this will be put into your "sender" value for an outgoing messsage

char publishChannel[] = "ACHAL"; // channel to publish YOUR data
char readChannel[] = "JAMIE"; // channel to read THEIR data

// JSON variables
StaticJsonDocument<200> dataToSend; // The JSON from the outgoing message
StaticJsonDocument<200> inMessage; // JSON object for receiving the incoming values
//create the names of the parameters you will use in your message
String JsonParamName1 = "publisher";
String JsonParamName2 = "pinTouched";



int serverCheckRate = 1000; //how often to read data on PN
unsigned long lastCheck; //time of last publish


const char* inMessagePublisher;
int JAMIEPIN;  ///the value Nick reads from Kates board via PN
int ACHALPIN; //the value I get locally

int tp = 4; //change this if you aren't using them all, start at pin 0
int ledPins[] = {2, 3, 4, 5};
int ledTotal = sizeof(ledPins) / sizeof(int);
boolean BlinkState[] = {false, false, false, false};
int jamieLastPinTouched = 0;

void setup() {

  Serial.begin(9600);

  //run this function to connect
  connectToPubNub();


  for (int i = 0; i < ledTotal ; i++)
  {
    pinMode(ledPins[i], OUTPUT);
  }

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}


void loop()
{
  //read the capacitive sensor

  MyLeds();
  checkAllPins(tp);


  //send and receive messages with PubNub, based on a timer
  sendReceiveMessages(serverCheckRate);
}



void connectToPubNub()
{
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to the network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    Serial.print("*");

    // wait 2 seconds for connection:
    delay(2000);
  }

  // once you are connected :
  Serial.println();
  Serial.print("You're connected to ");
  Serial.println(ssid);

  PubNub.begin(pubkey, subkey);
  Serial.println("Connected to PubNub Server");
}



void sendReceiveMessages(int pollingRate)
{
  //connect, publish new messages, and check for incoming
  if ((millis() - lastCheck) >= pollingRate)
  {
    //check for new incoming messages
    readMessage(readChannel);

    //save the time so timer works
    lastCheck = millis();
  }
}



void sendMessage(char channel[])
{
  Serial.print("Sending Message to ");
  Serial.print(channel);
  Serial.println(" Channel");

  char msg[64]; // variable for the JSON to be serialized into for your outgoing message

  // assemble the JSON to publish
  dataToSend[JsonParamName1] = myID; // first key value is sender: yourName
  dataToSend[JsonParamName2] = ACHALPIN; // second key value is the Pin number

  serializeJson(dataToSend, msg); // serialize JSON to send - sending is the JSON object, and it is serializing it to the char msg
  Serial.println(msg);

  WiFiClient* client = PubNub.publish(channel, msg); // publish the variable char
  if (!client)
  {
    Serial.println("publishing error"); // if there is an error print it out
  }
  else
  {
    Serial.print("   ***SUCCESS");
  }
}



void MyLeds()
{
  for (int i = 0; i < ledTotal; i++)
  {
    if (BlinkState[i] == true)
    {
      digitalWrite(ledPins[i], BlinkState[i]);
    } else if (BlinkState[i] == false)
    {
      digitalWrite(ledPins[i], BlinkState[i]);
    }
  }
}

void checkAllPins(int totalPins)
{
  // Get the currently touched pads
  currtouched = cap.touched();
  for (uint8_t i = 0; i < totalPins; i++)
  {
    // it if *is* touched and *wasnt* touched before, send a message
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) )
    {
      BlinkState[i] = !BlinkState[i];
      ACHALPIN = i;
      //JAMIEPIN = i;
      Serial.println("hereeeeeee");
      sendMessage(publishChannel);
    }
  }
  lasttouched = currtouched;
}



void readMessage(char channel[])
{
  String msg;
  auto inputClient = PubNub.history(channel, 1);
  if (!inputClient)
  {
    Serial.println("message error");
    delay(1000);
    return;
  }
  HistoryCracker getMessage(inputClient);
  while (!getMessage.finished())
  {
    getMessage.get(msg);
    //basic error check to make sure the message has content
    if (msg.length() > 0)
    {
      Serial.print("**Received Message on ");
      Serial.print(channel);
      Serial.println(" Channel");
      Serial.println(msg);
      //parse the incoming text into the JSON object

      deserializeJson(inMessage, msg); // parse the  JSON value received

      //read the values from the message and store them in local variables
      inMessagePublisher = inMessage[JsonParamName1]; // this is will be "their name"
      JAMIEPIN = inMessage[JsonParamName2]; // the value of their Temperature sensor

      Serial.println(ledPins[JAMIEPIN]);
      Serial.println(BlinkState[JAMIEPIN]);
      Serial.println(JAMIEPIN);

      if (JAMIEPIN != jamieLastPinTouched) {
        if (JAMIEPIN == 0) {
          if (BlinkState[0]) {
            BlinkState[0] = false;
            digitalWrite(0, LOW);
            delay(1000);
          } else {
            BlinkState[0] = true;
            digitalWrite(0, HIGH);
            delay(1000);
          }
        }

        if (JAMIEPIN == 1) {
          if (BlinkState[1]) {
            BlinkState[1] = false;
            digitalWrite(1, LOW);
            delay(1000);
          } else {
            BlinkState[1] = true;
            digitalWrite(1, HIGH);
            delay(1000);
          }
        }

        if (JAMIEPIN == 2) {
          if (BlinkState[2]) {
            BlinkState[2] = false;
            digitalWrite(2, LOW);
            delay(1000);
          } else {
            BlinkState[2] = true;
            digitalWrite(2, HIGH);
            delay(1000);
          }
        }

        if (JAMIEPIN == 3) {
          if (BlinkState[3]) {
            BlinkState[3] = false;
            digitalWrite(3, LOW);
            delay(1000);
          } else {
            BlinkState[3] = true;
            digitalWrite(3, HIGH);
            delay(1000);
          }
        }

        jamieLastPinTouched = JAMIEPIN;
      }

      /**  if (BlinkState[JAMIEPIN] == true) {
           digitalWrite(ledPins[ACHALPIN], BlinkState[JAMIEPIN]);
           //delay(1000);
           Serial.println(ledPins[JAMIEPIN]);
           Serial.println(BlinkState[JAMIEPIN]);
         }
         else {
           digitalWrite(ledPins[ACHALPIN], BlinkState[JAMIEPIN]);
           //  delay(1000);
           Serial.println(ledPins[JAMIEPIN]);
           Serial.println(BlinkState[JAMIEPIN]);
         }
      **/
    }
  }
  inputClient->stop();
}
