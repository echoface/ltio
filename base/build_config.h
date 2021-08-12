#ifndef LT_BASE_BUILD_CONFIG_H_
#define LT_BASE_BUILD_CONFIG_H_

//ref: https://abseil.io/docs/cpp/platforms/macros#architecture

// >>>> start detect os
#define OS_POSIX 1
#define OS_LINUX 1
// <<<<< start detect os

// >>>>> start CPU 架构
#if defined(_M_X64) || defined(__x86_64__)

#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_LITTLE_ENDIAN 1

#elif defined(_M_IX86) || defined(__i386__)

#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1

#elif defined(__aarch64__) || defined(_M_ARM64)

#define ARCH_CPU_ARM64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_LITTLE_ENDIAN 1

#else

#error Please add support for your architecture in build/build_config.h

#endif
//<<<<< end arch detection

#endif
