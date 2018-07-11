// setup a programmer as in
// https://www.allaboutcircuits.com/projects/breadboard-and-program-an-esp-01-circuit-with-the-arduino-ide/

#include <ArduinoHttpClient.h>
#include <ESP8266WiFi.h>
// define MYSSID and MYPASSWORD in this file. Do not commit to repo
#include "myssid.h"
// define AIO_FEED and AIO_KEY in this file. Do not commit to repo
#include "aio.h"

// must disable visual micro deep search for this to work
const char client_js_FileContent[] PROGMEM =
#include "..\..\arduino-esp8266-nrf24-door\esp8266-webserver\public\client.js"
;

const char index_html_FileContent[] PROGMEM =
#include "..\..\arduino-esp8266-nrf24-door\esp8266-webserver\public\index.html"
;

const char style_css_FileContent[] PROGMEM =
#include "..\..\arduino-esp8266-nrf24-door\esp8266-webserver\public\style.css"
;

const char newLine[] PROGMEM = "\r\n";
const char http_status_200_OK[] PROGMEM = "HTTP/1.1 200 OK";
const char http_status_404_NOTFOUND[] PROGMEM = "HTTP/1.1 404 NOT FOUND";

const char content_type_js[] PROGMEM = "Content-Type: application/javascript";
const char content_type_css[] PROGMEM = "Content-Type: text/css";
const char content_type_html[] PROGMEM = "Content-Type: text/html";
const char content_type_json[] PROGMEM = "Content-Type: application/json";
const char content_type_plain[] PROGMEM = "Content-Type: text/plain";
const char content_length_zero[] PROGMEM = "Content-Length: 0";

#define ESP_SERIAL Serial
#define ESP_SERIAL_BAUD_RATE 115200

#define DOOR_IS_OPEN HIGH
#define DOOR_IS_CLOSED LOW

// RX pin is input for current door status. HIGH = open, LOW = closed
#define DOOR_STATUS_IN_PIN 3
// GPIO2 is toggled when door should be opened or closed
#define DOOR_SIGNAL_OUT_PIN 2

// current signal state is initialized at boot
uint8_t currentSignalOut = HIGH;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
WiFiClient client;

#define AIO_SERVER "io.adafruit.com"
#define AIO_PORT 80
#define AIO_CONTENT_TYPE "Content-Type"
#define AIO_CONTENT_TYPE_APP_JSON "application/json"
#define AIO_POST_DOOR_WAIT_MILLIS 60000
#define AIO_CONTENT_LENGTH "Content-Length"
#define AIO_CONTENT_LENGTH_VALUE 16
#define AIO_DOOR_STATUS_HIGH "{ \"value\": \"1\" }"
#define AIO_DOOR_STATUS_LOW "{ \"value\": \"0\" }"

HttpClient httpClient(client, AIO_SERVER, AIO_PORT);

void setup()
{
    ESP_SERIAL.begin(ESP_SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
    delay(10);

    pinMode(DOOR_STATUS_IN_PIN, INPUT_PULLUP);
    pinMode(DOOR_SIGNAL_OUT_PIN, OUTPUT);

    initializeSignalOutPin();

    startWifi();
    startServer();
}

void startWifi()
{
    // Connect to WiFi network
    ESP_SERIAL.println();
    ESP_SERIAL.println();
    ESP_SERIAL.print(F("SSID "));
    ESP_SERIAL.println(MYSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(MYSSID, MYPASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        ESP_SERIAL.print(F("."));
    }

    ESP_SERIAL.println();
    ESP_SERIAL.println(F("ESP_WIFI_CONNECTED"));
    ESP_SERIAL.println(WiFi.localIP());

}

void startServer()
{
    server.begin();
    ESP_SERIAL.println(F("ESP_SERVER_STARTED"));
}

long prevPost = 0;
long openPostCount = 0;
char postBody[256];
void postDoorStatus()
{
    long m = millis();
    long diff = m - prevPost;
    if (diff < AIO_POST_DOOR_WAIT_MILLIS)
    {
        return;
    }
    prevPost = m;

    ESP_SERIAL.println(F("ESP_POST_DOOR_AIO"));
    int status = getDoorStatus();
    if (status == HIGH)
    {
        openPostCount++;
    }
    else
    {
        openPostCount = 0;
    }

    sprintf(postBody, "{ \"value\": \"%i\" }\0", openPostCount);

    httpClient.beginRequest();
    httpClient.post(AIO_DATA_URL);
    // defined in aio.h file
    httpClient.sendHeader(AIO_KEY_HEADER, AIO_KEY);
    httpClient.sendHeader(AIO_CONTENT_TYPE, AIO_CONTENT_TYPE_APP_JSON);
    httpClient.sendHeader(AIO_CONTENT_LENGTH, strlen(postBody));
    httpClient.beginBody();
    httpClient.print(postBody);
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    ESP_SERIAL.print(F("ESP_POST_DOOR_STATUS "));
    ESP_SERIAL.println(statusCode);
}

int getDoorStatus()
{
    return digitalRead(DOOR_STATUS_IN_PIN);
}

void initializeSignalOutPin()
{
    // always start with HIGH
    digitalWrite(DOOR_SIGNAL_OUT_PIN, HIGH);
}

void toggleDoorSignalOut()
{
    ESP_SERIAL.println(F("ESP_SIG_TOGGLE"));
    if (currentSignalOut == HIGH)
    {
        currentSignalOut = LOW;
    }
    else
    {
        currentSignalOut = HIGH;
    }
    digitalWrite(DOOR_SIGNAL_OUT_PIN, currentSignalOut);
}

void get_index_html(WiFiClient& client)
{
    String s = FPSTR(http_status_200_OK);
    s += FPSTR(newLine);
    s += FPSTR(content_type_html);
    s += FPSTR(newLine);
    s += FPSTR(newLine);
    s += FPSTR(index_html_FileContent);
    const char* buffer = s.c_str();
    clientWriteString(client, buffer);
}

int getMin(int a, int b)
{
    if (a < b)
    {
        return a;
    }

    return b;
}

void clientWriteString(WiFiClient& client, const char* buffer)
{
    int packetSize = 2000;
    int totLength = strlen(buffer);
    int index = 0;
    int remain = totLength - index;
    int sendLength = getMin(remain, packetSize);
    while (index < totLength)
    {
        client.write(&buffer[index], sendLength);
        index += packetSize;
        remain = totLength - index;
        sendLength = getMin(remain, packetSize);
    }

    if (remain > 0)
    {
        client.write(&buffer[index], remain);
    }
}

void get_client_js(WiFiClient& client)
{
    String s = FPSTR(http_status_200_OK);
    s += FPSTR(newLine);
    s += FPSTR(content_type_js);
    s += FPSTR(newLine);
    s += FPSTR(newLine);
    s += FPSTR(client_js_FileContent);
    const char* buffer = s.c_str();
    clientWriteString(client, buffer);
}

void get_style_css(WiFiClient& client)
{
    String s = FPSTR(http_status_200_OK);
    s += FPSTR(newLine);
    s += FPSTR(content_type_css);
    s += FPSTR(newLine);
    s += FPSTR(newLine);
    s += FPSTR(style_css_FileContent);    
    const char* buffer = s.c_str();
    clientWriteString(client, buffer);
}

void get_favicon(WiFiClient& client)
{
    String s = FPSTR(http_status_404_NOTFOUND);
    const char* buffer = s.c_str();
    clientWriteString(client, buffer);
};


void printClientRequest(String req)
{
    ESP_SERIAL.print(F("["));
    ESP_SERIAL.print(req);
    ESP_SERIAL.print(F("]"));
    ESP_SERIAL.println();
}

void handleClient(WiFiClient& client)
{
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.read(); // \r
    client.read(); // \n

    if (req.indexOf(F("GET / ")) >= 0) // space after / to make sure it's root
    {
        get_index_html(client);
    }
    else if (req.indexOf(F("GET /client.js")) >= 0)
    {
        get_client_js(client);
    }
    else if (req.indexOf(F("GET /style.css")) >= 0)
    {
        get_style_css(client);
    }
    else if (req.indexOf(F("GET /favicon.ico")) >= 0)
    {
        get_favicon(client);
    }
    else if (req.indexOf(F("POST /api/v1/toggle")) >= 0)
    {
        ESP_SERIAL.println(F("ESP_WAPI_TG"));

        toggleDoorSignalOut();

        // status
        String s = FPSTR(http_status_200_OK);
        s += FPSTR(newLine);
        // header
        s += FPSTR(content_type_plain);
        s += FPSTR(newLine);
        // blank line indicates data
        s += FPSTR(newLine);
        s += FPSTR(F("Ok"));
        const char* buffer = s.c_str();
        clientWriteString(client, buffer);
    }    
    else if (req.indexOf(F("GET /api/v1/status")) >= 0)
    {
        //ESP_SERIAL.println(F("ESP_WAPI_STATUS"));
        int result = getDoorStatus();
        // status
        String s = FPSTR(http_status_200_OK);
        s += FPSTR(newLine);
        // header
        s += FPSTR(content_type_plain);
        s += FPSTR(newLine);
        // blank line indicates data
        s += FPSTR(newLine);
        if (result == DOOR_IS_OPEN)
        {
            s += F("IsOpen");
        }
        else
        {
            s += F("IsClosed");
        }
        const char* buffer = s.c_str();
        clientWriteString(client, buffer);
    }
    else
    {
        ESP_SERIAL.println(F("ESP_WAPI_UN"));
        String s = FPSTR(http_status_404_NOTFOUND);
        const char* buffer = s.c_str();
        clientWriteString(client, buffer);
    }
}

void loop()
{
    postDoorStatus();

    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client)
    {
        return;
    }

    // Wait until the client sends some data
    while (!client.available())
    {
        delay(1);
    }

    handleClient(client);
    client.stop();
}

