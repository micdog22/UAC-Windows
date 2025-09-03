
cmake_minimum_required(VERSION 3.20)
project(uacctl LANGUAGES CXX RC)

add_executable(uacctl
    src/main.cpp
    res/uacctl.rc
)

target_compile_features(uacctl PRIVATE cxx_std_17)
target_compile_definitions(uacctl PRIVATE UNICODE _UNICODE)
if (MSVC)
  target_compile_options(uacctl PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(uacctl PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(uacctl PRIVATE advapi32)
