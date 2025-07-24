cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find curl package
find_package(CURL REQUIRED)

# Add indexer library first
add_subdirectory(src/indexer)

# Define the executable and its source files
add_executable(server
    main.cpp
    src/socket/TCPWebServer.cpp
    src/Cache/LRUCache.cpp
    # Add other relevant .cpp files here
)

# Include directories
target_include_directories(server PRIVATE
    include
    src/indexer
)

# Link libraries
target_link_libraries(server PRIVATE
    indexer
    CURL::libcurl
)
