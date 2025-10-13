cmake_minimum_required(VERSION 3.20)
project(SnakeGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Chỉ định đường dẫn đến SFMLConfig.cmake
set(SFML_DIR "${CMAKE_SOURCE_DIR}/SFML/lib/cmake/SFML")

# Thêm include và lib
include_directories("${CMAKE_SOURCE_DIR}/SFML/include")
link_directories("${CMAKE_SOURCE_DIR}/SFML/lib")

# Thêm tất cả các file .cpp và .h trong src
file(GLOB SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

# Tạo executable
add_executable(SnakeGame ${SRC_FILES})

# Tìm và link SFML (bao gồm audio)
find_package(SFML 2.6 COMPONENTS graphics window system audio REQUIRED)

target_link_libraries(SnakeGame sfml-graphics sfml-window sfml-system sfml-audio)

# Copy DLL sau khi build
add_custom_command(TARGET SnakeGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/SFML/bin"
    $<TARGET_FILE_DIR:SnakeGame>
)
