#include <rom/rtc.h>
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <stdio.h> 
#include <string.h>
#include<iostream>
#include <vector>
#define SerialMon Serial
#define SerialAT Serial1
#define GPS_BEGIN //caso deseje usar o GPS - Descomentar
#define GSM_TX 13
#define GSM_RX 14

const char apn[] = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";
const char *deviceId = "GsmClient";

const char *broker = "mqtt.thingspeak.com";
const int port = 1883;
const char *mqttUser = "user_user";
const char *mqttPassword = "pass_pass";
long channelID = 1310747;
char writeAPIKey[] = "API_API";
const char *topicLed = "&field1=";
const char *topicInit = "&field2=";
const char *topicLedStatus = "&field3=";
const unsigned long postingInterval = 15L * 1000L;
unsigned long lastConnectionTime = 0; 

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
using namespace std;
typedef struct
{
    
    float LATITUDE, LONGITUDE;
} GPS;
TinyGPSPlus gps;
GPS mGPS;
static const uint32_t GPSBaud = 9600;

#define LED_PIN 27
int ledStatus = LOW;
int count = 0;
long lastReconnectAttempt = 0;

void mqttPublishFeed();
void GpsAvailable();
void displayInfo();
boolean mqttConnect();

void setup()
{
    #if defined(GPS_BEGIN)
    Serial2.begin(GPSBaud);
    Serial.println("Inicializando GPS");
    while (!Serial2)
    {
        SerialMon.println("Falha serial, verifique conexão");
    }
    SerialMon.println("GPS ok");
    #endif
    SerialMon.begin(9600);
    SerialMon.println("Iniciando GSM/GPIO12...");
    pinMode(12,OUTPUT);
    digitalWrite(12,HIGH);
    delay(1000);
    digitalWrite(12,LOW);
    delay(500);
    // Set GSM module baud rate
    SerialAT.begin(9600, SERIAL_8N1, GSM_TX, GSM_RX, false);
    delay(3000);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Inicializando o modem...");
    modem.restart();

    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem: ");
    SerialMon.println(modemInfo);

    // Unlock your SIM card with a PIN
    //modem.simUnlock("1234");

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork())
    {
        SerialMon.println(" fail");
        while (true);
    }
    SerialMon.println(" OK");

    while (true)
    {
        SerialMon.printf("Trying to connect to %s...\n", apn);
        if (!modem.gprsConnect(apn, user, pass))
        {
            SerialMon.println(" fail");
            SerialMon.println("Trying again in 1s...");
            delay(1000);
        }
        else
        {
            SerialMon.println(" OK");
            break;
        }
    }

    // MQTT Broker setup
    mqtt.setServer(broker, port);
    void mqttPublishFeed();
}

void loop()
{
    GpsAvailable();
    if (!mqtt.connected())
    {
        SerialMon.println("=== MQTT NAO CONECTADO ===");
        // Reconnect every 10 seconds
        unsigned long t = millis();
        if (t - lastReconnectAttempt > 10000L)
        {
            lastReconnectAttempt = t;
            if (mqttConnect())
            {
                lastReconnectAttempt = 0;
                SerialMon.println("=== MQTT CONECTADO ===");
            }
        }
        delay(100);
        return;
    }
    mqtt.loop();
    if (millis() - lastConnectionTime > postingInterval) 
    {
        mqttPublishFeed();
        //count+=1;
     }
    SerialMon.printf("Status GPRS: %s\n", modem.isGprsConnected() ? "connected" : "not connected");
    SerialMon.printf("Qualidade do sinal: %d\n", modem.getSignalQuality());
    SerialMon.printf("Nivel da bateria: %d\n", modem.getBattPercent());
    float voltage = modem.getBattVoltage();
    SerialMon.printf("Tensao da bateria: %.2f\n", voltage > 32 ? -1 : voltage );
    String gsmTime = modem.getGSMDateTime(DATE_TIME);
    SerialMon.printf("GSM Hora: %s\n", gsmTime.c_str());
    String gsmDate = modem.getGSMDateTime(DATE_DATE);
    SerialMon.printf("GSM Data: %s\n", gsmDate.c_str());
    String cop = modem.getOperator();
    SerialMon.printf("Operadora: %s\n", cop.c_str());
    // String band = modem.getBand();
    // SerialMon.printf("Banda: %s\n", band.c_str());
}
void GpsAvailable()
{
    while (Serial2.available() > 0)
    if (gps.encode(Serial2.read()))
    displayInfo();

    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
        SerialMon.println(F("GPS não detectado: check ligacoes"));
        //while(true);
    }
}
void displayInfo()
{
    SerialMon.print(F("Location: ")); 
    if (gps.location.isValid())
    {
        mGPS.LATITUDE=(gps.location.lat());
        SerialMon.print(mGPS.LATITUDE,8);
        SerialMon.print(",");  
        mGPS.LONGITUDE=(gps.location.lng());
         SerialMon.println(mGPS.LONGITUDE,8); 
        //delay(1000); 
    }
    else
    {
        mGPS.LATITUDE = 00.00000000; //coordenadas fake apenas para debug
        mGPS.LONGITUDE = 00.00000000; //coordenadas fake apenas para debug
        SerialMon.println("Sem sinal");
        //delay(5000); 
    }


}   
boolean mqttConnect()
{
    SerialMon.print("Connecting to ");
    SerialMon.print(broker);

    // Connect to MQTT Broker
    //boolean status = mqtt.connect("GsmClientTest");

    // Or, if you want to authenticate MQTT:
    boolean status = mqtt.connect(deviceId, mqttUser, mqttPassword);

    if (status == false)
    {
        SerialMon.println(" fail");
        return false;
    }

    SerialMon.println(" OK");
    mqtt.publish(topicInit, "GSMClient iniciado");
    mqtt.subscribe(topicLed);
    return mqtt.connected();
}

void mqttPublishFeed() {
  
  //float t = 25; // Read temperature from DHT sensor.
  //float h = 95;  // Read humidity from DHT sensor.
  //int lightLevel = 105; // Read voltage from light sensor.
  
  // Create data string to send to ThingSpeak.
  String data = String("field1=") + String(mGPS.LATITUDE, DEC) + "&field2=" + String(mGPS.LONGITUDE, DEC)/* + "&field3=" + String(count, DEC)*/;
  int length = data.length();
  const char *msgBuffer;
  msgBuffer=data.c_str();
  SerialMon.println(msgBuffer);
  
  // Create a topic string and publish data to ThingSpeak channel feed. 
  String topicString = "channels/" + String( channelID ) + "/publish/"+String(writeAPIKey);
  length = topicString.length();
  const char *topicBuffer;
  topicBuffer = topicString.c_str();
  mqtt.publish( topicBuffer, msgBuffer );
  lastConnectionTime = millis();
}

