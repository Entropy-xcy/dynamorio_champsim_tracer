cmake_minimum_required(VERSION 3.7)

project(champsim_tracer)

# Find DynamoRIO
find_package(DynamoRIO REQUIRED)
if (NOT DynamoRIO_FOUND)
  message(FATAL_ERROR "DynamoRIO package required to build")
endif()

# Set up the client
set(DynamoRIO_USE_LIBC ON)
configure_DynamoRIO_client(champsim_tracer)
use_DynamoRIO_extension(champsim_tracer drmgr)
use_DynamoRIO_extension(champsim_tracer drutil)
use_DynamoRIO_extension(champsim_tracer droption)

# Build the tracer
add_library(champsim_tracer SHARED
    champsim_tracer.cpp
    trace_instruction.h
)

# Set C++ standard
set_property(TARGET champsim_tracer PROPERTY CXX_STANDARD 11)
