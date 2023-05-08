cmake_minimum_required(VERSION 3.0.0)
project(PedestrianSlayer2 VERSION 0.1.0)

include(CTest)
enable_testing()

FIND_PACKAGE( OpenCV REQUIRED )  
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
                            
add_executable(PedestrianSlayer2 
               src/main.cpp
               src/nodes/navigation/LaneDetector.hpp
               src/nodes/navigation/LaneDetector.cpp
               src/utils/math_fun.h
               src/utils/math_fun.cpp
               src/utils/miscs.h)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
target_link_libraries(PedestrianSlayer2 ${OpenCV_LIBS})

