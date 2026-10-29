cmake_minimum_required(VERSION 3.10)

# ==============================================================
# 1. Base Configuration & Toolchain
# ==============================================================
set(CMAKE_SYSTEM_NAME Linux) 
set(CMAKE_SYSTEM_PROCESSOR riscv64)
set(CMAKE_C_COMPILER "riscv64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "riscv64-linux-gnu-g++")
set(CMAKE_OBJCOPY "riscv64-linux-gnu-objcopy")

project(KCF_Tracker CXX)

# Architecture flags matching the XS-GEM5 bare-metal environment (rv64gcv)
set(AM_ARCH "riscv64-xs")
set(RISCV_MARCH "rv64gcv_zba_zbb_zbc_zbs_zbkb_zbkc_zbkx_zknd_zkne_zknh_zkr_zksed_zksh_zkt")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -march=${RISCV_MARCH} -mabi=lp64d -Wall -fno-tree-vectorize")

# nexus-am and gem5 paths
set(NEXUS_AM_HOME "$ENV{HOME}/nexus-am")
set(GEM5_BINARY "$ENV{HOME}/gem5/build/RISCV/gem5.opt")
set(GEM5_CONFIG "$ENV{HOME}/gem5/configs/example/kmhv3.py")

# ==============================================================
# 2. nexus-am Archives & Bare-Metal Link Options
# ==============================================================
# Define paths to am and klib archives built by nexus-am Makefiles
set(RISCV_AM_ARCHIVE  "${NEXUS_AM_HOME}/am/build/am-${AM_ARCH}.a")
set(RISCV_KLIB_ARCHIVE "${NEXUS_AM_HOME}/libs/klib/build/klib-${AM_ARCH}.a")

# Custom target to ensure the archives are built[cite: 3]
add_custom_command(
  OUTPUT  "${RISCV_AM_ARCHIVE}" "${RISCV_KLIB_ARCHIVE}"
  COMMAND ${CMAKE_MAKE_PROGRAM} -C "${NEXUS_AM_HOME}/am"
          ARCH=${AM_ARCH} LINUX_GNU_TOOLCHAIN=1 MARCH=${RISCV_MARCH}
  COMMAND ${CMAKE_MAKE_PROGRAM} -C "${NEXUS_AM_HOME}/libs/klib"
          ARCH=${AM_ARCH} LINUX_GNU_TOOLCHAIN=1 MARCH=${RISCV_MARCH}
  COMMENT "Building nexus-am am/klib archives for ${AM_ARCH}"
  VERBATIM)

add_custom_target(nexus_am_archives
  DEPENDS "${RISCV_AM_ARCHIVE}" "${RISCV_KLIB_ARCHIVE}")

# Nexus-am C preprocessor flags[cite: 3]
set(RISCV_AM_DEFINE_FLAGS
  "-D__ISA__=\"riscv64\"" "-D__ISA_RISCV64__" "-D__PLATFORM__=\"xs\""
  "-D__PLATFORM_XS__" "-D__ARCH__=riscv64-xs" "-D__ARCH_RISCV64_XS"
  "-DARCH_H=\"arch/riscv64-xs.h\"" "-DISA_H=\"riscv.h\"" "-DRISCV_BAREMETAL")

# Freestanding flags[cite: 3]
set(RISCV_BAREMETAL_COMPILE_FLAGS
  -ffreestanding -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector -Wno-main)

# Self-contained linker script[cite: 4]
set(RISCV_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/riscv_init_array.ld")

# The runtime interface target for bare-metal linking[cite: 3]
add_library(riscv_baremetal_runtime INTERFACE)
target_link_libraries(riscv_baremetal_runtime INTERFACE
  "-Wl,--start-group" "${RISCV_AM_ARCHIVE}" "${RISCV_KLIB_ARCHIVE}" "-Wl,--end-group" "-lgcc")
target_link_options(riscv_baremetal_runtime INTERFACE
  "-nostdlib" "-nostartfiles" "-Wl,--build-id=none"
  "-T" "${RISCV_LINKER_SCRIPT}"
  "-L" "${NEXUS_AM_HOME}/am/src/nemu/ldscript"
  "-Wl,--gc-sections")
add_dependencies(riscv_baremetal_runtime nexus_am_archives)

# ==============================================================
# 3. Executable and Linkage
# ==============================================================
include_directories(src) 
FILE(GLOB_RECURSE sourcefiles "src/*.cpp")

add_executable(KCF_gem5 ${sourcefiles})

# Apply bare-metal compile options[cite: 3]
target_compile_options(KCF_gem5 PRIVATE ${RISCV_AM_DEFINE_FLAGS} ${RISCV_BAREMETAL_COMPILE_FLAGS})

# Link against the baremetal runtime[cite: 3]
target_link_libraries(KCF_gem5 PRIVATE riscv_baremetal_runtime)

# Generate the .bin file from the ELF using objcopy[cite: 3]
set(KCF_GEM5_BIN "${CMAKE_CURRENT_BINARY_DIR}/KCF.bin")
add_custom_command(TARGET KCF_gem5 POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -S --set-section-flags .bss=alloc,contents
          -O binary "$<TARGET_FILE:KCF_gem5>" "${KCF_GEM5_BIN}"
  COMMENT "objcopy KCF_gem5 -> KCF.bin"
  VERBATIM)

# ==============================================================
# 4. Run XS-GEM5 Simulator (kmhv3.py)
# ==============================================================
set(m5out "${CMAKE_CURRENT_BINARY_DIR}/m5out_kcf")
set(log "${CMAKE_CURRENT_BINARY_DIR}/KCF_gem5.log")

# Custom target running kmhv3.py with --raw-cpt and --generic-rv-cpt pointing to the .bin file[cite: 3]
add_custom_target(run-gem5
  COMMAND /bin/sh -c
    "${GEM5_BINARY} --remote-gdb-port=0 -d ${m5out} ${GEM5_CONFIG} --raw-cpt --generic-rv-cpt=${KCF_GEM5_BIN} --disable-difftest 2>&1 | tee ${log}"
  DEPENDS KCF_gem5
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running KCF.bin on XS-GEM5 (output -> ${log})"
  USES_TERMINAL
  VERBATIM)