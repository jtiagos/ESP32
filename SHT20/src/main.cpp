#include <SPI.h>
#include <LoRa.h>
#include <Arduino.h>
#include <Wire.h>
#include <stdio.h> 
#include <string.h>
#include<iostream>
#include <vector>
#include "uFire_SHT20.h"

#define ss 5
#define rst 14
#define dio0 4
#define TELEMETRY_TRANSMIT_INTERVAL 60000 //1 amostra por minuto
#define VCC_int 35
//#define SERIAL_DEBUG
using namespace std;

uFire_SHT20 sht20;

void SendData();
void onReceive(int packetSize);
void sht_all();
void shtWrite();
void print_all();

uint8_t datasend[120];
float tempC = 0.00;
float dew_pointC = 0.00;
float RH = 0.00;
float voltage = 0.00;
char node_id [30] = "80501601234510";
time_t _lastTelemetryTransmitTime = 0;

void setup()
{
    #ifdef SERIAL_DEBUG
    
        Serial.begin(9600);
        while (!Serial);
        Serial.println(F("Start exemplo MQTT"));
    
    #endif

    LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(915000000))
    { 
        Serial.println("Starting LoRa falhou!");
        while (1);
    }
    LoRa.setSpreadingFactor(7);// Setup Spreading Factor (6 ~ 12)
    // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
    LoRa.setSignalBandwidth(250000);//Lower BandWidth for longer distance.
    LoRa.setCodingRate4(5);// Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
    LoRa.setSyncWord(0x34); //Endereçamento da rede LoRa
    Serial.println("LoRa iniciado");
    LoRa.onReceive(onReceive);   
    LoRa.receive(); 
    Wire.begin();
    sht20.begin(22,21);
    pinMode(VCC_int, INPUT);
}

void loop()
{   
    sht_all();
    shtWrite();
    voltage = ((3.3*4096)/analogRead(VCC_int));
    SendData();
    #ifdef SERIAL_DEBUG
    print_all(); 
    #endif
    LoRa.receive();
}

void sht_all()
{
    sht20.measure_all();
    tempC = sht20.tempC;
    dew_pointC = sht20.dew_pointC;
    RH = sht20.RH;
}
void print_all()
{
    
    Serial.println("Temperatura C/Ponto de orvalho C/Umidade %/VCC BAT :"); 
    Serial.print(sht20.tempC);
    Serial.print("\t"); 
    Serial.print(sht20.dew_pointC);
    Serial.print("\t"); 
    Serial.print(sht20.RH);
    Serial.print("\t"); 
    Serial.print(voltage);
    Serial.println(); 
    //Serial.print((String)sht20.RH + " %RH");
}
void shtWrite()
{
    char data[120];
    memset(data, 0, sizeof(data));
    snprintf(data, sizeof(data), "<%s>&field1=%.2f&field2=%.2f&field3=%.2f&field4=%.2f", node_id, tempC, dew_pointC , RH, voltage);
    strcpy((char*)datasend, data);
}
void SendData()
{
    if ((millis() - _lastTelemetryTransmitTime) > TELEMETRY_TRANSMIT_INTERVAL)
    {
        LoRa.beginPacket();
        LoRa.print((char *)datasend);
        LoRa.endPacket();
        Serial.print("Pacote enviado: ");
        Serial.println((char *)datasend);
        _lastTelemetryTransmitTime = millis();
    }
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
    //Serial.print(name);
    //Serial.print("\n");
       
    char * pch;
    pch = strtok (name,"<>&");
    while (pch != NULL)
    {
        //Serial.printf("%s\n",pch);
        valuePair.push_back(pch);
        pch = strtok (NULL, "<>&");
    }
    //Serial.print(valuePair[0]);
    //Serial.println();
    if(valuePair[0]!=node_id)
    {
        Serial.println("Ignorar pacote, destino invalido");
    }
    else
    {
        Serial.print("Pacote recebido: ");
        Serial.println(LoRaData.c_str()); 
        Serial.println(valuePair[1]); 
    }
}
