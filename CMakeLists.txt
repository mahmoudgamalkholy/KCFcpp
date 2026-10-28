cmake_minimum_required(VERSION 3.10)

# ==============================================================
set(CMAKE_SYSTEM_NAME Linux) 
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER "riscv64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "riscv64-linux-gnu-g++")
# ==============================================================

project(KCF_Tracker CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -march=rv64g -mabi=lp64d -Wall -static")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

include_directories(src) 
FILE(GLOB_RECURSE sourcefiles "src/*.cpp")

add_executable(KCF.elf ${sourcefiles})

#   Run QEMU Simulator 
#   qemu-riscv64 ./KCF.elf