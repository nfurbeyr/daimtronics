#include <Arduino.h>
#include "include/teensy_serial.h"

#define NUM_MSGS 4
#define SHORT 2
#define FLOAT 2

// number of times the teensy should read form serial for every time it writes
// to it. Helps with serial buffer overflow issues
#define READ_CYCLES 2


/**
 * @brief The primary function for communicating between the Teensy and the
 * Pi over the Serial UART port.
 * @param imu_angle
 */
void teensy_serial_loop_fn(system_data_t *system_data) {

   // reading from PC console and outputting to PC console
   if (system_data->updated) {
      Serial.print("Received from imu: ");
      Serial.println(system_data->sensors.imu_angle, DEC);

      if (HWSERIAL.availableForWrite()) {
         print_sensor_msg(&system_data->sensors);
         HWSERIAL.write((char*)&(system_data->sensors), sizeof(sensor_data_t));
         system_data->updated = false;
      }
   }


   Serial.print("HW bytes: ");
   Serial.println(HWSERIAL.available());
   // communicate with Pi and the ROS network
   for (int i = 0; i < READ_CYCLES; i++) {
      if (HWSERIAL.available() > 0) {
         read_from_pi(&(system_data->actuators));
         Serial.println("Received from PI:");
         print_actuator_msg(&system_data->actuators);
      }
   }


}


void teensy_serial_setup(){
   Serial.begin(9600);
   HWSERIAL.begin(9600);
}


void set_sensor_msg(int user_input, sensor_data_t *data_ptr) {
   data_ptr->wheel_speed = user_input;
   data_ptr->imu_angle= user_input+1;
   data_ptr->right_URF = user_input+2;
   data_ptr->left_URF = user_input+3;
}


void read_from_pi(actuator_data_t *actuators_ptr) {
   byte data_buffer[FLOAT];

   HWSERIAL.readBytes(data_buffer, SHORT);
   actuators_ptr->motor_output = (short) *data_buffer;
   HWSERIAL.readBytes(data_buffer, SHORT); // float data!
   actuators_ptr->steer_output = (short) *data_buffer;
   HWSERIAL.readBytes(data_buffer, SHORT);
   actuators_ptr->fifth_output = (short) *data_buffer;
}


void print_sensor_msg(sensor_data_t *sensors_ptr) {
   Serial.printf("Wheel speed: %i\t", sensors_ptr->wheel_speed);
   Serial.printf("IMU angle:");
   Serial.print(sensors_ptr->imu_angle, 4);
   Serial.printf("\t");
   Serial.printf("Right URF: %i\t", sensors_ptr->right_URF);
   Serial.printf("Left URF: %i\t", sensors_ptr->left_URF);
   Serial.printf("Rear URF: %i\n", sensors_ptr->rear_URF);
}

void print_actuator_msg(actuator_data_t *actuators_ptr) {
   Serial.printf("Motor output: %i\t", actuators_ptr->motor_output);
   Serial.printf("Steer output: %i\t", actuators_ptr->steer_output);
   Serial.printf("Fifth output: %i\n", actuators_ptr->fifth_output);
}
