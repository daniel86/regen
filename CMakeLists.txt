cmake_minimum_required( VERSION 2.6 FATAL_ERROR )

project (ogle)

include_directories(src/algebra/include)
include_directories(src/animations/include)
include_directories(src/buffers/include)
include_directories(src/camera/include)
include_directories(src/common/include)
include_directories(src/light/include)
include_directories(src/material/include)
include_directories(src/model/include)
include_directories(src/scene/include)
include_directories(src/shader/include)
include_directories(src/text/include)
include_directories(src/textures/include)

add_subdirectory(src)
add_subdirectory(unittest)
add_subdirectory(demos)

