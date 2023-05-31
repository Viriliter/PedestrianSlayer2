project(nlohmann VERSION 0.1.0)

add_library(nlohmann
            ${PROJECT_SOURCE_DIR}/json.hpp
            ${PROJECT_SOURCE_DIR}/json_fwd.hpp)