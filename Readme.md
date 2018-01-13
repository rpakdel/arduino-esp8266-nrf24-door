# Wireless Garage Door Sensor and Opener

## Hardware
* Base-station with relay: https://imgur.com/5AiFq67
* Wireless door sensor: https://media1.giphy.com/media/3o752bfTEIFtwjCC2I/giphy.gif
* The door sensor uses a generic window reed switch used commonly for home security (not operational yet).
* The door sensor is a separate Arduino that talks to the base-station via NRF24L01.
* RX pin on ESP8266 (ESP-01) is input for door status. HIGH = open, LOW = closed.
* GPIO2 on ESP8266 (ESP-01) is output for relay toggle signal. A change in voltage indicates toggle.
* I found that shorting the two connections on the garage door opener toggles the door.
* A simple relay (Normally Open) is used to achieve the same from Arduino.

## Software
* The web page and web API are hosted on the ESP8266 including the HTML, client side JavaScript and CSS. 
* The ESP8266 and Arduino communicate via The RX and GPIO2 pins on the ESP-01.
* I'm using Basic HTTP auth + SSL to reduce risk of somebody getting a hold of the door.
* The door status is uploaded to Adafruit.IO once per minute by the ESP8266.
* This IFTT applet then sends me notifications if the door has been left open: 
  https://ifttt.com/applets/317604p-if-the-door-is-opened-send-me-a-notification
  
## High Level Data Flow

### Web -> Relay
web toggle button -> nginx -> esp8266-webserver -> Arduino (1) -> relay to short the garage door button

### Door status -> Web 
Door sensor -> Arduino (2) -> NRF24L01 -> Arduino (1) -> esp8266-webserver -> nginx -> web door status
