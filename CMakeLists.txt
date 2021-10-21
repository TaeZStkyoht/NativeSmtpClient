cmake_minimum_required(VERSION 3.1)

project(NativeSmtpClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

option(BUILD_EXAMPLES "build examples" ON)

include_directories(include)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

add_library(${PROJECT_NAME} SHARED
    src/MailAddress.cpp
    src/Mail.cpp
    src/SmtpClient.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})
