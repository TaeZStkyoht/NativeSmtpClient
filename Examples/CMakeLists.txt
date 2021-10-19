cmake_minimum_required(VERSION 3.0)

project(Examples)

include_directories(../NativeSmtpClient/include)

add_subdirectory(../NativeSmtpClient NativeSmtpClientLib)

add_executable(Examples
    src/main.cpp
)

target_link_libraries(Examples
    $<TARGET_OBJECTS:NativeSmtpClient>
)

set_property(TARGET Examples PROPERTY CXX_STANDARD 17)
