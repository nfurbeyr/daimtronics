#include "pi_comm_node.h"
#include "system_data.h"
#include "semi_truck/Teensy_Sensors.h"
#include "semi_truck/Teensy_Actuators.h"

#include <wiringSerial.h>
#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>
#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h>

#define SHORT_SIZE 2

#define RELAY_PIN_1 7
#define RELAY_PIN_2 0

// the value that the teensy sends over to sync communication
#define SYNC_VALUE -32000
// number of bytes in a full set of sensors including sync data
#define SENSOR_DATA_SIZE_W_SYNC 14
// number of bytes in a full set of sensors excluding sync data
#define SENSOR_DATA_SIZE 12

#define UART "/dev/ttyS0"
#define BAUDRATE 9600

// in hz; should match with simulation rate control block of simulink model
#define LOOP_FREQUENCY 20

//#define DEBUG

using namespace std;
static int serial;


/**
 * @brief The main function for the ROS node to communicate with the Teensy
 * over UART. It receives sensor data from the Teensy and publishes this
 * data to the teensy_sensor_data topic. It also subscribes to the
 * teensy_actuator_data topic and writes the values it gets to the Teensy
 * over UART.
 */
int main(int argc, char **argv) {
   short waiting_bytes;
	serial = serialOpen(UART, BAUDRATE);

	// Set the pins on the Pi that toggle the relay between automatic and manual
   wiringPiSetup();
   pinMode(RELAY_PIN_1, OUTPUT);
   pinMode(RELAY_PIN_2, OUTPUT);

   ros::init(argc, argv, "pi_comm_node");
   ros::NodeHandle nh("~");

   semi_truck::Teensy_Sensors sensor_data;
   ros::Publisher publisher = nh.advertise<semi_truck::Teensy_Sensors>
    ("teensy_sensor_data", 10);

   semi_truck::Teensy_Actuators actuator_data;
   ros::Subscriber subscriber = nh.subscribe("teensy_actuator_data", 1,
    actuator_cb);

   ros::WallRate loop_rate(LOOP_FREQUENCY);

   while (ros::ok()) {

      // reads the number of full sets of sensor values coming from the teensy
      waiting_bytes = serialDataAvail(serial);
      for (int i = 0; i < waiting_bytes/SENSOR_DATA_SIZE_W_SYNC; i++) {
         pi_sync();  // prevents data becoming mismatched

         // don't want to read any sensor data unless there is a full set
         waiting_bytes = serialDataAvail(serial);
         if (waiting_bytes >= SENSOR_DATA_SIZE) {
            read_from_teensy(serial, sensor_data);
         }

         print_sensors(sensor_data);
      }

	   publisher.publish(sensor_data);

      // toggles the relay by setting the connected output pins to the relay
      digitalWrite(RELAY_PIN_1, sensor_data.drive_mode);
      digitalWrite(RELAY_PIN_2, !sensor_data.drive_mode);
      
      ros::spinOnce();
      loop_rate.sleep();
   }
}

/**
 * @brief Called before reading sensor data from the Teensy. It will read the
 * buffer until it encounters the SYNC_VALUE that has been defined. Once
 * this happens, the next set of bytes are the set of sensor values.
 */
void pi_sync() {
   int16_t data;
   int16_t avail;

   while ((data = read_sensor_msg(serial, 2)) != SYNC_VALUE){
      printf("error\n");
   }
}


/**
 * @brief Reads a single value from the UART communication buffer.
 * @param serial The serial file descriptor to read from
 * @param num_bytes The number of bytes to read from
 * @return The value of the bytes that have been read
 */
short read_sensor_msg(int serial, char num_bytes) {
   short byte;
   short sensor_msg = 0; // type must match the size of data coming in.

   for (char i = 0; i < num_bytes; i++) {
      byte = serialGetchar(serial);
      byte <<= 8*i; 
      sensor_msg |= byte;
   }

   //printf("sensor message: %i\n", sensor_msg);
   return sensor_msg;
}


/**
 * @brief Reads an entire set of Teensy sensor data by calling
 * read_sensor_msg on each sensor data in the UART message.
 */
void read_from_teensy(int serial, semi_truck::Teensy_Sensors &sensors) {
   short avail = serialDataAvail(serial);
   printf("available bytes: %i\n", avail);

   sensors.imu_angle = read_sensor_msg(serial, SHORT_SIZE);
   sensors.wheel_speed = read_sensor_msg(serial, SHORT_SIZE);
   sensors.right_TOF = read_sensor_msg(serial, SHORT_SIZE);
   sensors.left_TOF = read_sensor_msg(serial, SHORT_SIZE);
   sensors.rear_TOF = read_sensor_msg(serial, SHORT_SIZE);
   sensors.drive_mode = read_sensor_msg(serial, SHORT_SIZE);
}


/**
 * @brief Writes a single actuator value to the Teensy.
 *
 * @param serial The serial file descriptor to write to
 * @param actuator_val The value that is written through UART to the Teensy
 * @param num_bytes Number of bytes to write to the UART
 */
void write_actuator_msg(int serial, short actuator_val, char num_bytes) {
   char *byte_ptr = (char*)&actuator_val;

   for (char i = 0; i < num_bytes; i++) {
      serialPutchar(serial, *byte_ptr++); // write 1 byte for each byte in val
   }
}


/**
 * @brief Writes an entire set of actuator data to the Teensy via UART
 *
 * @param serial The serial file descriptor to write to
 * @param actuators The object that holds all of the actuator data
 */
void write_to_teensy(int serial, const semi_truck::Teensy_Actuators &actuators) {
   write_actuator_msg(serial, SYNC_VALUE, SHORT_SIZE);
   write_actuator_msg(serial, actuators.motor_output, SHORT_SIZE);
   write_actuator_msg(serial, actuators.steer_output, SHORT_SIZE);
   write_actuator_msg(serial, actuators.fifth_output, SHORT_SIZE);
}


/**
 * @brief A function useful for debugging serial communication. Prints the
 * entire set of sensor data to the console.
 *
 * @param sensors The object that holds all of the sensor data
 */
void print_sensors(const semi_truck::Teensy_Sensors &sensors) {
   ROS_INFO("imu angle:\t [%i]", sensors.imu_angle);
   ROS_INFO("wheel speed:\t [%i]", sensors.wheel_speed);
   ROS_INFO("right_TOF:\t [%i]", sensors.right_TOF);
   ROS_INFO("left_TOF:\t [%i]", sensors.left_TOF);
   ROS_INFO("rear_TOF:\t [%i]", sensors.rear_TOF);
   ROS_INFO("drive_mode:\t [%i]", sensors.drive_mode);
}

/**
 * @brief A function useful for debugging serial communication. Prints the
 * entire set of actuator data to the console.
 *
 * @param actuators The object that holds all of the actuator data
 */
void print_actuators(const semi_truck::Teensy_Actuators &actuators) {
   ROS_INFO("motor output:\t [%i]", actuators.motor_output);
   ROS_INFO("steer output:\t [%hu]", actuators.steer_output);
   ROS_INFO("fifth output:\t [%hu]\n", actuators.fifth_output);
}


/**
 * @brief The callback function the the subscriber to the teensy_actuator_data
 * topic. This function will run every time ROS::spinOnce is called. It reads
 * the set of data from the topic and immediately writes these values to the
 * Teensy via UART.
 *
 * @param msg A set of actuator data that has come from the actuator topic
 * and needs to be written to the Teensy.
 */
void actuator_cb(const semi_truck::Teensy_Actuators &msg) {
   #ifdef DEBUG
   ROS_INFO("Got Actuator Message!");
   print_actuators(msg);
   printf("\n********SENDING MESSAGE*********\n");
   printf("Time: %lf\n", ros::WallTime::now().toSec());
   #endif
   write_to_teensy(serial, msg);
}
