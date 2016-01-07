#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <DHT.h>
#include <string.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* Definitions for Libraries used */
#define DHTTYPE DHT11
#define DHTPIN 6    // Onboard Temp/Humidity Sensor

#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

/* Define I2C controll Numbers */
#define RTC_CNTRL 104
#define EC_CONTRL 100

/* Definitions for Digital Pins */
#define GFCI_RELAY 8
#define PUMP1_RELAY 5
#define PUMP2_RELAY 9
#define PUMP_3_RELAY 7
#define PS_ON 2    // Onboard ATX "Power Supply On" wire.  High to power off the ATX, Low to turn it on.
                    // Arduino itself gets power from the ATX when not plugged into a USB port, which means,
                    // when it is not connected to a secondary power supply, setting PS_ON HIGH literally
                    // kills all the power to the board which will simultaneous kill to all AC circuits
                    // that are running on a relay. --An accidentally discovered safety measure for the
                    // testing environment; when software design is sufficiently robust it can be given a
                    // secondary power source to effectively remove the 'feature'.

/* Definitions for Analog Pins */
#define LIGHT_SENSOR A0
#define PH_SENSOR A1


/* Constants Used */
const long ONE_SECOND_INTERVAL = 15624;
