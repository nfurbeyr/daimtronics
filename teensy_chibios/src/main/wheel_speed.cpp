//
// Created by nate on 4/2/19.
//
#include <Arduino.h>
#include "include/wheel_speed.h"

volatile unsigned long prev_time = 0;
float speed;

/**
 * This is the primary function controlling the wheel speed sensor. It reads an
 * IR sensor mounted so that the sensor reads an alternating black and white
 * strip of tape on the inside of the wheel rims. The frequency that the IR
 * sensor detects a change in reflection of the tape determines the speed of
 * the vehicle.
 *
 * @return the speed that the wheel speed sensor is detecting
 */
int16_t wheel_speed_loop_fn(short PhaseB_pin) {

   short phaseB_val = digitalRead(PhaseB_pin);

   unsigned long time = micros();
   if(phaseB_val==HIGH){
       //Going Forward
       volatile unsigned long t_time = time - prev_time;
       speed = (1/(t_time*0.000001));
       //Serial.println("Wheel_Speed = ");
       //Serial.print(speed);

   } else if(phaseB_val==LOW){
       //Going Backward
       volatile unsigned long t_time = time - prev_time;
       speed = -(1/(t_time*0.000001));
       //Serial.println("Wheel_Speed_neg = ");
       //Serial.print(speed);
   }

   prev_time = time;
   return 0; // placeholder for compilation
}

void wheel_speed_setup() {

}
