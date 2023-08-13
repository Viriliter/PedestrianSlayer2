# Pedestrian Car 2 Project - Rover
This branch includes source code of Rover application. The application runs under real-time Linux and it is developed on C++. It utilizes inter-process communication in between both the real-time threads and the Server application (https://github.com/Viriliter/PedestrianSlayer2/tree/server). Real-time thread communication is performed using POSIX qmessage, and the messages are transmitted in MessagePack format. ```nlohmann``` library (https://github.com/nlohmann/json) is used for serialization and deserialization of these messages. The library is included inside the project so no need to install into the system. 

The application executes high level decisions like navigating, driving the rover and handle user inputs. Sensor fusion algorithms and image processing algorithms are also performed in this application. Corresponding outputs of these algorithms are shared with other applications (server and board) using either serial communication or inter-process communication techniques. For this purpose, Redis server is used to establish communication between Server and Rover applications. In this way, user inputs and rover information can be shared among different applications.
 
## Getting Started

### Prerequisites
This branch requires following 3rd-party libraries to build the project:

* OpenCV  (github: https://github.com/opencv/opencv)
* spdlog  (github: https://github.com/gabime/spdlog)
* hiredis  (github: https://github.com/redis/hiredis)
* redis-plus-plus (github: https://github.com/sewenew/redis-plus-plus)
* libserial  (github: https://github.com/crayzeewulf/libserial)

Additionally, redis-server should be installed to run the application. For installation procedures, follow the link: https://redis.io/

### Installing
Run following command on the terminal to clone the project:
```
git clone -b rover --single-branch https://github.com/Viriliter/PedestrianSlayer2.git
```

## Overview of Codebase
 The project source code is located under ```/src``` folder. ```/src/Main.cpp``` is a point where application runs. There are several tasks defined in  ```/src/tasks``` runs independetly from each other. Additionally, there are set of different modules, which are located in ```/src/modules```, used by various tasks.
 
 ```TaskScheduler``` class is responsible to configure, to start, and to stop the corresponding tasks.

 ```ControllerTask``` is a decision making task that process incomming data from other tasks and executes appropriate outputs for relevant tasks. 

 ```NavigationTask``` is responsible to obtain sensor outputs (GNSS, IMU, etc.) and runs localization algorithms and Extended Kalman Filter (EKF) in order to feed position of vehicle to ```ControllerTask```. 

 ```SlaveCommunicationTask``` is responsible to coomunicate with  Board firmware via serial messages.


## Contributing

## Versioning
1.0

## Authors
Mert Limoncuoğlu

## Acknowledgments

