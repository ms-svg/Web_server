cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find CURL package
find_package(CURL REQUIRED)

# Add subdirectory for Search module
add_subdirectory(src/Search)

# Define the executable and its sources
add_executable(server
    main.cpp
    src/Server/TCPServer.cpp
    src/Server/RequestHandler.cpp
    src/Cache/LRUCache.cpp
)

# Include directories
target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link the correct libraries
target_link_libraries(server PRIVATE
    Sengine
    CURL::libcurl
)