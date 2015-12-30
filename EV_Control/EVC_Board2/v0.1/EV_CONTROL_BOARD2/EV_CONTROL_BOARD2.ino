#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>


#define GFCI_PIN 8
#define RELAY2_PIN 7
#define PS_ON 2

const int ONE_SECOND_INTERVAL = 15624;

volatile int seconds;
volatile int minutes;

void setup()
{
   pinMode(PS_ON, OUTPUT); 
   pinMode(RELAY2_PIN, OUTPUT);
   pinMode(GFCI_PIN, OUTPUT);
   
   digitalWrite(PS_ON, LOW);
   digitalWrite(GFCI_PIN, LOW);
   digitalWrite(RELAY2_PIN, HIGH);
   
   minutes = 0;
   seconds = 0;
   
   cli();  // Disables Interrupts
   
   TCCR1A = 0;  // Zero out Timer Control Register
   TCCR1B = 0;
   
   // set compare match register to desired timer seconds of 4 seconds
   OCR1A = ONE_SECOND_INTERVAL * 4;
   
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
    delay(1000);
  }
}


/*
  Timer Compare Interrupt: Not currently disabling interrupts, 
  but in the long term this should be done.  
*/
ISR(TIMER1_COMPA_vect)
{
  seconds++;
  
  if(seconds >= 15) // 1 minute (60/4=15)
  {
    minutes++;
    seconds = 0;
    if(minutes >= 60)
    {
      minutes = 0;
      digitalWrite(GFCI_PIN, !(digitalRead(GFCI_PIN))); 
    }
  }
}
