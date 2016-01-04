#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <DHT.h>
#include <string.h>

#define DHTTYPE DHT11

#define GFCI_PIN 8
#define RELAY2_PIN 7
#define RELAY3_PIN 9
#define LIGHT_SENSOR A0
#define DHTPIN 6    // Onboard Temp/Humidity Sensor
#define PS_ON 2    // Onboard ATX "Power Supply On" wire.  High to power off the ATX, Low to turn it on.
                    // Arduino itself gets power from the ATX when not plugged into a USB port, which means,
                    // when it is not connected to a secondary power supply, setting PS_ON HIGH literally
                    // kills all the power to the board which will simultaneous kill to all AC circuits
                    // that are running on a relay. --An accidentally discovered safety measure for the
                    // testing environment; when software design is sufficiently robust it can be given a
                    // secondary power source to effectively remove the 'feature'.


const long ONE_SECOND_INTERVAL = 15624;


/*
*****  User's Section: Primary Environment Settings Variables
*/

const int FEED_TIME = 60;  // seconds
const int FEEDING_INTERVAL = 5; // hours

/*
*****  End User's Section: Primary Environment Settings Variables
*/

// Timer Variables
volatile int seconds = 0;
volatile int minutes = 0;
volatile int hours = 0;



volatile unsigned char eating;  // Boolean: Is a feeding afoot?
volatile int remaining_chow_time; // logical
volatile int time_to_chow; //also logical


DHT dht(DHTPIN, DHTTYPE); // Crappy Humidity/Temp sensor initializer

void setup()
{
   Serial.begin(9600);
   Serial.print("Beginning Setup.  EV Data is forthcoming\n\n");
   Serial.end();
   pinMode(PS_ON, OUTPUT);
   pinMode(RELAY2_PIN, OUTPUT);
   pinMode(GFCI_PIN, OUTPUT);

   digitalWrite(PS_ON, LOW);
   digitalWrite(GFCI_PIN, HIGH);
   digitalWrite(RELAY2_PIN, HIGH);

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

   // Enable Timer1 Overflow Interrupt:
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
    delay(5000);
    check_local_environment();
  }
}

void check_local_environment()
{

  Serial.begin(9600);

  char* buffer = (char *)calloc(128, sizeof(char));
  int tmp = (int)(dht.readTemperature(true, true)*10);
  int hmd = (int)(dht.readHumidity()*10);

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

  if(eating)
  {
    sprintf(feeding_status_string, "%d seconds remaining in of feeding.", remaining_chow_time);
  }else
  {
    sprintf(feeding_status_string, "%d:%d (hh:mm) till the next feeding.", time_to_chow/60, time_to_chow%60);
  }


  sprintf(buffer, "Timer: %d:%d:%d -- %d minutes left till the next feeding. \n\nTemp: %d.%d | Humidity: %d.%d | Lighting is %s | ", hours, minutes, seconds, tmp/10, tmp%10, hmd/10, hmd%10, lighting);

  Serial.print(buffer);

  Serial.end();

  free(lighting);
  free(feeding_status_string);
  free(buffer);


}

/*
  Timer Compare Interrupt: Not currently disabling interrupts,
  but in the long term this should be done.
*/
ISR(TIMER1_COMPA_vect)
{
  cli();
  seconds++;

  if(seconds >= 20) // 1 minute (60/4=15)
  {
    minutes++;
    seconds = 0;
    if(minutes >= FEEDING_INTERVAL)
    {
        eating = true;
        minutes = 0;
        digitalWrite(GFCI_PIN, LOW);

    }else if(eating && remaining_feed_time-- <= 0)
    {
      eating = false;
      time_to_chow = 60 *
      digitalWrite(GFCI_PIN, HIGH);
    }
  }
  sei();
}
