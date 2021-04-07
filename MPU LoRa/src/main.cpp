#include <Arduino.h>
#include<Wire.h>
#include <LoRa.h>
#include <SPI.h>
#include <stdio.h> 
#include <string.h>
#include<iostream>
#include <vector> 
//Endereco I2C do MPU6050
const int MPU=0x68;  
//Variaveis para armazenar valores dos sensores
int AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
#define ss 5
#define rst 14
#define dio0 4
#define TELEMETRY_TRANSMIT_INTERVAL 1000 //1 amostra por minuto 60000
#define VCC_int 35
using namespace std;

void SendData();
void onReceive(int packetSize);
void LoRaWrite();
void print_all();

uint8_t datasend[120];
float voltage = 0.00;
char node_id [30] = "80501601234710";
time_t _lastTelemetryTransmitTime = 0;

void setup()
{
  Serial.begin(9600);

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
    pinMode(VCC_int, INPUT);
    Wire.begin(21,22);
    Wire.beginTransmission(MPU);
    Wire.write(0x6B); 
   
  //Inicializa o MPU-6050
  Wire.write(0); 
  Wire.endTransmission(true);
}
void loop()
{
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  //Solicita os dados do sensor
  Wire.requestFrom(MPU,14,true);  
  //Armazena o valor dos sensores nas variaveis correspondentes
  AcX=Wire.read()<<8|Wire.read();  //0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY=Wire.read()<<8|Wire.read();  //0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  //0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  //0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  //0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  //0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
   
  //Envia valor X do acelerometro para a serial e o LCD
  Serial.print("AcX = "); Serial.print(AcX);
   
  //Envia valor Y do acelerometro para a serial e o LCD
  Serial.print(" | AcY = "); Serial.print(AcY);
   
  //Envia valor Z do acelerometro para a serial e o LCD
  Serial.print(" | AcZ = "); Serial.print(AcZ);
   
  //Envia valor da temperatura para a serial e o LCD
  //Calcula a temperatura em graus Celsius
  Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);
   
  //Envia valor X do giroscopio para a serial e o LCD
  Serial.print(" | GyX = "); Serial.print(GyX);
   
  //Envia valor Y do giroscopio para a serial e o LCD  
  Serial.print(" | GyY = "); Serial.print(GyY);

  //Envia valor Z do giroscopio para a serial e o LCD
  Serial.print(" | GyZ = "); Serial.println(GyZ);
   
  //Aguarda 300 ms e reinicia o processo
  delay(300);

  voltage = ((3.3*4096)/analogRead(VCC_int));
    //delay(2000);
    LoRaWrite();
    SendData();
    LoRa.receive();
}

void LoRaWrite()
{
    char data[120];
    memset(data, 0, sizeof(data));
    snprintf(data, sizeof(data), "<%s>&field1=%d&field2=%d&field3=%d&field4=%d&field5=%d&field6=%d", node_id, AcX, AcY , AcZ, GyX, GyY, GyZ);
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
    }
}
