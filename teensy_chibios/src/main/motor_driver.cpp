#include "include/motor_driver.h"
#include <Arduino.h>
#include <Servo.h>

#define WHEEL_SPEED_STOP 0 // wheel speed of 0 is no velocity
#define MOTOR_STOP 90 // motor output of 90 is no torque
#define FORWARDS 120 // motor output of 90 is no torque
#define INIT_VALUE 68// this output is the same as the receiver outputs in idle
#define KP 1
#define KI 0.05f
#define SAT_ERROR 1000 // max error sum that can accumulate for integral control
#define MAX_TIME_STEP 500 // in millis
#define WHEEL_SPEED_RANGE 1000 // max error from wheel speed
#define MOTOR_RANGE 180 // max error from wheel speed
#define FULL_REVERSE 1087 // Time in microseconds of pulse width corresponding
// to full reverse
#define FULL_FORWARD 1660 // Time in microseconds of pulse width corresponding to full forward
//#define DEBUG

/**
 * @brief This is a Servo object to control the motor. It relies on code from
 * Servo.h which is built into the Arduino IDE, and is able to control motors
 * as well as servos.
 */
static Servo motor;

/**
 * @brief The accumulated error for integral control of the control loop
 * that stops the motor when the dead man's switch is not pressed.
 */
static int16_t error_sum = 0;

/**
 * @brief This is the primary function controlling the motor. It reads the
 * motor output value from the system data and controls the motor with this
 * value.
 *
 * @param motor_output the output to the motor
 */
void motor_driver_loop_fn(int16_t motor_output) {
   Serial.print("before scale :   ");
   Serial.println(motor_output);
   motor_output = scale_output(motor_output);
   Serial.print("after scale:   ");
   Serial.println(motor_output);
   motor.write(motor_output);
}

/**
 * @brief Scales a value coming from the pi between -100 and 100 to between 0
 * and 180 for what the Servo library requires.
 *
 * @param motor_output An input value, ideally between -100 and 100 (although
 * it will be limited to -100 or 100 if outside this range
 * @return a value to output to the motor between 0 and 180
 */
int16_t scale_output(int16_t motor_output) {
   motor_output = (motor_output > 100) ? 100 : motor_output;
   motor_output = (motor_output < -100) ? -100 : motor_output;

   motor_output = 0.9 * motor_output + 90;

   motor_output = (motor_output < 35) ? 35 : motor_output;

   return motor_output;
}

/**
 * @brief Set up the motor driver task to write to the pin attached to
 * the motor, and to be in the locked position. It also outputs a value
 * corresponding to zero torque.
 *
 * @param motor_pin The pin sends a PWM signal to the motor.
 */
void motor_driver_setup(short motor_pin) {
   motor.attach(motor_pin, FULL_REVERSE, FULL_FORWARD);
   // delay(15);
   motor.write(INIT_VALUE);
}

/**
 * @brief Runs a control loop to stop the motor based on the reported wheel
 * speed, and returns a value to be output to the motor
 *
 * @param wheel_speed speed of the truck read by the wheel speed sensor
 * @param time_step number of millis since the last time this task ran; used
 * in integral control
 * @return the output to the motor
 */
int16_t stop_motor(int16_t wheel_speed, int16_t time_step) {
   int16_t motor_output;
   int16_t error_range = (KI * SAT_ERROR) + (KP * WHEEL_SPEED_RANGE);
   int16_t error = WHEEL_SPEED_STOP - wheel_speed;

   if (error_sum < SAT_ERROR && (time_step > 0 && time_step < MAX_TIME_STEP)) {
      error_sum += time_step * error;
      error_sum = error_sum > SAT_ERROR ? SAT_ERROR : error_sum;
   }

   motor_output = (((KP * error) + (KI * error_sum)) * (MOTOR_RANGE /
    error_range)) + MOTOR_STOP;

   return motor_output;
}

