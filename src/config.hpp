#ifndef CONFIG_HPP
#define CONFIG_HPP

// Vehicle Parameters
#define VEHICLE_WEIGHT 0  // Weight of the vehicle in gram
#define AXLE_DISTANCE 0  // Distance between the axles in cm


// Camera Parameters
#define CAMERA_RES_H 720  // Camera resolution in width
#define CAMERA_RES_W 1280  // Camera resolution in height
#define CAMERA_TILT_ANGLE 0  // Angle between center of camera diagram and vertical axis of the vehicle
#define CAMERA_HEIGHT 0  // Distance between camera diagram to ground


// Communication Parameters
#define SLAVE_PORTNAME "/dev/ttyUSB0"
#define SLAVE_BAUD 115200
#define SLAVE_CON_TIMEOUT 100  // Timeout duration in miliseconds

//Task Message Queues
#define MAX_MQ_MSG_SIZE 255  // Defines size of message queue in bytes

//Redis
#define REDIS_ADDR "127.0.0.1"
#define REDIS_PORT "6379"
#define REDIS_URI "tcp://" REDIS_ADDR ":" REDIS_PORT

#endif  //CONFIG_HPP