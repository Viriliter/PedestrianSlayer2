# Pedestrian Car 2 Project - Server
This branch includes source code of Server application. The application runs under Linux operating system. The application uses Flask Web framework, thus, backend of the application is written on Python. For frontend side, Javascript and HTML are used. The backend connects to Redis server, and it communicates with ```Rover``` application.
 
## Getting Started

### Prerequisites
#### Hardware:
Not available

#### Software:
Python Libraries:
* OpenCV (pypi: https://pypi.org/project/opencv-python/)
* Flask (pypi: https://pypi.org/project/Flask/)
* redis (pypi: https://pypi.org/project/redis/)

Additionally, redis-server should be installed on the system to run the application. For installation procedures, follow the link: https://redis.io/

### Installing
Run following command on the terminal to clone the project:
```
git clone -b server --single-branch https://github.com/Viriliter/PedestrianSlayer2.git
```

## Overview of Codebase
```app.py``` is a point where application runs. ```\template``` folder includes HTML pages. ```\template\index.html``` is home page of website. 3rd party Javascript libraries and resources are located in ```\static``` folder.

## Note to Developers
Since mobile devices requires https requests to share device orientation and location data, user should enter the website with https prefix, for example https://127.0.0.1:5000. Although, browsers warn users about that this site is insecure, user should ignore these warnings to acccess the website. A solution for this problem is genarating a valid SSL certification for the website, however, it is not the scope of the project.

Especially for WiFi connections, there could be huge delays in sending POST requests to the server applications. It deteroiates the performance of the system since device orientation cannot be obtained in time which is essential to drive the vehicle. However, in wired connections, this problem is not observed. Therefore, there is some work to fix this issue for WiFi connections.

## Contributing

## Versioning
1.0

## Authors
Mert Limoncuoğlu

## Acknowledgments

