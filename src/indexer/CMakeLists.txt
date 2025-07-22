add_library(indexer STATIC
    FileIndexer.cpp
)

target_include_directories(indexer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
