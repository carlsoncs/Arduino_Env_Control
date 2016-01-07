#include "EV_CONTROL_BOARD2.h"

// Timer Variables
volatile int seconds;
volatile int minutes;
volatile int hours;

  // Relay Specific Timer Variables
    volatile int gfci_time_remaining;
    const int GFCI_TIMER_INTERVAL;

    volatile int pump1_time_remaining;
    const int PUMP1_TIMER_INTERVAL;

    volatile int pump2_time_remaining;
    const int PUMP2_TIMER_INTERVAL;

    volatile int pump3_time_remaining;
    const int PUMP3_TIMER_INTERVAL;


// Relay State Variables
volatile boolean GFCI_ON;
volatile boolean PUMP1_ON;
volatile boolean PUMP2_ON;
volatile boolean PUMP3_ON;

// Seson State Variables
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
   Serial.print("Locating devices...");
   sensors.begin();
   Serial.print("Found ");
   Serial.print(sensors.getDeviceCount(), DEC);
   Serial.println(" devices.");

   // report parasite power requirements
   Serial.print("Parasite power is: ");
   if (sensors.isParasitePowerMode()) Serial.println("ON");
   else Serial.println("OFF");

   // assign address manually.  the addresses below will beed to be changed
   // to valid device addresses on your bus.  device address can be retrieved
   // by using either oneWire.search(deviceAddress) or individually via
   // sensors.getAddress(deviceAddress, index)
   //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

   // Method 1:
   // search for devices on the bus and assign based on an index.  ideally,
   // you would do this to initially discover addresses on the bus and then
   // use those addresses and manually assign them (see above) once you know
   // the devices on your bus (and assuming they don't change).
   if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");

   // method 2: search()
   // search() looks for the next device. Returns 1 if a new address has been
   // returned. A zero might mean that the bus is shorted, there are no devices,
   // or you have already retrieved all of them.  It might be a good idea to
   // check the CRC to make sure you didn't get garbage.  The order is
   // deterministic. You will always get the same devices in the same order
   //
   // Must be called before search()
   //oneWire.reset_search();
   // assigns the first address found to insideThermometer
   //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

   // show the addresses we found on the bus
   Serial.print("Device 0 Address: ");
   printAddress(insideThermometer);
   Serial.println();

   // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
   sensors.setResolution(insideThermometer, 9);

   Serial.print("Device 0 Resolution: ");
   Serial.print(sensors.getResolution(insideThermometer), DEC);
   Serial.println();
   Serial.end();

   Wire.begin();

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

  if(eating)
  {
    sprintf(feeding_status_string, "%d seconds remaining in of feeding.", remaining_chow_time);
  }else
  {
    sprintf(feeding_status_string, "%d:%d (hh:mm) till the next feeding.", time_to_chow/60, time_to_chow%60);
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

/* Print Address function from the One wire libray */

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
