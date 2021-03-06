#ifndef IMU_H
#define IMU_H

#include <stdint.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

short imu_loop_fn();

void imu_setup();

void print_imu_data(sensors_event_t *event);
#endif
