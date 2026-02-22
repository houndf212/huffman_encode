cmake_minimum_required(VERSION 3.16)

project(huffman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(huffman
    main.cpp
    huffman_encode.h
    huffman_encode.cpp)

include(GNUInstallDirs)
install(TARGETS huffman
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
