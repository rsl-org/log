cmake_minimum_required(VERSION 3.15)
project(mypkg CXX)




add_library(mypkg src/mypkg.cpp)
target_include_directories(mypkg PUBLIC include)



set_target_properties(mypkg PROPERTIES PUBLIC_HEADER "include/mypkg.h")
install(TARGETS mypkg)
