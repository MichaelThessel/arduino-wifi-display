#include "SoftwareSerial.h"
#include "ESP8266.h"
#include "LiquidCrystal.h"

#define SSID "MYSSID"
#define PASSWORD "MYPASSWORD"
#define TCPPORT 8090

SoftwareSerial mySerial(8, 9);
ESP8266 wifi(mySerial);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // RS, E, D4, D5, D6, D7

uint8_t boundaries[2];

/**
 * Print debug message
 */
void debugMessage(String message, int delayTime = 0, bool showOnDisplay = true)
{
    if (showOnDisplay) {
        lcd.clear();
        lcd.print(message);
    }

    Serial.println(message);

    delay(delayTime);
}

/**
 * Print startup debug message
 */
void debugStartupMessage(String message)
{
    debugMessage(message, 1000, true);
}

/**
 * Initialize the LCD
 */
void setupLcd()
{
    lcd.begin(16, 2);
}

/**
 * Initialize serial connection
 */
void setupSerial()
{
    Serial.begin(9600);
}

/**
 * Set up ESP8266 connection
 */
void setupWifi()
{
    debugStartupMessage("Welcome!!!");

    // Add a 5s delay to make sure the ESP is ready
    debugMessage("Waiting for ESP", 5000);

    if (wifi.setOprToStation()) {
        debugStartupMessage("OK set station");
    } else {
        debugStartupMessage("ERR set station");
    }

    if (wifi.joinAP(SSID, PASSWORD)) {
        debugStartupMessage("OK join AP");
    } else {
        debugStartupMessage("ERR join AP");
    }

    if (wifi.enableMUX()) {
        debugStartupMessage("OK set MUX");
    } else {
        debugStartupMessage("ERR set MUX");
    }

    if (wifi.startTCPServer(TCPPORT)) {
        debugStartupMessage("OK start TCPSrv");
    } else {
        debugStartupMessage("ERR start TCPSrv");
    }

    if (wifi.setTCPServerTimeout(10)) {
        debugStartupMessage("OK set Timeout");
    } else {
        debugStartupMessage("ERR set Timeout");
    }
}

/**
 * Parse header of server message
 *
 * Header:
 * boundaryLine1|boundaryLine2:data
 * i.e.
 * 10|13:THISIS10!!FOO
 * If boundary == 0 line won't be printed
 */
void parseHeader(uint8_t *buffer, uint32_t bufferLength)
{
    char c;
    uint8_t boundaryIndex = 0;
    uint8_t digitIndex = 1;
    uint8_t digit = 0;

    // Offset by header length
    boundaries[0] = 6;
    boundaries[1] = 6;

    for (uint32_t i = 0; i < bufferLength; i++) {
        c = (char)buffer[i];

        if (c == ':') { break; }

        if (c == '|') {
            boundaryIndex++;
            digitIndex = 1;
            continue;
        }

        digit = c - '0';
        boundaries[boundaryIndex] += digit * pow(10, digitIndex--);
    }
}

/**
 * Populate display
 */
void populateDisplay()
{
    // 38 == max line length (16) * 2 + header length (6)
    uint8_t buffer[38] = {0};
    uint8_t mux_id;

    char line1[17];
    char line2[17];
    char c;

    uint32_t bufferLength = wifi.recv(&mux_id, buffer, sizeof(buffer), 100);

    if (bufferLength > 0) {
        parseHeader(buffer, bufferLength);

        uint8_t j = 0, k = 0;
        for (uint32_t i = 6; i < bufferLength; i++) {
            c = (char)buffer[i];

            // Don't print newline chars
            if (c == '\n') { continue; }
            if (c == '\r') { continue; }

            if (boundaries[0] > 0 && i < boundaries[0]) {
                line1[j++] = c;
            } else if (boundaries[1] > 0) {
                line2[k++] = c;
            }
        }

        // NULL terminate
        line1[j] = '\0';
        line2[k] = '\0';

        // Write to display
        lcd.clear();
        if (boundaries[0] > 0) {
            lcd.setCursor(0, 0);
            lcd.print(line1);
        }
        if (boundaries[1] > 0) {
            lcd.setCursor(0, 1);
            lcd.print(line2);
        }

        wifi.releaseTCP(mux_id);
    }
}

/**
 * Setup
 */
void setup(void)
{
    setupLcd();
    setupSerial();
    setupWifi();

    lcd.clear();
}

/**
 * Loop
 */
void loop(void)
{
    populateDisplay();
}
