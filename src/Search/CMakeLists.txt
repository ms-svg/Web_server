add_library(Sengine STATIC
    FileIndexer.cpp
    Sorting.cpp       
)

target_include_directories(Sengine PUBLIC
    ${CMAKE_SOURCE_DIR}/include/Search
)