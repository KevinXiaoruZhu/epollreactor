cmake_minimum_required(VERSION 3.10)
project(EpollReactor)

set(CMAKE_CXX_STANDARD 14)

add_executable(EpollReactor server.cpp epoll_reactor_event.h epoll_reactor_event.cpp)