cmake_minimum_required(VERSION 3.5)
project(myMuduo)

#myMuduo最终编译成so动态库 设置动态库的路径 放在根目录下的lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置调试信息 以及c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
#定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)

# 手动列出源文件
#set(SRC_LIST test.cpp *.h *.cc)
#编译生成动态库myMuduo
add_library(myMuduo SHARED ${SRC_LIST})