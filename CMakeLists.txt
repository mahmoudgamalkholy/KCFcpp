cmake_minimum_required(VERSION 3.10)

# ==============================================================
# 1. Paths & Variables
# ==============================================================
set(NEXUS_AM_HOME "$ENV{HOME}/nexus-am" CACHE PATH "Path to nexus-am")
set(GEM5_BINARY "$ENV{HOME}/gem5/build/RISCV/gem5.opt" CACHE FILEPATH "gem5 binary")
set(GEM5_CONFIG "$ENV{HOME}/gem5/configs/example/kmhv3.py" CACHE FILEPATH "gem5 config")

project(KCF_Tracker CXX)

set(AM_ARCH "riscv64-xs")
# RISCV_MARCH & RISCV_MABI are already set by the toolchain file

# ==============================================================
# 2. nexus-am Archives & Bare-Metal Flags (for gem5)
# ==============================================================
set(RISCV_AM_ARCHIVE  "${NEXUS_AM_HOME}/am/build/am-${AM_ARCH}.a")
set(RISCV_KLIB_ARCHIVE "${NEXUS_AM_HOME}/libs/klib/build/klib-${AM_ARCH}.a")

add_custom_command(
  OUTPUT  "${RISCV_AM_ARCHIVE}" "${RISCV_KLIB_ARCHIVE}"
  COMMAND ${CMAKE_MAKE_PROGRAM} -C "${NEXUS_AM_HOME}/am" ARCH=${AM_ARCH} LINUX_GNU_TOOLCHAIN=1 MARCH=${RISCV_MARCH}
  COMMAND ${CMAKE_MAKE_PROGRAM} -C "${NEXUS_AM_HOME}/libs/klib" ARCH=${AM_ARCH} LINUX_GNU_TOOLCHAIN=1 MARCH=${RISCV_MARCH}
  COMMENT "Building nexus-am am/klib archives for ${AM_ARCH}"
  VERBATIM)

add_custom_target(nexus_am_archives DEPENDS "${RISCV_AM_ARCHIVE}" "${RISCV_KLIB_ARCHIVE}")

set(RISCV_AM_DEFINE_FLAGS
  "-D__ISA__=\"riscv64\"" "-D__ISA_RISCV64__" "-D__PLATFORM__=\"xs\""
  "-D__PLATFORM_XS__" "-D__ARCH__=riscv64-xs" "-D__ARCH_RISCV64_XS"
  "-DARCH_H=\"arch/riscv64-xs.h\"" "-DISA_H=\"riscv.h\"" "-DRISCV_BAREMETAL")

set(RISCV_BAREMETAL_COMPILE_FLAGS -ffreestanding -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector -Wno-main)
set(RISCV_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/riscv_init_array.ld")

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
# 3. Executable and Linkage (Bare-Metal for gem5)
# ==============================================================
FILE(GLOB_RECURSE sourcefiles "src/*.cpp")
# استبعاد ملف الـ baremetal_runtime من الكومبايل الخاص بـ qemu لو موجود
list(REMOVE_ITEM sourcefiles "${CMAKE_CURRENT_SOURCE_DIR}/src/baremetal_runtime.cpp")

add_executable(KCF_gem5 ${sourcefiles})

target_include_directories(KCF_gem5 PRIVATE  
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${NEXUS_AM_HOME}/am"
  "${NEXUS_AM_HOME}/am/include"
  "${NEXUS_AM_HOME}/libs/klib/include"
)

target_compile_options(KCF_gem5 PRIVATE ${RISCV_AM_DEFINE_FLAGS} ${RISCV_BAREMETAL_COMPILE_FLAGS})
target_link_libraries(KCF_gem5 PRIVATE riscv_baremetal_runtime)

set(KCF_GEM5_BIN "${CMAKE_CURRENT_BINARY_DIR}/KCF.bin")
add_custom_command(TARGET KCF_gem5 POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -S --set-section-flags .bss=alloc,contents -O binary "$<TARGET_FILE:KCF_gem5>" "${KCF_GEM5_BIN}"
  COMMENT "objcopy KCF_gem5 -> KCF.bin"
  VERBATIM)

# ==============================================================
# 4. Run XS-GEM5 Simulator (kmhv3.py)
# ==============================================================
set(m5out "${CMAKE_CURRENT_BINARY_DIR}/m5out_kcf")
set(log "${CMAKE_CURRENT_BINARY_DIR}/KCF_gem5.log")

add_custom_target(run-gem5
  COMMAND /bin/sh -c "${GEM5_BINARY} --remote-gdb-port=0 -d ${m5out} ${GEM5_CONFIG} --raw-cpt --generic-rv-cpt=${KCF_GEM5_BIN} --disable-difftest --mem-size=4GB 2>&1 | tee ${log}"
  DEPENDS KCF_gem5
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running KCF.bin on XS-GEM5 (output -> ${log})"
  USES_TERMINAL
  VERBATIM)

# ==============================================================
# 5. Run QEMU Simulator (Linux ELF)
# ==============================================================
FILE(GLOB_RECURSE qemu_sourcefiles "src/*.cpp")
list(REMOVE_ITEM qemu_sourcefiles "${CMAKE_CURRENT_BINARY_DIR}/src/baremetal_runtime.cpp")

add_executable(KCF_qemu.elf ${qemu_sourcefiles})
target_include_directories(KCF_qemu.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(KCF_qemu.elf PRIVATE -std=c++11 -O3 -march=${RISCV_MARCH} -mabi=lp64d -Wall -fno-tree-vectorize)
target_link_options(KCF_qemu.elf PRIVATE -static)
target_link_libraries(KCF_qemu.elf PRIVATE "-lm")

add_custom_target(run-qemu
  COMMAND qemu-riscv64 -cpu max,vlen=128 ./KCF_qemu.elf
  DEPENDS KCF_qemu.elf
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running KCF on QEMU user-mode..."
  USES_TERMINAL
  VERBATIM)