#include <SPI.h>
#include <LoRa.h>
#include <Arduino.h>
#include <Wire.h>
#include <stdio.h> 
#include <string.h>
#include<iostream>
#include <vector>

#define ss 5
#define rst 14
#define dio0 4
#define TELEMETRY_TRANSMIT_INTERVAL 5000 //1 amostra por minuto 60000
#define VCC_int 35
#define Digital1 26
#define Digital2 32
#define Digital3 25
#define Digital4 33
#define button 12
//GPIO26 D1
//GPIO32 D2
//GPIO25 D3
//GPIO33 D4
//GPIO12 botão
//GPIO35 BATERIA

#define SERIAL_DEBUG
using namespace std;

void SendData();
void onReceive(int packetSize);
void LoRaWrite();
void print_all();
void liga_valve();

uint8_t datasend[120];
bool DD1 = false,DD2 = false,DD3 = false,DD4 = false,BT = false;
int counter = 0;
float voltage = 0.00;
char node_id [30] = "80501601234810";
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
    pinMode(VCC_int, INPUT);
    pinMode(Digital1, OUTPUT);
    pinMode(Digital2, OUTPUT);
    pinMode(Digital3, OUTPUT);
    pinMode(Digital4, OUTPUT);
    pinMode(button, INPUT);
    digitalWrite(Digital1,LOW);
}

void loop()
{   
    //DD1 = digitalRead(Digital1);
    //DD2 = digitalRead(Digital2);
    //DD3 = digitalRead(Digital3);
    //DD4 = digitalRead(Digital4);
    //BT = digitalRead(button);
    //voltage = ((3.3*4096)/analogRead(VCC_int));
    delay(500);
    //LoRaWrite();
    //SendData();
    liga_valve();
    #ifdef SERIAL_DEBUG
    //print_all(); 
    #endif
    LoRa.receive();
}

void print_all()
{
    
    Serial.println("Digital 1/Digital 2/Digital 3/Digital 4/Botao RESET/VCC BAT :"); 
    Serial.print(DD1);
    Serial.print("\t");
    Serial.print(DD2);
    Serial.print("\t");
    Serial.print(DD3);
    Serial.print("\t");
    Serial.print(DD4);
    Serial.print("\t");
    Serial.print(BT); 
    Serial.print("\t");
    Serial.print(voltage); 
    Serial.println();
}
void LoRaWrite()
{
    char data[120];
    memset(data, 0, sizeof(data));
    snprintf(data, sizeof(data), "<%s>&field1=%d&field2=%d&field3=%d&field4=%d&field5=%d&field6=%.2f", node_id, DD1, DD2 , DD3, DD4, BT, voltage);
    strcpy((char*)datasend, data);
}
void SendData()
{
    if ((millis() - _lastTelemetryTransmitTime) > TELEMETRY_TRANSMIT_INTERVAL)
    {
        LoRa.beginPacket();
        LoRa.print((char *)datasend);
        LoRa.endPacket();
        Serial.println();
        Serial.print("Pacote enviado: ");
        Serial.println((char *)datasend);
        Serial.println();
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
        char liga [5] = "1";
        if(valuePair[1].equals(liga)==1)
        {
            counter++;
        }
        else
        counter = 0;
    }
}
void liga_valve()
{   
    if (counter == 0)
    {
       digitalWrite(Digital1,LOW); 
    }
    else
        digitalWrite(Digital1,HIGH);
        //delay(5000);
        //digitalWrite(Digital1,LOW);
        //delay(5000);
        //counter = 0;
}
