cmake_minimum_required(VERSION 3.10)
project(streams)

set(CMAKE_CXX_STANDARD 17)

add_executable(streams main.cpp include/streams.h include/code_fmt_stream.h)