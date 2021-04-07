#include <SPI.h>
#include <LoRa.h>
#include <Arduino.h>
#include <Wire.h>
#include <stdio.h> 
#include <string.h>
#include<iostream>
#include <vector>
#include <TinyGPS++.h>
using namespace std;
#define ss 5
#define rst 14
#define dio0 4
#define GPS_BEGIN //caso deseje usar o GPS - Descomentar
//#define LORA_BEGIN
// The TinyGPS++ object
TinyGPSPlus gps;

void GpsWrite();
void SendData();
void onReceive(int packetSize);
void GpsAvailable();
void displayInfo();
float Lat = -00.00000000;                                                                                                                                                                                                                                                                                                                                                                                                                                                     
float Long = -00.00000000;
char node_id [30]= "80501601234510";
char node_id2 [30]= "80501601234610";
uint8_t datasend[36];
static const uint32_t GPSBaud = 9600;
float counter = 0.000000;

void setup()
{
    
    Serial.begin(9600);
    while (!Serial);
    Serial.println(F("Start exemplo MQTT"));
    #if defined(LORA_BEGIN)
    LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(915000000))
    { 
        Serial.println("Starting LoRa falhou!");
        while (1);
    }
    
    // Setup Spreading Factor (6 ~ 12)
    LoRa.setSpreadingFactor(7);
    // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
    //Lower BandWidth for longer distance.
    LoRa.setSignalBandwidth(250000);
    // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
    LoRa.setCodingRate4(5);
    LoRa.setSyncWord(0x34); //Endereçamento da rede LoRa
    Serial.println("LoRa iniciado");
    LoRa.onReceive(onReceive);   
    LoRa.receive(); 
    #endif
    pinMode(26, OUTPUT);
    digitalWrite(26, LOW);
    

    #if defined(GPS_BEGIN)
    Serial2.begin(GPSBaud);
    Serial.println("Inicializando GPS");
    while (!Serial2)
    {
        Serial.println("Falha serial, verifique conexão");
    }
    Serial.println("GPS ok");
    #endif
}

void loop()
{   
    #if defined(GPS_BEGIN)
    GpsAvailable();
    #endif
    #if defined(LORA_BEGIN)
    GpsWrite();
    SendData(); 
    LoRa.receive();
    #endif
}

void displayInfo()
{
    Serial.print(F("Location: ")); 
    if (gps.location.isValid())
    {
        Lat = (gps.location.lat(), 6);
        Long = (gps.location.lng(), 6);
        Serial.print(gps.location.lat(), 6);
        Serial.print(F(","));
        Serial.print(gps.location.lng(), 6);
    }
    else
    {
        Lat = -25.43211807; //coordenadas fake apenas para debug
        Long = -49.24725384; //coordenadas fake apenas para debug
        Serial.print(F("Sem sinal"));
    }

    Serial.print(F("  Date/Time: "));
    if (gps.date.isValid())
    {
        Serial.print(gps.date.month());
        Serial.print(F("/"));
        Serial.print(gps.date.day());
        Serial.print(F("/"));
        Serial.print(gps.date.year());
    }
    else
    {
        Serial.print(F("INVALID"));
    }

    Serial.print(F(" "));
    if (gps.time.isValid())
    {
        if (gps.time.hour() < 10) Serial.print(F("0"));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        if (gps.time.minute() < 10) Serial.print(F("0"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        if (gps.time.second() < 10) Serial.print(F("0"));
        Serial.print(gps.time.second());
        Serial.print(F("."));
        if (gps.time.centisecond() < 10) Serial.print(F("0"));
        Serial.print(gps.time.centisecond());
    }
    else
    {
        Serial.print(F("INVALID"));
    }
    Serial.println();
}

void GpsAvailable()
{
    while (Serial2.available() > 0)
    if (gps.encode(Serial2.read()))
    displayInfo();

    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
        Serial.println(F("GPS não detectado: check ligacoes"));
        while(true);
    }
}

void GpsWrite()
{
    char data[70];//50
    memset(data, 0, sizeof(data));
    snprintf(data, sizeof(data), "<%s>&field1=%.6f&field2=%.6f", node_id, counter, Long);
    strcpy((char*)datasend, data);
}

void SendData()
{
    delay(5000);
    LoRa.beginPacket();
    LoRa.print((char *)datasend);
    LoRa.endPacket();
    counter++;
    Serial.print("Pacote enviado: ");
    Serial.println((char *)datasend);
}    

void onReceive(int packetSize) 
{
    // read packet
    String LoRaData = "";
    while (LoRa.available()) 
    {
        LoRaData += (char)LoRa.read();
    }
    vector<String> valuePair;
    char name[1024];
    strcpy(name, LoRaData.c_str());
    Serial.print(name);
    Serial.print("\n");
       
    char * pch;
    pch = strtok (name,"<>");
    while (pch != NULL)
    {
        //Serial.printf("%s\n",pch);
        valuePair.push_back(pch);
        pch = strtok (NULL, "<>");
    }
    Serial.println(valuePair[0]);
    
    if(valuePair[0]!=node_id2)
    {
        Serial.println("Ignorar pacote, destino invalido");
    }
    else
    {
        Serial.print("Pacote recebido: ");
        Serial.println(LoRaData.c_str()); 
        //Serial.println(valuePair[1]);
        char comparador[20] = "abrir";
        if(valuePair[1] == comparador) 
        {
            Serial.println("Abrindo portom");
            digitalWrite(26, HIGH);   // turn the LED on (HIGH is the voltage level)
            delay(3000);                       // wait for a second
            Serial.println("Fechando portom");
        }
        else
        {
            Serial.println("Ignorar pacote");
            Serial.println(valuePair[1]);
        }
    }
}