/* timed relay system. */

#include <SoftI2C.h>
#include <DS3232RTC.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <TimerOne.h>



#define feed_pin 8
#define waste_pin 12


SoftI2C i2c(A4, A5);
DS3232RTC rtc(i2c);

const int feed_period = 30; // Number of seconds to feed.
const int feed_interval = 7; // Hours to wait between feedings
const struct RTCTime ZEROHOUR = { 0, 0, 0};
const int status_check_interval = 60; // number of interrupts before reconsidering feeding.
const int n_waste_loops = 30;


volatile struct RTCTime prev_feed_time;
volatile int feed_count;

int status_loop_counter;
int waste_loop_counter;
boolean waste_loop_trigger;

void setup() {

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

    Timer1.initialize(5000000);// initialized to 5 second intervals
    Timer1.attachInterrupt(status_check);
 }



void printDec2(int value)
{
    Serial.print((char)('0' + (value / 10)));
    Serial.print((char)('0' + (value % 10)));
}


// Reset Clock to 00:00:00
void clock_reset()
{
  rtc.writeTime(&ZEROHOUR);
}

void status_check() {

  if(status_loop_counter >= status_check_interval)
  {
    status_loop_counter = 0;
    feeding_routine();
  }
  if(waste_loop_trigger)
  {
     if(waste_loop_counter++ == 0)
     {
        digitalWrite(waste_pin, LOW);
     }
     else if(waste_loop_counter >= n_waste_loops)
   {
      waste_loop_counter = 0;
      waste_loop_trigger = false;
      digitalWrite(waste_pin, HIGH);
   }
  }

}

void feeding_routine() {
  // Interrupt routine.
  RTCTime current_time;
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
       digitalWrite(feed_pin, HIGH);
       feed_count++;
       waste_loop_trigger = true;
       waste_loop_counter = 0;
    }
  }
}
