# Set system name and processor
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR ARM)

# Specify the cross compiler
SET(CMAKE_C_COMPILER arm-none-eabi-gcc)
SET(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Target environment paths
SET(CMAKE_FIND_ROOT_PATH /usr/arm-none-eabi /usr/lib/arm-none-eabi)

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MCU "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -marm")
set(OBJECT_GEN_FLAGS "${MCU} -fno-builtin -fno-exceptions -Wall -ffunction-sections -fdata-sections -fomit-frame-pointer -finline-functions -Wno-attributes -Wno-strict-aliasing -Wno-maybe-uninitialized -Wno-missing-attributes -Wno-stringop-overflow")

set(CMAKE_C_FLAGS "${OBJECT_GEN_FLAGS} -std=gnu99 -DNO_SERIAL_OPCODES -DPFFFT_SIMD_DISABLE " CACHE INTERNAL "C Compiler options")
set(CMAKE_CXX_FLAGS "${OBJECT_GEN_FLAGS} -Wno-register" CACHE INTERNAL "C++ Compiler options")
set(CMAKE_ASM_FLAGS "${OBJECT_GEN_FLAGS} -x assembler-with-cpp " CACHE INTERNAL "ASM Compiler options")

# Adjusted linker flags for Cortex-A9
set(CMAKE_EXE_LINKER_FLAGS "${MCU} -Wl,--gc-sections -Wl,-Map=${CMAKE_PROJECT_NAME}.map" CACHE INTERNAL "Linker options")


