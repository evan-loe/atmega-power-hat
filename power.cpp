#include <Arduino.h>

#include <Wire.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <stdint.h>

// power_switch must be HIGH to send power to RPi
const int power_switch = 8;
const int alarm_int_pin = 2;

// the interrupt routine will set this to a positive value when we need to shutdown.
volatile int64_t shutdown_in = -1;
volatile int8_t successful_boot = 0;
int64_t loop_counter = 0;
unsigned char txn_type = 0;
int8_t bytes_recvd = 0;

#define RPI_ON HIGH
#define RPI_OFF LOW

void wakeUp();
void deep_sleep();
void receiveEvent(int howMany); // i2c receive howMany bytes

void wakeUp() {
  // Interrupt Service Routine (ISR)
  // Nothing is needed here, just wake up the microcontroller
}



void setup() {
  pinMode(power_switch, OUTPUT);
  digitalWrite(power_switch, RPI_ON);  // RPi should be on initially.

  pinMode(alarm_int_pin, INPUT_PULLUP);  // Set interrupt pin as input with internal pull-up resistor

  // Attach an interrupt to the pin
  attachInterrupt(digitalPinToInterrupt(alarm_int_pin), wakeUp, FALLING);

  // Initialize I2C communication as a slave with address 0x20
  Wire.begin(0x20);
  // Set up the function to handle incoming data
  Wire.onReceive(receiveEvent);

  Serial.begin(9600);  // For debugging
  Serial.println("\nI2C Receiver Ready. Waiting for signal from Pi to cut power...");
}

void loop() {

  if (successful_boot == 0 && loop_counter > 1200) {
    Serial.println("ERROR: No communication from PI so trying to resuscitate with a power cycle");
    digitalWrite(power_switch, RPI_OFF);
    delay(1000 * 20); // wait 20 seconds
    digitalWrite(power_switch, RPI_ON);
    loop_counter = 0;
    successful_boot = 0;
    shutdown_in = -1;
    Serial.println("Finished power cycle, waiting for communication from PI ...");
  } 
  else if(shutdown_in > 0)
  {
    Serial.print("ATMEGA will cut power to the RPi in ");
    Serial.print((long)shutdown_in);
    Serial.println(" seconds.");
    Serial.flush();
    delay(1000 * shutdown_in);
    shutdown_in = -1;
    digitalWrite(power_switch, RPI_OFF);

    Serial.println("ATMEGA entering sleep mode. Alarm will wake me up...");
    Serial.flush();

    deep_sleep();
    Serial.println("ATMEGA woke up. Turning PI on and waiting for signal to shutdown again...");
    Serial.flush();
    successful_boot = 0;
    loop_counter = 0;

    digitalWrite(power_switch, RPI_ON);
  }

  if (successful_boot != 1) {
    if (loop_counter % 5 == 0) {
      Serial.print("Looping count ");
      Serial.println((int) loop_counter, DEC);
    }
    loop_counter++;
  }
  delay(200);
}

void deep_sleep() {
  // Disable unnecessary peripherals to save power
  ADCSRA &= ~(1 << ADEN);  // Disable ADC
  power_adc_disable();     // Disable ADC
  power_spi_disable();     // Disable SPI
  power_twi_disable();     // Disable I2C
  power_timer0_disable();  // Disable timer 0
  power_timer1_disable();  // Disable timer 1
  power_timer2_disable();  // Disable timer 2


  // Prepare for sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Set the sleep mode to the deepest power-down mode
  noInterrupts();                       // Disable interrupts while configuring sleep mode
  sleep_enable();                       // Enable sleep mode
  interrupts();                         // Re-enable interrupts

  sleep_cpu();  // Put the MCU to sleep. Execution halts here until an interrupt occurs.

  // Execution resumes here after waking up
  sleep_disable();  // Disable sleep mode

  // Re-enable peripherals after waking up
  power_adc_enable();     // enable ADC
  power_spi_enable();     // enable SPI
  power_twi_enable();     // enable I2C
  power_timer0_enable();  // enable timer 0
  power_timer1_enable();  // enable timer 1
  power_timer2_enable();  // enable timer 2
  ADCSRA |= (1 << ADEN);  // Re-enable ADC
}

// This function executes whenever i2c data is received from the master
void receiveEvent(int howMany) {
  // txn_types:
  // 0 - shutdown
  // 1 - successful boot
  while (Wire.available())
  {
    if (bytes_recvd == 0) 
    {
      txn_type = Wire.read();
      bytes_recvd++;
      Serial.println("Received first...");
      Serial.println(txn_type);
    } 
    else 
    {
      bytes_recvd = 0;
      unsigned char data = Wire.read();
      Serial.println("Received second...");
      Serial.println(data);

      if (txn_type == 0) {
        shutdown_in = (int) data;
      } 
      else if (txn_type == 1) 
      {
        successful_boot = 1;
      }
    }
  }
}
