/**
 * @file This file holds the main loop that that runs on the Teensy. It
 * creates a ChibiOS thread for each task in the system and assigns
 * priorities and stack sizes. Within the while loop for each thread, the
 * "primary" function for a task is called. The project is organized so that
 * the implementation of primary a function for a task is held in the task's
 * respective .cpp file (which is also within the same "main" folder that
 * main.ino is in). Header files for each cpp file are found within the
 * "include" folder that is in the "main" folder, and these header files must
 * be included here.
 *
 * @author Daimtronics
 */

#include <stdint.h>
#include "include/system_data.h"
#include "include/fifth_wheel.h"
#include "include/imu.h"
#include "include/motor_driver.h"
#include "include/range_finder.h"
#include "include/RC_receiver.h"
#include "include/steer_servo.h"
#include "include/teensy_serial.h"
#include "include/wheel_speed.h"

#include <ChRt.h>

#define HWSERIAL Serial1

/**
 * @brief The data for the entire system. Synchronization will be achieved
 * through the use of ChibiOS's mutex library.
 *
 * Tasks that control a sensor will call the primary function to read that
 * sensor, obtain the mutex to for the system data, write the sensor data to
 * the system_data, and then release the mutex on the system_data.
 *
 * Tasks that control actuators will read from the system_data (no mutex
 * needed as the system_data is read-only for actuator tasks) to receive the
 * specific data that corresponds to their task and then run their primary
 * functions to control the actuators.
 */
static system_data_t system_data = {0};
MUTEX_DECL(sysMtx);


/**
 * @brief Heartbeat Thread: blinks the LED periodically so that the user is
 * sure that the Teensy program is still running and has not crashed.
 */
static THD_WORKING_AREA(heartbeat_wa, 64);

static THD_FUNCTION(heartbeat_thread, arg) {
   (void)arg;
   pinMode(LED_BUILTIN, OUTPUT);
   while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      chThdSleepMilliseconds(100);
      digitalWrite(LED_BUILTIN, LOW);
      chThdSleepMilliseconds(1500);
   }
}



/**
 * @brief Fifth Wheel Thread: Reads desired state of the fifth wheel from the
 * system_data and outputs a servo angle corresponding to locked or unlocked.
 *
 * This thread calls fifth_wheel_loop_fn() which is the primary function for the
 * fifth wheel and whose implementation is found in fifth_wheel.cpp.
 */
static THD_WORKING_AREA(fifth_wheel_wa, 64);

static THD_FUNCTION(fifth_wheel_thread, arg) {
   int16_t fifth_output;

   while (true) {
      //Serial.println("fifth wheel");
      fifth_output = system_data.actuators.fifth_output;

      fifth_wheel_loop_fn(fifth_output);

      chThdSleepMilliseconds(100);
   }
}



/**
 * @brief IMU Thread: Reads euler angles from the BNO055 IMU and writes the
 * data to the system_data after obtaining the system_data mutex.
 *
 * This thread calls imu_loop_fn() which is the primary function for the IMU
 * and whose implementation is found in imu.cpp.
 */
static THD_WORKING_AREA(imu_wa, 2048);

static THD_FUNCTION(imu_thread, arg) {
   int16_t imu_angle;

   while (true) {
      Serial.println("*****************************************************");
      //Serial.println("imu");
      imu_angle = imu_loop_fn();


      chMtxLock(&sysMtx);
      system_data.sensors.imu_angle = imu_angle;
      system_data.updated = true;
      chMtxUnlock(&sysMtx);

      chThdSleepMilliseconds(500);
   }
}



/**
 * @brief Motor Driver Thread: Reads motor output levels from the system data
 * and controls the motor based on this value.
 *
 * This thread calls motor_driver_loop_fn which is the primary function for
 * the motor and whose implementation is found in motor_driver.cpp.
 */
static THD_WORKING_AREA(motor_driver_wa, 64);

static THD_FUNCTION(motor_driver_thread, arg) {
   int16_t motor_output;
   int16_t wheel_speed;
   int16_t time_step;
   int16_t last_time = ST2MS(chVTGetSystemTime());
   int16_t current_time = last_time;

   while (true) {
      //Serial.println("motor");
      motor_output = system_data.actuators.motor_output;
      wheel_speed = system_data.sensors.wheel_speed;
      
      current_time = ST2MS(chVTGetSystemTime());
      time_step = current_time - last_time;

      if (system_data.deadman) {
         motor_driver_loop_fn(motor_output);
      }
      else {
         motor_output = stop_motor(wheel_speed, time_step);
         motor_driver_loop_fn(motor_output);
      }
       
      last_time = current_time;
      chThdSleepMilliseconds(100);
   }
}


/**
 * @brief Range Finder Thread: Controls the HC-SR04 URF where nearby object
 * distance is calculated and written to the system data.
 *
 * @todo need to test this task with the Maxbotix URF
 */
static THD_WORKING_AREA(range_finder_wa, 64);

static THD_FUNCTION(range_finder_thread, arg) {
   (void)arg;
   //Serial.println("starting up URF driver");

   while (true) {

      chSysLock();

      range_finder_ping();

      chSysUnlock();

      chThdSleepMilliseconds(100);
   }
}

BSEMAPHORE_DECL(urf_isrSem, true);

void urf_ISR_Fcn() {
    CH_IRQ_PROLOGUE();
    // IRQ handling code, preemptable if the architecture supports it.

    chSysLockFromISR();
    // Invocation of some I-Class system APIs, never preemptable.

    // Signal handler task.
    chBSemSignalI(&urf_isrSem);
    chSysUnlockFromISR();

    // More IRQ handling code, again preemptable.

    // Perform rescheduling if required.
    CH_IRQ_EPILOGUE();
}

// Handler task for interrupt.
static THD_WORKING_AREA(urf_isr_wa_thd, 128);

static THD_FUNCTION(urf_handler, arg) {
    (void)arg;
    long urf_dist;
    while (true) {
        // wait for event
        chBSemWait(&urf_isrSem);

        urf_dist = range_finder_loop_fn();

    }
}

/**
 * @brief RC Receiver Thread: Reads auxiliary signals from the RC receiver
 * for determining what drive mode the semi-truck is in.
 *
 * This thread calls RC_receiver_loop_fn which is the primary function for
 * the RC receiver and whose implementation is found in RC_receiver.cpp
 */
static THD_WORKING_AREA(RC_receiver_wa, 64);

static THD_FUNCTION(RC_receiver_thread, arg) {
   //int16_t drive_mode;

   while (true) {
      //Serial.println("rc");
      //drive_mode = RC_receiver_loop_fn();

      //chMtxLock(&sysMtx);
      //system_data.drive_mode = drive_mode;
      //chMtxUnlock(&sysMtx);

      chThdSleepMilliseconds(100);
   }
}

BSEMAPHORE_DECL(rc_sw1_isrSem, true);

void RC_SW1_ISR_Fcn() {
    CH_IRQ_PROLOGUE();
    // IRQ handling code, preemptable if the architecture supports it.

    chSysLockFromISR();
    // Invocation of some I-Class system APIs, never preemptable.

    // Signal handler task.
    chBSemSignalI(&rc_sw1_isrSem);
    chSysUnlockFromISR();

    // More IRQ handling code, again preemptable.

    // Perform rescheduling if required.
    CH_IRQ_EPILOGUE();
}

/**
 * @brief RC Receiver Swicth 1 Thread: Reads auxiliary signals from the RC receiver switch 1
 * for determining if the deadman switch is pressed
 *
 * This thread calls RC_receiver_SW1_fn which is the primary function for
 * the RC receiver switch 1 and whose implementation is found in RC_receiver.cpp
 */
static THD_WORKING_AREA(rc_sw1_isr_wa_thd, 64);

static THD_FUNCTION(rc_sw1_handler, arg) {
    (void)arg;
    int16_t deadman_mode;
    while (true) {
        // wait for event
        chBSemWait(&rc_sw1_isrSem);

        deadman_mode = RC_receiver_SW1_fn(15);

        chMtxLock(&sysMtx);
        system_data.deadman = deadman_mode;
        system_data.updated = true;
        chMtxUnlock(&sysMtx);

    }
}


BSEMAPHORE_DECL(rc_sw2_isrSem, true);

void RC_SW2_ISR_Fcn() {
    CH_IRQ_PROLOGUE();
    // IRQ handling code, preemptable if the architecture supports it.

    chSysLockFromISR();
    // Invocation of some I-Class system APIs, never preemptable.

    // Signal handler task.
    chBSemSignalI(&rc_sw2_isrSem);
    chSysUnlockFromISR();

    // More IRQ handling code, again preemptable.

    // Perform rescheduling if required.
    CH_IRQ_EPILOGUE();
}

/**
 * @brief RC Receiver Swicth 2 Thread: Reads auxiliary signals from the RC receiver switch 2
 * for determining what drive mode the semi-truck is in.
 *
 * This thread calls RC_receiver_SW2_fn which is the primary function for
 * the RC receiver switch 2 and whose implementation is found in RC_receiver.cpp
 */
static THD_WORKING_AREA(rc_sw2_isr_wa_thd, 64);

static THD_FUNCTION(rc_sw2_handler, arg) {
    (void)arg;
    int16_t drive_mode;
    while (true) {
        // wait for event
        chBSemWait(&rc_sw2_isrSem);

        drive_mode = RC_receiver_SW2_fn(16);

        chMtxLock(&sysMtx);
        system_data.drive_mode_1 = drive_mode;
        system_data.updated = true;
        chMtxUnlock(&sysMtx);

    }
}


BSEMAPHORE_DECL(rc_sw3_isrSem, true);

void RC_SW3_ISR_Fcn() {
    CH_IRQ_PROLOGUE();
    // IRQ handling code, preemptable if the architecture supports it.

    chSysLockFromISR();
    // Invocation of some I-Class system APIs, never preemptable.

    // Signal handler task.
    chBSemSignalI(&rc_sw3_isrSem);
    chSysUnlockFromISR();

    // More IRQ handling code, again preemptable.

    // Perform rescheduling if required.
    CH_IRQ_EPILOGUE();
}

/**
 * @brief RC Receiver Swicth 3 Thread: Reads auxiliary signals from the RC receiver switch 3
 * for determining what drive mode the semi-truck is in.
 *
 * This thread calls RC_receiver_SW3_fn which is the primary function for
 * the RC receiver switch 3 and whose implementation is found in RC_receiver.cpp
 */
static THD_WORKING_AREA(rc_sw3_isr_wa_thd, 64);

static THD_FUNCTION(rc_sw3_handler, arg) {
    (void)arg;
    int16_t drive_mode;
    while (true) {
        // wait for event
        chBSemWait(&rc_sw3_isrSem);

        drive_mode = RC_receiver_SW3_fn(17);

        chMtxLock(&sysMtx);
        system_data.drive_mode_2 = drive_mode;
        system_data.updated = true;
        chMtxUnlock(&sysMtx);

    }
}

/**
 * @brief Steer Servo Thread: Controls the servo that dictates the driving
 * angle of the semi truck
 *
 * This thread calls steer_servo_loop_fn which is the primary function for
 * the steering servo and whose implementation is found in steer_servo.cpp
 */
static THD_WORKING_AREA(steer_servo_wa, 64);

static THD_FUNCTION(steer_servo_thread, arg) {
   int16_t steer_output;

   while (true) {

      //Serial.println("steer");
      steer_output = system_data.actuators.steer_output;

      steer_servo_loop_fn(steer_output);

      chThdSleepMilliseconds(100);
   }
}



/**
 * @brief Teensy Serial Thread: Communicates over the serial (UART) port to
 * relay system data between the Teensy and the Raspberry Pi.
 *
 * This thread calls serial_loop_fn() which is the primary function for the
 * serial communication and whose implementation is found in teensy_serial.cpp.
 */
static THD_WORKING_AREA(teensy_serial_wa, 2048);

static THD_FUNCTION(teensy_serial_thread, arg) {

   while (true) {

      //Serial.println("serial");

      chMtxLock(&sysMtx);
      teensy_serial_loop_fn(&system_data);
      chMtxUnlock(&sysMtx);

      chThdSleepMilliseconds(20);
   }
}



BSEMAPHORE_DECL(speed_isrSem, true);

void speed_ISR_Fcn() {
    CH_IRQ_PROLOGUE();
    // IRQ handling code, preemptable if the architecture supports it.

    chSysLockFromISR();
    // Invocation of some I-Class system APIs, never preemptable.

    // Signal handler task.
    chBSemSignalI(&speed_isrSem);
    chSysUnlockFromISR();

    // More IRQ handling code, again preemptable.

    // Perform rescheduling if required.
    CH_IRQ_EPILOGUE();
}

/**
 * @brief Wheel Speed Thread: Controls the Hall sensor that is outputted from
 * the main motor of the vehicle for determining wheel speed.
 *
 * This thread calls wheel_speed_loop_fn() which is the primary function for the
 * Hall sensor and whose implementation is found in wheel_speed.cpp.
 */
static THD_WORKING_AREA(wheel_speed_wa, 64);

static THD_FUNCTION(wheel_speed_thread, arg) {
   int16_t wheel_speed;

   while (true) {

      chBSemWait(&speed_isrSem);
      //Serial.println("wheel");
      wheel_speed = wheel_speed_loop_fn(7);

      chMtxLock(&sysMtx);
      system_data.sensors.wheel_speed = wheel_speed;
      chMtxUnlock(&sysMtx);

   }
}


/**
 * @brief Creates the threads to be run by assigning the thread function,
 * working space, priority and any parameters that the thread needs.
 *
 * While the static thread definitions are written before this function, none
 * of them are used until chThdCreateStatic(...) is called.
 */
void chSetup() {
   chThdCreateStatic(heartbeat_wa, sizeof(heartbeat_wa),
   NORMALPRIO, heartbeat_thread, NULL);

   chThdCreateStatic(fifth_wheel_wa, sizeof(fifth_wheel_wa),
   NORMALPRIO, fifth_wheel_thread, NULL);

   chThdCreateStatic(imu_wa, sizeof(imu_wa),
   NORMALPRIO, imu_thread, NULL);

   chThdCreateStatic(motor_driver_wa, sizeof(motor_driver_wa),
   NORMALPRIO, motor_driver_thread, NULL);

   chThdCreateStatic(range_finder_wa, sizeof(range_finder_wa),
   NORMALPRIO, range_finder_thread, NULL);

   chThdCreateStatic(RC_receiver_wa, sizeof(RC_receiver_wa),
   NORMALPRIO, RC_receiver_thread, NULL);

   chThdCreateStatic(steer_servo_wa, sizeof(steer_servo_wa),
   NORMALPRIO, steer_servo_thread, NULL);

   chThdCreateStatic(teensy_serial_wa, sizeof(teensy_serial_wa),
   NORMALPRIO, teensy_serial_thread, NULL);

   chThdCreateStatic(wheel_speed_wa, sizeof(wheel_speed_wa),
   NORMALPRIO, wheel_speed_thread, NULL);
   attachInterrupt(digitalPinToInterrupt(11), speed_ISR_Fcn, RISING);

   chThdCreateStatic(urf_isr_wa_thd, sizeof(urf_isr_wa_thd),
   NORMALPRIO + 1, urf_handler, NULL);
   attachInterrupt(digitalPinToInterrupt(24), urf_ISR_Fcn, CHANGE);

   chThdCreateStatic(rc_sw1_isr_wa_thd, sizeof(rc_sw1_isr_wa_thd),
   NORMALPRIO + 1, rc_sw1_handler, NULL);
   attachInterrupt(digitalPinToInterrupt(15), RC_SW1_ISR_Fcn, CHANGE);

   chThdCreateStatic(rc_sw2_isr_wa_thd, sizeof(rc_sw2_isr_wa_thd),
   NORMALPRIO + 1, rc_sw2_handler, NULL);
   attachInterrupt(digitalPinToInterrupt(16), RC_SW2_ISR_Fcn, CHANGE);

   chThdCreateStatic(rc_sw3_isr_wa_thd, sizeof(rc_sw3_isr_wa_thd),
   NORMALPRIO + 1, rc_sw3_handler, NULL);
   attachInterrupt(digitalPinToInterrupt(17), RC_SW3_ISR_Fcn, CHANGE);

}


/**
 * @brief Initializes the semi-truck system so that it is ready to run in an
 * RTOS environment.
 *
 * The setup function is default to Arduino sketches, and holds all of the
 * code that must be run before the main loop can be run. In this project,
 * each task will have a setup function in it's .cpp file that is called here.
 * Finally, ChibiOS setup is called to start initialize threads and start the
 * thread scheduling that is built in to ChibiOS.
 */
void setup() {
   // Setup the serial ports -- both the hardware (UART) and console (USB)
   teensy_serial_setup();
   // Setup the IMU to make sure it is connected and reading
   imu_setup();
   // Setup the fifth wheel
   fifth_wheel_setup();
   // Setup the motor driver
   motor_driver_setup();
   // Setup the URFs
   range_finder_setup();
   // Setup the RC receiver
   RC_receiver_setup();
   // Setup the steering servo
   steer_servo_setup();
   // Setup the wheel speed sensors
   wheel_speed_setup();
   // chBegin() resets stacks and should never return.
   chBegin(chSetup);

   while (true) {}
}



// loop() is the main thread.  Not used in this example.
void loop() {
}



//ISR FUNCTION CODE
