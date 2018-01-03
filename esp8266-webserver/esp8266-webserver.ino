// setup a programmer as in
// https://www.allaboutcircuits.com/projects/breadboard-and-program-an-esp-01-circuit-with-the-arduino-ide/

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
//AdafruitIO aio(AIO_USER, AIO_KEY);

void setup()
{
    ESP_SERIAL.begin(ESP_SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
    delay(10);

    pinMode(DOOR_STATUS_IN_PIN, INPUT_PULLUP);
    pinMode(DOOR_SIGNAL_OUT_PIN, OUTPUT);

    initializeSignalOutPin();

    // Connect to WiFi network
    ESP_SERIAL.println();
    ESP_SERIAL.println();
    ESP_SERIAL.print("Connecting to ");
    ESP_SERIAL.println(MYSSID);

    WiFi.begin(MYSSID, MYPASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        ESP_SERIAL.print(".");
    }

    ESP_SERIAL.println();
    ESP_SERIAL.println("WiFi connected");

    //aio.connect();
    //ESP_SERIAL.println(aio.statusText());

    // Start the server
    server.begin();
    ESP_SERIAL.println("Server started");

    // Print the IP address
    ESP_SERIAL.println(WiFi.localIP());
}

void postDoorStatus()
{
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
    clientWriteString(client, s);
}

int getMin(int a, int b)
{
    if (a < b)
    {
        return a;
    }

    return b;
}

void clientWriteString(WiFiClient& client, String s)
{
    while (s.length() > 2000)
    {
        int endIndex = getMin(s.length(), 2000);
        String ss = s.substring(0, endIndex);
        const char* buffer = ss.c_str();
        client.write(buffer, ss.length());
        s = s.substring(endIndex);
    }

    if (s.length() > 0)
    {
        const char* buffer = s.c_str();
        client.write(buffer, s.length());
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
    clientWriteString(client, s);
}

void get_style_css(WiFiClient& client)
{
    String s = FPSTR(http_status_200_OK);
    s += FPSTR(newLine);
    s += FPSTR(content_type_css);
    s += FPSTR(newLine);
    s += FPSTR(newLine);
    s += FPSTR(style_css_FileContent);    
    clientWriteString(client, s);
}

void get_favicon(WiFiClient& client)
{
    String s = FPSTR(http_status_404_NOTFOUND);
    clientWriteString(client, s);
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
    else if (req.indexOf(F("GET /api/v1/toggle")) >= 0)
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
        clientWriteString(client, s);
    }    
    else if (req.indexOf(F("GET /api/v1/status")) >= 0)
    {
        ESP_SERIAL.println("ESP_WAPI_STATUS");
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
        clientWriteString(client, s);
    }
    else
    {
        ESP_SERIAL.println(F("ESP_WAPI_UN"));
        String s = FPSTR(http_status_404_NOTFOUND);
        clientWriteString(client, s);
    }
}

void loop()
{
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
