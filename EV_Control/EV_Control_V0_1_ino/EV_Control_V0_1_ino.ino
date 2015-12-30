#include <SoftI2C.h>
#include <DS3232RTC.h>
#include <string.h>
#include <TimerOne.h>


#define feed_pin 8
#define waste_pin 7


SoftI2C i2c(A4, A5);
DS3232RTC rtc(i2c);

const boolean dev_mode = true;

const int feed_period = 100; // Number of seconds to feed.
const int feed_interval = 8; // Hours to wait between feedings
const struct RTCTime ZEROHOUR = { 0, 0, 0};
const int loop_interval = 50; // number of interrupts before reconsidering feeding.
const int n_waste_loops = 200;
const long interrupt_interval_length = 5000000;  // microseconds ~ 5 seconds


struct RTCTime prev_feed_time;
struct RTCTime current_time;
volatile int feed_count;

int status_loop_counter;
int waste_loop_counter;
boolean waste_loop_trigger;


void setup() {
  
    Serial.begin(9600);
    pinMode(feed_pin, OUTPUT);
    pinMode(waste_pin, OUTPUT);
    
    digitalWrite(feed_pin, HIGH);
    digitalWrite(waste_pin, HIGH);
    
    feed_count = 0;
    prev_feed_time.hour = 0;
    prev_feed_time.minute = 0;
    prev_feed_time.second = 0;
    waste_loop_trigger = false;
    status_loop_counter = 0;
    
    clock_reset();
    
    Timer1.initialize(interrupt_interval_length);// initialized to 5 second intervals
    Timer1.attachInterrupt(loop);   
    
    interrupts();


 }
 
 
 /*------------------------------------------------------------------------------------------------------------------------
Main Loop
-------------------------------------------------------------------------------------------------------------------------*/

void loop()
{
 while(true)
{
   delay(5000);
   timer_update();
   
} 
  
}


/*------------------------------------------------------------------------------------------------------------------------
 Serial Print the Time
-------------------------------------------------------------------------------------------------------------------------*/


void print_time(RTCTime *given_time)
{
   Serial.print("\ntime: ");
   Serial.print(given_time->hour);
   Serial.print(":");
   Serial.print(given_time->minute);
   Serial.print(":");
   Serial.print(given_time->second);
   Serial.print("\n");
   Serial.flush();
}

void init_rtc_struct(RTCTime *given_struct, int hours, int minutes, int seconds)
{
   if(hours > 23)
  {
   hours = 0;
   minutes = 0; 
   seconds = 0;
  }
  else if(minutes > 60)
  {
    hours = 0;
    minutes = 0;
    seconds = 0;
  }
  else if(seconds > 60)
  {
    hours = 0; 
    minutes = 0;
    seconds = 0;
  }
  
  given_struct->hour = hours;
  given_struct-> minute = minutes;
  given_struct -> second = seconds;
  
}

/*------------------------------------------------------------------------------------------------------------------------
 Reset Clock
-------------------------------------------------------------------------------------------------------------------------*/


// Reset Clock to 00:00:00
void clock_reset()
{
  rtc.writeTime(&ZEROHOUR);
}

/*------------------------------------------------------------------------------------------------------------------------
 Timer Update Loop
-------------------------------------------------------------------------------------------------------------------------*/

void timer_update() {
  
    
    if(dev_mode)
    {
      print_time(&current_time);
      Serial.print("status_loop_counter = ");
      Serial.print(status_loop_counter);
      Serial.print("\nTotal feedings: ");
      Serial.print(feed_count);
      Serial.print("\nLast feeding was at ");
      print_time(&prev_feed_time);
      Serial.print("\nWaste Loop is ");
      if(waste_loop_trigger)
      {
        Serial.print("currently running and has ");
        Serial.print(n_waste_loops - waste_loop_counter);
        Serial.print("loops remaining.\n");
      }
      else
      {
         Serial.print("not currently activated.\n"); 
      }
      Serial.flush();
    }  
    
    if(status_loop_counter >= loop_interval)
    {
      status_loop_counter = 0;
      feeding_routine(); 
    }
    if(waste_loop_trigger)
    {
       if(waste_loop_counter == 0)
       {
          digitalWrite(waste_pin, HIGH); 
          waste_loop_counter++;
       }
       else if(waste_loop_counter >= n_waste_loops)
       {
          waste_loop_counter = 0;
          waste_loop_trigger = false;
          digitalWrite(waste_pin, LOW);
       } 
    }
  
}

/*------------------------------------------------------------------------------------------------------------------------
Feeding Loop
-------------------------------------------------------------------------------------------------------------------------*/

void feeding_routine() {
  
  rtc.readTime(&current_time);
  int i = 0;
  
  if((current_time.hour - feed_interval) >= prev_feed_time.hour)
  {
     if(current_time.minute >= prev_feed_time.minute || current_time.hour - (feed_interval - 2) > prev_feed_time.hour)
    {
       digitalWrite(feed_pin, LOW);
       for(i=0; i<feed_period; i++)
       {
          delay(1000); 
       }
       digitalWrite(feed_pin,HIGH);
       waste_loop_trigger = true;
       waste_loop_counter = 0;
    } 
  }
}
