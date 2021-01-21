cmake_minimum_required(VERSION 2.8)

project(daq)

set(CMAKE_INSTALL_PREFIX "..")

add_executable(daq
  daq.cpp
  dwf.h
  sample.h
)

target_link_libraries(daq dwf)

install(TARGETS daq
  RUNTIME DESTINATION bin
)

