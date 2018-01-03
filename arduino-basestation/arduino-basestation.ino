#include <SoftwareSerial.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define DOOR_STATUS_OUTPUT_PIN 8
#define DOOR_SIGNAL_INPUT_PIN 6

#define DOOR_IS_OPEN HIGH
#define DOOR_IS_CLOSED LOW

#define RELAY_PIN 7

#define DEBUG_SERIAL Serial
#define ESP_SERIAL_PIN 2

#define ESP_RESET_PIN 4

#define MAX_DOOR_STATUS_WAIT_MILLIS 60000 // 60 seconds

#define DOOR_IS_CLOSED_CODE 11
#define DOOR_IS_OPEN_CODE 22

#define CEPIN 9
#define CSPIN 10

SoftwareSerial espSerial(ESP_SERIAL_PIN, 3);

int currentDoorStatus = DOOR_IS_CLOSED;
uint8_t currentDoorSignal = HIGH;

RF24 radio(CEPIN, CSPIN);
byte addresses[][6] = { "1Door" };

void setupRadio()
{
    radio.begin();
    radio.setPALevel(RF24_PA_MIN);
    radio.setAutoAck(1);
    radio.setRetries(15, 15);
    radio.openReadingPipe(0, addresses[0]);
    radio.startListening();
    // wait until initialized
    delay(1000);
}

void setup()
{
    DEBUG_SERIAL.begin(115200);
    DEBUG_SERIAL.println(F("BASESTATION"));
    espSerial.begin(115200);
    
    DEBUG_SERIAL.println(F("BASE_PIN_SETUP"));
    
    pinMode(13, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
    pinMode(DOOR_STATUS_OUTPUT_PIN, OUTPUT);
    pinMode(DOOR_SIGNAL_INPUT_PIN, INPUT_PULLUP);
    pinMode(ESP_RESET_PIN, OUTPUT);

    DEBUG_SERIAL.println(F("BASE_RADIO_SETUP"));
    setupRadio();

    DEBUG_SERIAL.println(F("BASE_INIT_DOOR_STATUS"));
    checkDoorStatus();

    resetESP();

    initializeCurrentDoorSignalIn();
    DEBUG_SERIAL.println(F("BASE_READY"));
}

void resetESP()
{
    DEBUG_SERIAL.println(F("BASE_RESET_ESP"));
    digitalWrite(ESP_RESET_PIN, LOW);
    delay(50);
    digitalWrite(ESP_RESET_PIN, HIGH);
    DEBUG_SERIAL.println(F("BASE_RESET_ESP_WAIT"));
    // wait for ESP to boot and connet
    delay(5000);
}

long prevDoorStatusCodeMillis = 0;

int getDoorStatus()
{
    long currentMillis = millis();
    if (radio.available())
    {
        uint8_t buffer;
        radio.read((uint8_t*)&buffer, sizeof(uint8_t));
        //DEBUG_SERIAL.print(F("Door status code "));
        //DEBUG_SERIAL.println(buffer);
        prevDoorStatusCodeMillis = currentMillis;
        if (buffer == DOOR_IS_CLOSED_CODE)
        {
            return DOOR_IS_CLOSED;
        }

        // if code doesn't say it's closed report open
        return DOOR_IS_OPEN;
    }
    else if ((currentMillis - prevDoorStatusCodeMillis) > MAX_DOOR_STATUS_WAIT_MILLIS)
    {
        //DEBUG_SERIAL.println(F("Door status is delayed"));
        // we have not received door status for a while, something is wrong
        return DOOR_IS_OPEN;
    }

    // no radio, return the current door status
    return currentDoorStatus;
}

int getDoorSignalIn()
{
    return digitalRead(DOOR_SIGNAL_INPUT_PIN);
}

void initializeCurrentDoorSignalIn()
{
    // always initilize with high
    currentDoorSignal = HIGH;
}

void checkDoorStatus()
{
    int s = getDoorStatus();
    if (currentDoorStatus != s)
    {        
        currentDoorStatus = s;
        if (currentDoorStatus == DOOR_IS_OPEN)
        {
            DEBUG_SERIAL.println(F("BASE_DOOR_IS_OPEN"));
            digitalWrite(DOOR_STATUS_OUTPUT_PIN, DOOR_IS_OPEN);
        }
        else
        {
            DEBUG_SERIAL.println(F("BASE_DOOR_IS_CLOSED"));
            digitalWrite(DOOR_STATUS_OUTPUT_PIN, DOOR_IS_CLOSED);
        }
    }
}

void toggleRelay()
{
    digitalWrite(RELAY_PIN, LOW);
    delay(125);
    digitalWrite(RELAY_PIN, HIGH);
    delay(125);
}

void checkDoorSignalIn()
{
    uint8_t signal = getDoorSignalIn();
    if (signal == currentDoorSignal)
    {
        return;
    }
    currentDoorSignal = signal;
    DEBUG_SERIAL.println(F("BASE_TOGGLE_RELAY"));
    toggleRelay();
}

void loop()
{
    if (espSerial.available())
    {
        String c = espSerial.readString();
        DEBUG_SERIAL.print(c);
    }

    checkDoorStatus();
    checkDoorSignalIn();
}
