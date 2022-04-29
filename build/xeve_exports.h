
#ifndef XEVE_EXPORT_H
#define XEVE_EXPORT_H

#ifdef XEVE_STATIC_DEFINE
#  define XEVE_EXPORT
#  define XEVE_NO_EXPORT
#else
#  ifndef XEVE_EXPORT
#    ifdef xeveb_dynamic_EXPORTS
        /* We are building this library */
#      define XEVE_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define XEVE_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef XEVE_NO_EXPORT
#    define XEVE_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef XEVE_DEPRECATED
#  define XEVE_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef XEVE_DEPRECATED_EXPORT
#  define XEVE_DEPRECATED_EXPORT XEVE_EXPORT XEVE_DEPRECATED
#endif

#ifndef XEVE_DEPRECATED_NO_EXPORT
#  define XEVE_DEPRECATED_NO_EXPORT XEVE_NO_EXPORT XEVE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef XEVE_NO_DEPRECATED
#    define XEVE_NO_DEPRECATED
#  endif
#endif

#endif /* XEVE_EXPORT_H */
