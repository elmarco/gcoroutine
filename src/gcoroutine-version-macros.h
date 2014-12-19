#ifndef __GCOROUTINE_VERSION_MACROS_H__
#define __GCOROUTINE_VERSION_MACROS_H__

#if !defined(GCOROUTINE_H_INSIDE) && !defined(GCOROUTINE_COMPILATION)
#error "Only gcoroutine.h can be included directly."
#endif

#include "gcoroutine-version.h"
#include "gcoroutine-macros.h"

/**
 * GCOROUTINE_ENCODE_VERSION:
 * @major: a major version
 * @minor: a minor version
 * @micro: a micro version
 *
 * Encodes the given components into a value that can be used for
 * version checks.
 *
 * Since: 1.0
 */
#define GCOROUTINE_ENCODE_VERSION(major,minor,micro) \
  ((major) << 24 | (minor) << 16 | (micro) << 8)

#define _GCOROUTINE_ENCODE_VERSION(maj,min) \
  ((maj) << 16 | (min) << 8)

#ifdef GCOROUTINE_DISABLE_DEPRECATION_WARNINGS
# define GCOROUTINE_DEPRECATED            _GCOROUTINE_PUBLIC
# define GCOROUTINE_DEPRECATED_FOR(f)     _GCOROUTINE_PUBLIC
# define GCOROUTINE_UNAVAILABLE(maj,min)  _GCOROUTINE_PUBLIC
#else
# define GCOROUTINE_DEPRECATED            _GCOROUTINE_DEPRECATED _GCOROUTINE_PUBLIC
# define GCOROUTINE_DEPRECATED_FOR(f)     _GCOROUTINE_DEPRECATED_FOR(f) _GCOROUTINE_PUBLIC
# define GCOROUTINE_UNAVAILABLE(maj,min)  _GCOROUTINE_UNAVAILABLE(maj,min) _GCOROUTINE_PUBLIC
#endif

/**
 * GCOROUTINE_VERSION:
 *
 * The current version of the library, as encoded through
 * the %GCOROUTINE_ENCODE_VERSION macro. Can be used for version
 * checking, for instance:
 *
 * |[<!-- language="C" -->
 *   #if GCOROUTINE_VERSION >= GCOROUTINE_ENCODE_VERSION (1, 2, 3)
 *     // code that uses API introduced after version 1.2.3
 *   #endif
 * ]|
 *
 * Since: 1.0
 */
#define GCOROUTINE_VERSION                \
  GCOROUTINE_ENCODE_VERSION (GCOROUTINE_MAJOR_VERSION, \
                           GCOROUTINE_MINOR_VERSION, \
                           GCOROUTINE_MICRO_VERSION)

#define GCOROUTINE_CHECK_VERSION(major,minor,micro) \
  ((major) > GCOROUTINE_MAJOR_VERSION || \
   (major) == GCOROUTINE_MAJOR_VERSION && (minor) > GCOROUTINE_MINOR_VERSION || \
   (major) == GCOROUTINE_MAJOR_VERSION && (minor) == GCOROUTINE_MINOR_VERSION && (micro) >= GCOROUTINE_MICRO_VERSION)

/* version defines
 *
 * remember to add new macros at the beginning of each development cycle
 */

#define GCOROUTINE_VERSION_1_0    (_GCOROUTINE_ENCODE_VERSION (1, 0))

/* unconditional */
#define GCOROUTINE_DEPRECATED_IN_1_0              GCOROUTINE_DEPRECATED
#define GCOROUTINE_DEPRECATED_IN_1_0_FOR(f)       GCOROUTINE_DEPRECATED_FOR(f)
#define GCOROUTINE_AVAILABLE_IN_1_0               _GCOROUTINE_PUBLIC

#ifndef GCOROUTINE_VERSION_MIN_REQUIRED
# define GCOROUTINE_VERSION_MIN_REQUIRED  (GCOROUTINE_VERSION_1_0)
#endif


#ifndef GCOROUTINE_VERSION_MAX_ALLOWED
# define GCOROUTINE_VERSION_MAX_ALLOWED  (GCOROUTINE_VERSION_MIN_REQUIRED)
#endif

/* sanity checks */
#if GCOROUTINE_VERSION_MAX_ALLOWED < GCOROUTINE_VERSION_MIN_REQUIRED
# error "GCOROUTINE_VERSION_MAX_ALLOWED must be >= GCOROUTINE_VERSION_MIN_REQUIRED"
#endif
#if GCOROUTINE_VERSION_MIN_REQUIRED < GCOROUTINE_VERSION_1_0
# error "GCOROUTINE_VERSION_MIN_REQUIRED must be >= GCOROUTINE_VERSION_1_0"
#endif

#endif /* __GCOROUTINE_VERSION_MACROS_H__ */
