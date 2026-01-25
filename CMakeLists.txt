cmake_minimum_required(VERSION 3.25)

message(STATUS "~~~~~ Build type: ${CMAKE_BUILD_TYPE} ~~~~~")

project(MyElectricityMonitorProject
    VERSION 1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_custom_target(print_build_type
    COMMAND ${CMAKE_COMMAND} -E echo "~~~~~ Build type: ${CMAKE_BUILD_TYPE} ~~~~~"
)

add_executable(MyElectricityMonitor)

add_dependencies(MyElectricityMonitor print_build_type)

target_sources(MyElectricityMonitor
    PRIVATE
        Logger.cpp
        Database.cpp
        Utils.cpp
        UnixUtils.cpp
        ElectricityMonitor.cpp
        Configuration.cpp
        Json.cpp
        SmlDecoder.cpp
        EbzDd3.cpp
        HoymilesHmDtu.cpp
        Gpio.cpp
        SerialPort.cpp
        OnScopeExit.cpp
        CancellationToken.cpp
        main.cpp
)

target_compile_options(MyElectricityMonitor
    PRIVATE
        $<$<CONFIG:DEBUG>: -g -O0 >
        $<$<CONFIG:RELEASE>: -O2 >
        -fdiagnostics-color=always
        -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(MyElectricityMonitor
    PRIVATE
        sqlite3
        json-c
        gpiod
        rf24)

