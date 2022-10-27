/////////////////////////////////////////////////////////////////
/*                                                             //
  Created by Bermudez and Maciel. (MoDCS Research Group)       //
*/
//
/////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include <RadioLib.h>
#include "boards.h"
#define RADIOLIB_ERR_NONE (0)

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DI0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
TinyGPSPlus gps;

int transmissionState = RADIOLIB_ERR_NONE;
volatile bool transmittedFlag = false;
volatile bool enableInterrupt = true;
unsigned long gpsLastMillis;
StaticJsonDocument<512> doc;

void setFlag(void)
{
    if (!enableInterrupt)
    {
        return;
    }
    transmittedFlag = true;
}

static void smartDelay(unsigned long ms)
{
    unsigned long start = millis();
    do
    {
        while (Serial1.available())
            gps.encode(Serial1.read());
    } while (millis() - start < ms);
}

void fetchingGPS()
{
    doc.clear();
    doc["appid"] = "LoRa_GPS";
    doc["system"]["Name"] = "Transmitter";
    doc["system"]["RSSI"] = radio.getRSSI();
    doc["system"]["SNR"] = radio.getSNR();
    doc["system"]["Status"] = transmissionState;
    doc["system"]["Battery"]["Connection"] = PMU.isBatteryConnect();
    doc["system"]["Battery"]["Voltage"] = String((PMU.getBattVoltage() / 1000), 2);
    doc["location"]["lat"] = gps.location.lat();
    doc["location"]["lng"] = gps.location.lng();
    doc["satellites"] = gps.satellites.value();
    doc["altitude"] = String((gps.altitude.feet() / 3.2808), 2);
    char gpsDateTime[20];
    sprintf(gpsDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            gps.date.year(),
            gps.date.month(),
            gps.date.day(),
            gps.time.hour(),
            gps.time.minute(),
            gps.time.second());
    doc["gpsDateTime"] = gpsDateTime;

    String output;
    serializeJson(doc, output);
    Serial.println(output);
    transmissionState = radio.startTransmit(output);
    smartDelay(1000);
}

void setup()
{
    initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);

    // initialize SX1276 with default settings
    Serial.print(F("[SX1276] Initializing ... "));
    int state = radio.begin(LoRa_frequency);
    Serial.print(F("LoRa_frequency:"));
    Serial.println(LoRa_frequency);

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setDio0Action(setFlag);

    // start transmitting the first packet
    Serial.print(F("[SX1276] Sending first packet ... "));

    // you can transmit C-string or Arduino string up to
    // 256 characters long
    transmissionState = radio.transmit("***Sending First Packet!***");
}

void loop()
{
    // check if the previous transmission finished
    if (transmittedFlag)
    {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE)
        {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }

        delay(1000);
        fetchingGPS();

        enableInterrupt = true;
    }
}
