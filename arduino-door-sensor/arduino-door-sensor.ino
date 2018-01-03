
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define DEBUG_SERIAL Serial

#define DOOR_SENSOR_INPUT_PIN 5

#define DOOR_IS_OPEN HIGH
#define DOOR_IS_CLOSED LOW

#define DOOR_IS_CLOSED_CODE 11
#define DOOR_IS_OPEN_CODE 22

#define CEPIN 9
#define CSPIN 10

RF24 radio(CEPIN, CSPIN);
byte addresses[][6] = { "1Door" };

void setupRadio()
{
    radio.begin();
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_HIGH);
    radio.openWritingPipe(addresses[0]);
    radio.stopListening();
}

void setup()
{
    DEBUG_SERIAL.begin(115200);
    DEBUG_SERIAL.println(F("Door sensor"));

    DEBUG_SERIAL.println(F("Pin setup"));
    pinMode(DOOR_SENSOR_INPUT_PIN, INPUT_PULLUP);

    setupRadio();
}

int getDoorStatus()
{
    return digitalRead(DOOR_SENSOR_INPUT_PIN);
}

void loop()
{
    int doorStatus = getDoorStatus();
    uint8_t doorStatusCode = DOOR_IS_OPEN_CODE;
    if (doorStatus == DOOR_IS_CLOSED)
    {
        doorStatusCode = DOOR_IS_CLOSED_CODE;
    }

    DEBUG_SERIAL.print(F("Door status code: "));
    DEBUG_SERIAL.print(doorStatusCode);
    if (!radio.write((uint8_t*)&doorStatusCode, sizeof(uint8_t)))
    {
        DEBUG_SERIAL.println(F("...no ack"));
    }
    else
    {
        DEBUG_SERIAL.println(F("...ack"));
    }

    delay(250);
}
