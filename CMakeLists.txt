cmake_minimum_required(VERSION 3.10)

# ==============================================================
# 1. Compiler Settings
# ==============================================================
set(CMAKE_SYSTEM_NAME Linux) 
set(CMAKE_SYSTEM_PROCESSOR riscv64)
set(CMAKE_C_COMPILER "riscv64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "riscv64-linux-gnu-g++")

project(KCF_Tracker CXX)

# -march=rv64g (بدون v)
# -fno-tree-vectorize لمنع الكومبايلر من استخدام أي Vector instructions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -march=rv64g -mabi=lp64d -Wall -static -fno-tree-vectorize")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

include_directories(src) 
FILE(GLOB_RECURSE sourcefiles "src/*.cpp")

add_executable(KCF.elf ${sourcefiles})

# ==============================================================
# 2. Run QEMU Simulator (Functional Test)
# ==============================================================
add_custom_target(run-qemu
    COMMAND qemu-riscv64 ./KCF.elf
    DEPENDS KCF.elf
    COMMENT "Running KCF.elf on QEMU..."
)

# ==============================================================
# 3. Run gem5 Simulator (Syscall Emulation)
# ==============================================================
set(GEM5_DIR "$ENV{HOME}/gem5")

add_custom_target(run-gem5
    COMMAND ${GEM5_DIR}/build/RISCV/gem5.opt 
        -d ./m5out_kcf           
        ${GEM5_DIR}/configs/example/se.py                  
        --cmd=./KCF.elf          
        --cpu-type=DerivO3CPU                              
        --caches                                           
        --l2cache                                          
        --mem-type=DDR4_2400_8x8                           
        --mem-size=2GB
    DEPENDS KCF.elf
    COMMENT "Running KCF.elf on gem5 (SE Mode)..."
)