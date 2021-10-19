cmake_minimum_required(VERSION 3.0)

project(NativeSmtpClient)

include_directories(include)

add_library(NativeSmtpClient OBJECT
    src/MailAddress.cpp
    src/Mail.cpp
    src/SmtpClient.cpp
)

set_property(TARGET NativeSmtpClient PROPERTY CXX_STANDARD 17)
