#include <Arduino.h>
#include <Wire.h>

#define TERMINATOR 0x0

void realloc_buffer();
void on_data_received(int count);
void on_data_requested();
void serial_write_data(uint8_t count);
void on_data_received_terminator(int count);
void serial_write_data_terminator(uint8_t terminator);
void on_data_requested_terminator();

uint8_t* buffer = NULL;

void setup() 
{
  buffer = (uint8_t*)malloc(0xFF);

  Serial.begin(9600);
  Wire.begin(0x0F);

  Wire.onReceive(on_data_received_terminator);
  Wire.onRequest(on_data_requested_terminator);

  pinMode(13, OUTPUT);
  Serial.println("test");
}

void loop() 
{
  
}

void on_data_received(int count)
{
  Wire.readBytes(buffer, count);

  serial_write_data((uint8_t)count);

  digitalWrite(13, 1);
}

void on_data_requested()
{
  Serial.println("Data requested");

  Wire.write("Hello world!", 12);

  digitalWrite(13, 1);
}

void on_data_received_terminator(int count)
{
  Wire.readBytesUntil(TERMINATOR, buffer, count);

  serial_write_data_terminator(TERMINATOR);

  digitalWrite(13, 1);
}

void on_data_requested_terminator()
{
  Serial.println("Data requested");
  
  //char* tx_buffer = (char*)malloc(14);
  //strcpy(tx_buffer, "Hello World!");
  //tx_buffer[13] = TERMINATOR;
  
  Wire.write("Hello world!", 13);

  digitalWrite(13, 1);
}

void serial_write_data(uint8_t count)
{
  Serial.println("#: T | BBBBBBBB | HH | DEC");

  for (uint8_t i = 0; i < count; i++)
  {
    Serial.print(i, DEC);
    Serial.print(": ");
    Serial.print((char)buffer[i]);
    Serial.print(" | ");
    Serial.print(buffer[i], BIN);
    Serial.print(" | ");
    Serial.print(buffer[i], HEX);
    Serial.print(" | ");
    Serial.println(buffer[i], DEC);
  }

  Serial.println("--------------------------");
}

void serial_write_data_terminator(uint8_t terminator)
{
  Serial.println("#: T | BBBBBBBB | HH | DEC");

  for (uint8_t i = 0; true; i++)
  {
    Serial.print(i, DEC);
    Serial.print(": ");
    Serial.print((char)buffer[i]);
    Serial.print(" | ");
    Serial.print(buffer[i], BIN);
    Serial.print(" | ");
    Serial.print(buffer[i], HEX);
    Serial.print(" | ");
    Serial.println(buffer[i], DEC);

    if (buffer[i] == TERMINATOR) break;
  }

  Serial.println("--------------------------");
}