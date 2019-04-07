#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

typedef struct sensor_data_t {
   short wheel_speed;
   float imu_angle;
   long right_URF;
   long left_URF;
} sensor_data_t;


typedef struct actuator_data_t {
   short motor_output;
   short steer_output;
   short fifth_output;
} actuator_data_t;


typedef struct system_data_t {
   bool updated;
   short drive_mode;
   sensor_data_t sensors;
   actuator_data_t actuators;
} system_data_t;

#endif
