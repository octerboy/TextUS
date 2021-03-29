#ifndef TEXTUS_OS_H
#define TEXTUS_OS_H

#define TLONG_FMT "%ld"
#define TLONG_FMTx "%lx"
#define TLONG_FMTu "%lu"
#if defined(__LP64__)
// LP64 machine, OS X or Linux
#define TEXTUS_LONG long
#define TEXTUS_PLATFORM_64 1
#elif defined( _MSC_VER ) && (  defined(_M_X64)  ||  defined(_WIN64))
// msvc 64 
#define TEXTUS_LONG __int64
#define TEXTUS_PLATFORM_64 1
#undef  TLONG_FMT
#define TLONG_FMT "%I64d"
#undef  TLONG_FMTx
#define TLONG_FMTx "%I64x"
#undef  TLONG_FMTu
#define TLONG_FMTu "%I64u"
#elif defined(__LLP64__)
#define TEXTUS_LONG long long
#define TEXTUS_PLATFORM_64 1
#undef  TLONG_FMT
#define TLONG_FMT "%lld"
#undef  TLONG_FMTx
#define TLONG_FMTx "%llx"
#undef  TLONG_FMTu
#define TLONG_FMTu "%llu"
#else
// 32-bit machine, Windows or Linux or OS X
#define TEXTUS_LONG long
#define TEXTUS_PLATFORM_32 1
#endif

#if defined(_WIN64)
#  if defined(_M_X64) || defined(_M_AMD64) || defined(_AMD64_)
#    define TEXTUS_LITTLE_ENDIAN 1
#  else
#    error "CPU type is unknown"
#  endif
#elif defined(_WIN32)
#  if defined(_M_IX86)
#    define TEXTUS_LITTLE_ENDIAN 1
#  else
#    error "CPU type is unknown"
#  endif
#elif defined(__APPLE__) || defined(__powerpc__) || defined(__ppc__)
#  if __LITTLE_ENDIAN__
#    define TEXTUS_LITTLE_ENDIAN 1
#  elif __BIG_ENDIAN__
#    define TEXTUS_BIG_ENDIAN 1
#  endif
#elif defined(__GNUC__) && \
      defined(__BYTE_ORDER__) && \
      defined(__ORDER_LITTLE_ENDIAN__) && \
      defined(__ORDER_BIG_ENDIAN__)
   /*
    * Some versions of GCC provide architecture-independent macros for
    * this.  Yes, there are more than two values for __BYTE_ORDER__.
    */
#  if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    define TEXTUS_LITTLE_ENDIAN 1
#  elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#    define TEXTUS_BIG_ENDIAN 1
#  else
#    error "Can't handle mixed-endian architectures"
#  endif
#elif defined(__sparc) || defined(__sparc__) || \
      defined(_POWER) || defined(__hppa) || \
      defined(_MIPSEB) || defined(__ARMEB__) || \
      defined(__s390__) || defined(__AARCH64EB__) || \
      (defined(__sh__) && defined(__LITTLE_ENDIAN__)) || \
      (defined(__ia64) && defined(__BIG_ENDIAN__))
#  define TEXTUS_BIG_ENDIAN 1
#elif defined(__i386) || defined(__i386__) || \
      defined(__x86_64) || defined(__x86_64__) || \
      defined(_MIPSEL) || defined(__ARMEL__) || \
      defined(__alpha__) || defined(__AARCH64EL__) || \
      (defined(__sh__) && defined(__BIG_ENDIAN__)) || \
      (defined(__ia64) && !defined(__BIG_ENDIAN__))
#  define TEXTUS_LITTLE_ENDIAN 1
#endif

#if TEXTUS_BIG_ENDIAN
#  define TEXTUS_LITTLE_ENDIAN 0
#elif TEXTUS_LITTLE_ENDIAN
#  define TEXTUS_BIG_ENDIAN 0
#else
#  error "Cannot determine endianness"
#endif

#if TEXTUS_PLATFORM_64
#  define TEXTUS_PLATFORM_32 0
#elif TEXTUS_PLATFORM_32
#  define TEXTUS_PLATFORM_64 0
#else
#  error "Cannot determine 64/32 bits for platform"
#endif

#if !defined(TEXTUS_AMOR_STORAGE)
#if defined(_WIN32) 
#define TEXTUS_AMOR_STORAGE __declspec(dllimport) 
#else
#define TEXTUS_AMOR_STORAGE
#endif
#endif
#endif
