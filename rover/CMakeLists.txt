#FIND_PACKAGE( OpenCV REQUIRED )  
FIND_PACKAGE( Threads REQUIRED )
FIND_PACKAGE( spdlog REQUIRED )
FIND_PACKAGE( PkgConfig )
pkg_check_modules(SERIAL libserial)
pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv4)

FIND_LIBRARY( LIBRT rt )  # It is for mqueue

#INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${LIBSERIAL_INCLUDE_DIRS} )

# <------------ add hiredis dependency --------------->
FIND_PATH(HIREDIS_HEADER hiredis)
FIND_LIBRARY(HIREDIS_LIB hiredis)

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
FIND_PATH(REDIS_PLUS_PLUS_HEADER sw)
FIND_LIBRARY(REDIS_PLUS_PLUS_LIB redis++)

file (GLOB_RECURSE rover_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_compile_options(-pthread)
add_compile_options(-Wno-psabi)
message(rover_SOURCES)
add_executable(rover main.cpp
                src/modules/communication/Message.cpp
                src/modules/modes/Modes.cpp
                src/modules/navigation/EKF.cpp
                src/modules/navigation/LaneDetector.cpp
                src/modules/navigation/UBXDriver.cpp
                src/tasks/ControllerTask.cpp
                src/tasks/NavigationTask.cpp
                src/tasks/SlaveCommunicationTask.cpp
                src/tasks/UserControlTask.cpp
                src/tasks/Task.cpp
                src/tasks/LatencyTracker.cpp
                src/tasks/PosixMutex.cpp
                src/tasks/Schedulers.cpp
                src/tasks/TaskScheduler.cpp
                src/utils/MathFun.cpp)

# Add external libs
include_directories(${CMAKE_SOURCE_DIR}/libs)
link_directories(${CMAKE_SOURCE_DIR}/libs/nlohmann ${CMAKE_SOURCE_DIR}/libs/simdjson)

target_link_libraries(rover ${OPENCV_LIBRARIES} ${SERIAL_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT} spdlog::spdlog ${LIBRT} ${HIREDIS_LIB} ${REDIS_PLUS_PLUS_LIB})
target_include_directories(rover PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${OPENCV_INCLUDE_DIRS} ${SERIAL_INCLUDE_DIRS} ${HIREDIS_HEADER} ${REDIS_PLUS_PLUS_HEADER})
