cmake_minimum_required(VERSION 3.0.0)
project(PedestrianSlayer2 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

include(CTest)
enable_testing()

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

file (GLOB_RECURSE PedestrianSlayer2_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file (GLOB_RECURSE PedestrianSlayer2_HEADERS CONFIGURE_DEPENDS "src/*.h")

set (PedestrianSlayer2_INCLUDE_DIRS "")

foreach (_headerFile ${PedestrianSlayer2_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND PedestrianSlayer2_INCLUDE_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES PedestrianSlayer2_INCLUDE_DIRS)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_compile_options(-pthread)
add_compile_options(-Wno-psabi)
 
add_executable(${CMAKE_PROJECT_NAME} ${PedestrianSlayer2_SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${PedestrianSlayer2_INCLUDE_DIRS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
target_link_libraries(${CMAKE_PROJECT_NAME} ${OPENCV_LIBRARIES} ${SERIAL_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT} spdlog::spdlog ${LIBRT} ${HIREDIS_LIB} ${REDIS_PLUS_PLUS_LIB})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OPENCV_INCLUDE_DIRS} ${SERIAL_INCLUDE_DIRS} ${HIREDIS_HEADER} ${REDIS_PLUS_PLUS_HEADER})
