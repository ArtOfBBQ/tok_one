#ifndef T1_CONDITIONAL_INCLUDES_H
#define T1_CONDITIONAL_INCLUDES_H

#if (T1_LOG_PRINTF_ACTIVE == 1) || \
    (T1_COLLISION_PRINTF_ACTIVE == 1) || \
    (INFLATE_PRINTF_ACTIVE == 1)  || \
    (T1_IMG_PRINTF_ACTIVE == 1)
#include <stdio.h>
#endif

#if (T1_LOG_ASSERTS_ACTIVE == 1) || \
    (!defined(T1_COLLISION_IGNORE_ASSERTS)) || \
    (T1_LOG_ASSERTS_ACTIVE == 1)
#include <assert.h>
#endif

#endif // T1_CONDITIONAL_INCLUDES_H
