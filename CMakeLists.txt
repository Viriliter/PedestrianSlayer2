cmake_minimum_required(VERSION 3.0.0)
project(PedestrianSlayer2 VERSION 0.1.0)

include(CTest)
enable_testing()

FIND_PACKAGE( OpenCV REQUIRED )  
FIND_PACKAGE( Threads REQUIRED )
FIND_PACKAGE(spdlog REQUIRED)
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES( ${LIBSERIAL_INCLUDE_DIRS} )

FIND_PACKAGE( PkgConfig )
pkg_check_modules(SERIAL libserial)

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

#SET(CMAKE_CXX_STANDARD 17)
add_executable(PedestrianSlayer2 ${PedestrianSlayer2_SOURCES})

target_include_directories(PedestrianSlayer2 PRIVATE ${PedestrianSlayer2_INCLUDE_DIRS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
target_link_libraries(PedestrianSlayer2 ${OpenCV_LIBS} ${SERIAL_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT} spdlog::spdlog)
target_include_directories(PedestrianSlayer2 PRIVATE ${SERIAL_INCLUDE_DIRS})
