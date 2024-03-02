add_library(simdjson  SHARED simdjson.cpp simdjson.h)

target_include_directories(simdjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})