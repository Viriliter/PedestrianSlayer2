file(WRITE main.cpp "int main() { return 0; }")

add_library(nlohmann OBJECT main.cpp)

target_include_directories(nlohmann PUBLIC ${PROJECT_SOURCE_DIR}/json.hpp ${PROJECT_SOURCE_DIR}/json_fwd.hpp)