#ifndef __OGG_COMPAT_H__
#define __OGG_COMPAT_H__

#ifndef __GNUC__

#ifdef WIN32
#define __INLINE__ __inline
#else
#define __INLINE__ __inline__
#endif

#ifndef WIN32
#undef off_t
#else
#include <sys/types.h>
#endif

#endif

#endif /* __OGG_COMPAT_H__ */
