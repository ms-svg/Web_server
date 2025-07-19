cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)

add_executable(server
    main.cpp
    src/socket/TCPWebServer.cpp
    # add other .cpp files used in your project
)

target_link_libraries(server PRIVATE CURL::libcurl)