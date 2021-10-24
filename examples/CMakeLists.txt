add_executable(Examples
    src/main.cpp
)

target_link_libraries(Examples
    $<TARGET_OBJECTS:NativeSmtpClient>
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
)
