#ifndef __GCOROUTINE_MACROS_H__
#define __GCOROUTINE_MACROS_H__

#if !defined(GCOROUTINE_H_INSIDE) && !defined(GCOROUTINE_COMPILATION)
#error "Only gcoroutine.h can be included directly."
#endif

#ifndef _GCOROUTINE_PUBLIC
#define _GCOROUTINE_PUBLIC        extern
#endif

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define _GCOROUTINE_DEPRECATED __attribute__((__deprecated__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#define _GCOROUTINE_DEPRECATED __declspec(deprecated)
#else
#define _GCOROUTINE_DEPRECATED
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define _GCOROUTINE_DEPRECATED_FOR(f) __attribute__((__deprecated__("Use '" #f "' instead")))
#elif defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define _GCOROUTINE_DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define _GCOROUTINE_DEPRECATED_FOR(f) _GCOROUTINE_DEPRECATED
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define _GCOROUTINE_UNAVAILABLE(maj,min) __attribute__((deprecated("Not available before " #maj "." #min)))
#elif defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define _GCOROUTINE_UNAVAILABLE(maj,min) __declspec(deprecated("is not available before " #maj "." #min))
#else
#define _GCOROUTINE_UNAVAILABLE(maj,min) _GCOROUTINE_DEPRECATED
#endif

#endif /* __GCOROUTINE_MACROS_H__ */
