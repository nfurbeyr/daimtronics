#include "include/steer_servo.h"
#include <Arduino.h>
#include <Servo.h>

#define STRAIGHT 90
//#define DEBUG
#define MIN_ANGLE 1400 // Time in microseconds of pulse width corresponding to minimum angle
#define MAX_ANGLE 1800 // Time in microseconds of pulse width corresponding to maximum angle

/**
 * This is a Servo object to control the steering servo. It relies on code from
 * Servo.h which is built into the Arduino IDE.
 */
static Servo steer_servo;

/**
 * The is the primary function controlling the steering servo wheel. It reads a
 * value form the system data and controls the steering servo based
 * on what the system data contains.
 *
 * @param steer_output the output to the steering servo that controls angle
 */
void steer_servo_loop_fn(int16_t steer_output) {
   // if output is out of the 0-180 degree range, drive straight
#ifdef DEBUG
   Serial.print("outputting to steer servo: ");
   Serial.println(steer_output);
#endif

   if (steer_output > 180 || steer_output < 0) {
      steer_servo.write(STRAIGHT);
   }
   else if (steer_output != steer_servo.read()) {
      steer_servo.write(steer_output);
   }
}

/**
 * @brief Set up the steer servo task to write to the pin attached to
 * the servo controlling angle of the trucks's front axis.
 *
 * @param servo_pin The pin that signals a PWM to the steering servo.
 */
void steer_servo_setup(short servo_pin) {
   steer_servo.attach(servo_pin, MIN_ANGLE, MAX_ANGLE);
   // delay(15);
   steer_servo.write(STRAIGHT);

}
