/* The generated callback closure uses only GCC's noinline attribute. Keep
   its source unchanged while building it with the live app's MSVC toolchain. */
#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(attributes) __declspec(noinline)
#endif
