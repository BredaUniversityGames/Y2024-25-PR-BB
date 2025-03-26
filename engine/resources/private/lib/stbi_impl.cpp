
// The OPTIMIZE macro causes conflicts when compiling STB Image on WSL
#if not defined(NDEBUG)
#undef __OPTIMIZE__
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
