#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <DHT.h>
#include <string.h>
#include <Wire.h>
#include <OneWire.h>

/* Definitions for Libraries used */
#define DHTTYPE DHT11
#define DHTPIN 6    // Onboard Temp/Humidity Sensor

#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS);

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

// Timer Variables
volatile int seconds;
volatile int minutes;
volatile int hours;
volatile boolean IS_DAYTIME;

  // Relay Specific Timer Variables
    volatile int gfci_time_remaining;
    const int GFCI_TIMER_INTERVAL = 240;  // in minutes == 4 hours.

    volatile int pump1_time_remaining;
    const int PUMP1_TIMER_INTERVAL = 1; // seconds.

    volatile int pump2_time_remaining;
    const int PUMP2_TIMER_INTERVAL = 1;  // seconds.

    volatile int pump3_time_remaining;
    const int PUMP3_TIMER_INTERVAL = 3;  // seconds.


// Relay State Variables
volatile boolean GFCI_ON;
volatile boolean PUMP1_ON;
volatile boolean PUMP2_ON;
volatile boolean PUMP3_ON;

// Sensor State Variables
volatile boolean PH_SENSOR_READING_COMPLETE;
volatile boolean EC_SENSOR_READING_COMPLETE;
volatile boolean RTC_UPDATE_READY;
volatile boolean RTC_DOWN;
volatile boolean TIMER_ERROR;


DHT dht(DHTPIN, DHTTYPE); // Crappy Humidity/Temp sensor initializer

void setup()
{
   Serial.begin(9600);
   Serial.print("Beginning Setup.  EV Data is forthcoming\n\n");

   Serial.print("Initiating I2C Communications...\n");

   Serial.print("Getting Status Information from ppm meter:\n");
   Wire.begin();
   
   boolean good_read = false;

  
   Wire.beginTransmission(0x64);
   Wire.write("STATUS\r");
   Wire.endTransmission();
   delay(400);
   char initial_response = Wire.read();
   char next = '1';
   switch(initial_response)
   {
      case 1: 
        while(next != '\0')
        {
           next = Wire.read();
           Serial.print(next); 
        }
        break;
       case 2: 
         Serial.print("Request Failed");
         break;
        
       case 254: 
         Serial.print("Request Pending");
         break;
         
       case 255:
         Serial.print("No Data Recieved");
         break;
         
       default:
         break;
   }


   pinMode(PS_ON, OUTPUT);
   pinMode(PUMP1_RELAY, OUTPUT);
   pinMode(PUMP2_RELAY, OUTPUT);
   pinMode(PUMP_3_RELAY, OUTPUT);
   pinMode(GFCI_RELAY, OUTPUT);
   
   pinMode(LIGHT_SENSOR, INPUT);
   pinMode(PH_SENSOR, INPUT);

   digitalWrite(PS_ON, LOW);
   digitalWrite(GFCI_RELAY, HIGH);
   digitalWrite(PUMP1_RELAY, HIGH);
   digitalWrite(PUMP2_RELAY, LOW);
   digitalWrite(PUMP_3_RELAY, HIGH);

   minutes = 0;
   seconds = 0;

   cli();  // Disables Interrupts

   TCCR1A = 0;  // Zero out Timer Control Register
   TCCR1B = 0;

   // set compare match register to desired timer seconds of 3 seconds
   OCR1A = ONE_SECOND_INTERVAL * 3;

   // turn on CTC mode:
    TCCR1B |= (1 << WGM12);
   // Set CS10 and CS12 bits for 1024 prescaler:
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS12);

   // Enable Timer1 Compare Interrupt:
   TIMSK1 |= (1 << OCIE1A);

   sei();  // Enable Interrupts
}


/*
  Loop Function:  Eventually it will be optimized out
  entirely.  Until then, it just delays.
*/

void loop()
{
  while(true)
  {
    delay(2000);
    check_local_environment();
  }
}

void check_local_environment()
{

  Serial.begin(9600);

  char* buffer = (char *)calloc(128, sizeof(char));
  int onboard_temp = (int)(dht.readTemperature(true, true)*10);

  int hmd = (int)(dht.readHumidity()*10);
  int ph = analogRead(PH_SENSOR);

  int light = analogRead(LIGHT_SENSOR);
  char* lighting = (char *)calloc(8, sizeof(char));
  if(light < 100)
  {
    lighting = "Bright";
  }
  else if(light < 250)
  {
    lighting = "Dim";
  }
  else
  {
    lighting = "Dark";
  }

  char *feeding_status_string = (char *)calloc(64, sizeof(char));

  if(GFCI_ON)
  {
    sprintf(feeding_status_string, "%d seconds remaining in of feeding.", gfci_time_remaining);
  }else
  {
    sprintf(feeding_status_string, "%d:%d (hh:mm) till the next feeding.", GFCI_TIMER_INTERVAL/60, GFCI_TIMER_INTERVAL%60);
  }


  sprintf(buffer, "Timer: %d:%d:%d\n\nTemp: %d.%d | Humidity: %d.%d | Lighting is %s | PH is %d", hours, minutes, seconds, onboard_temp/10, onboard_temp%10, hmd/10, hmd%10, lighting, ph);

  Serial.print(buffer);

  Serial.end();

  free(lighting);
  free(feeding_status_string);
  free(buffer);


}

/*
  Get PH Reading:
*/
double get_ph()
{
  int samples = 50;
  double ph_sum = 0.0;
  int i;

  for(i=0;i<samples;i++)
  {
    ph_sum += analogRead(PH_SENSOR);
    delay(2);
  }
  return((ph_sum/samples));
}

/*
  Timer Compare Interrupt: Not currently disabling interrupts,
  but in the long term this should be done.
*/
ISR(TIMER1_COMPA_vect)
{

}

