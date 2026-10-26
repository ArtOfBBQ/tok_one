#ifndef T1_OBJC_H
#define T1_OBJC_H

/*
Calls into Objective-C and Apple's frameworks from plain C, without
compiling as Objective-C or linking frameworks at compile time. Classes,
objects and selectors are void *, arguments are passed as uintptr_t: there's
no type safety. Apple's enums are 64-bit, so the uintptr_t results can be
assigned to them directly.
*/

#include "T1_stdint.h"

void T1_objc_init(
    void * (* malloc_perma)(size_t),
    char ** const fatal_error);

/*
The raw objc_msgSend(), to cast to a method's exact signature: only for
methods taking structs by value or many arguments, prefer T1_objc_msg_2arg()
etc., which check their arguments in debug builds. On x86-64, methods that
return structs that don't fit in integer registers need objc_msgSend_stret.

typedef void (* my_fn)(void *, void *, MyStruct);
((my_fn)T1_objc_unsafe_get_raw_msg_send())(object, selector, my_struct);
*/
typedef void (* T1ObjcRawFn)(void);
T1ObjcRawFn T1_objc_unsafe_get_raw_msg_send(void);

b8 T1_objc_open_framework(
    const char * framework_name);

#ifdef NDEBUG
#define T1_objc_autorelease_pool_push(x) T1_objc_autorelease_pool_push_internal()
void * T1_objc_autorelease_pool_push_internal(void);
#else
#define T1_objc_autorelease_pool_push(x) T1_objc_autorelease_pool_push_internal_with_func(__func__)
void * T1_objc_autorelease_pool_push_internal_with_func(
    const char * func);
#endif

#ifdef NDEBUG
#define T1_objc_autorelease_pool_pop(pool) T1_objc_autorelease_pool_pop_internal(pool)
void T1_objc_autorelease_pool_pop_internal(void * pool);
#else
#define T1_objc_autorelease_pool_pop(pool) T1_objc_autorelease_pool_pop_internal_with_func(pool, __func__)
void T1_objc_autorelease_pool_pop_internal_with_func(
    void * pool,
    const char * func);
#endif

void * T1_objc_get_func(
    const char * func_name);

void * T1_objc_get_class(
    const char * class_name);

void * T1_objc_inherit_from_class(
    const char * base_class_name,
    const char * new_class_name);

/*
Objective-C Type Encoding Reference
(64-bit macOS / LP64):

Return & Argument Types:
v  = void
B  = C99 bool / BOOL
c  = int8_t
C  = uint8_t
s  = int16_t
S  = uint16_t
i  = int32_t
I  = uint32_t
q  = int64_t
Q  = uint64_t
f  = float (32-bit IEEE float)
d  = double / CGFloat (64-bit IEEE float)
@  = Object pointer (id, NSWindow *, NSEvent *, etc.)
:  = Selector (SEL)
^v = Raw pointer (void *)

Note: Every method implicitly receives self (@) and _cmd (:) as its first two
parameters.

Example Type Encodings:
"v@:@" <- void return, self (@), _cmd (:), 1 object arg (@) [e.g., keyDown:]
"B@:"  <- BOOL return, self (@), _cmd (:)           [e.g., canBecomeKeyWindow]
"d@:"  <- double/CGFloat return, self (@), _cmd (:) [e.g., button value]
"Q@:"  <- NSUInteger return, self (@), _cmd (:)     [e.g., array count]
*/
void T1_objc_add_method(
    void * to_class,
    const char * method_name,
    void * func_ptr,
    const char * types_magicstr);

void T1_objc_commit_child_class(void * class_ptr);

void * T1_objc_reg_sel(
    const char * selector_name);

uintptr_t T1_objc_msg(
    void * recip,
    void * selector);

uintptr_t T1_objc_msg_1arg(
    void * recip,
    void * selector,
    uintptr_t arg1);

uintptr_t T1_objc_msg_2arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2);

uintptr_t T1_objc_msg_3arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3);

uintptr_t T1_objc_msg_4arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    uintptr_t arg4);

uintptr_t T1_objc_msg_1f64(
    void * recip,
    void * selector,
    f64 arg1);

f64 T1_objc_msg_get_f64(
    void * recip,
    void * selector);

void * T1_objc_nsstring_construct(
    const char * from);

char * T1_objc_nsstring_to_cstring(
    void * nsstring);

#endif // T1_OBJC_H
