#ifndef T1_OBJC_H
#define T1_OBJC_H

/*
These are convenience functions to interface with
objective-c and apple frameworks without having to
compile as objective-c or linking any frameworks at
compile time.

Your classes, class instances, and selectors will all
be represented by void *, so you lose all type safety.

When you pass any type of int or pointer as a message,
it's passed as a typeless uintptr_t, so using this
again strips you of all type safety.

Most of the time, you don't need to do anything - all
Apple enums I've encountered are 64-bit, so you can
just assign to them from a uintptr_t return value
directly. Any pointer can also be assigned directly.
(Or you can use an implicit cast to silence compiler
warnings.)

If your return value is something smaller like an
int32_t, it comes in the form of a uintptr_t with some
useless bytes, so make sure you're extracting the
exact bytes you want.

When you pass or receive floats or doubles, they use
the floating point registers, so you have to use
dedicated functions for that. You can't use the
functions that return uintpr_t or that take uintptr_t
as an argument, it won't work. 

During initialization, you don't need to early exit
when something goes wrong or constantly check the
"perma_good_checker" value is 1, you can set up your
entire framework and check if everything worked once
at the end. 

*******************
Example usage:
*******************
static u8 good = false; // track if somehting went wrong
static void * class_mtl_texture_desc = NULL;
static void * sel_new = NULL;

static void sample_initialization_once_only(void) {
    // Try to open the Metal framework at runtime
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/MetalKit.framework/MetalKit",
        &good);
    
    // fetch a class from the framework
    class_mtl_texture_desc = T1_objc_get_class("MTLTextureDescriptor");
    
    // register a "selector" to send as a "message" to "objects"
    sel_new = T1_objc_reg_sel("new");
    
    T1_objc_close_current_framework();
    
    if (!good) {
        // failure path
    }
}

static void sample_messaging_use_repeatedly(void) {
    // this is equivalent to
    // id<MTLTextureDescriptor> a = [MTLTextureDescriptor new];
    void * a =
        (void *)T1_objc_msg(
            class_mtl_texture_desc,
            sel_new);
}
*/

#include "T1_stdint.h"

b8 T1_objc_init(
    void * (* malloc_perma)(size_t),
    char * error_message,
    u32 error_message_cap);

b8 T1_objc_open_framework(
    const char * framework_name);

void * T1_objc_autorelease_pool_push(void);

void T1_objc_autorelease_pool_pop(void * pool);

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

Note: Every method implicitly receives self (@) and _cmd (:) as its first two parameters.

Example Type Encodings:
"v@:@" <- void return, self (@), _cmd (:), 1 object arg (@) [e.g., keyDown:]
"B@:"  <- BOOL return, self (@), _cmd (:)             [e.g., canBecomeKeyWindow]
"d@:"  <- double/CGFloat return, self (@), _cmd (:)   [e.g., button value]
"Q@:"  <- NSUInteger return, self (@), _cmd (:)       [e.g., array count]
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

uintptr_t T1_objc_msg_5arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    uintptr_t arg4,
    uintptr_t arg5);

uintptr_t T1_objc_msg_1f64(
    void * recip,
    void * selector,
    f64 arg1);

uintptr_t T1_objc_msg_1bigstructarg(
    void * recip,
    void * selector,
    void * struct_16bytesplus_arg);

void * T1_objc_msgx2_get_ptr(
    void * recip,
    void * selector_1,
    void * selector_2);

f64 T1_objc_msg_get_f64(
    void * recip,
    void * selector);

void * T1_objc_nsstring_construct(
    const char * from);

char * T1_objc_nsstring_to_cstring(
    void * nsstring);

/*
Passing structs
*/
typedef struct {
    uintptr_t a, b;
} T1ObjcPair;

typedef struct {
    f64 a, b;
} T1ObjcPairf64;

typedef struct {
    uintptr_t a, b, c;
} T1ObjcSet;

typedef struct {
    f64 a, b, c, d;
} T1ObjcQuadf64;

T1ObjcPair T1_objc_pair_construct(
    uintptr_t a,
    uintptr_t b);

T1ObjcSet T1_objc_set_construct(
    uintptr_t a,
    uintptr_t b,
    uintptr_t c);

T1ObjcQuadf64 T1_objc_quadf64_construct(
    f64 a, f64 b, f64 c, f64 d);

typedef struct {
    f64 a, b, c, d, e, f;
} T1ObjcSextf64;

T1ObjcSextf64 T1_objc_sextf64_construct(
    f64 a, f64 b, f64 c, f64 d, f64 e, f64 f);

uintptr_t T1_objc_msg_1quadf64(
    void * target,
    void * selector,
    T1ObjcQuadf64);

uintptr_t T1_objc_msg_1quadf64_1arg(
    void * target,
    void * selector,
    T1ObjcQuadf64 quad_1,
    uintptr_t arg_1);

uintptr_t T1_objc_msg_1quadf64_3arg(
    void * target,
    void * selector,
    T1ObjcQuadf64 quad_1,
    uintptr_t arg_1,
    uintptr_t arg_2,
    uintptr_t arg_3);

uintptr_t T1_objc_msg_1sextf64(
    void * target,
    void * selector,
    T1ObjcSextf64);

uintptr_t T1_objc_msg_2arg_2pair(
    void * target, 
    void * selector, 
    uintptr_t arg1, 
    uintptr_t arg2, 
    T1ObjcPair pair1, 
    T1ObjcPair pair2);

uintptr_t T1_objc_msg_2set(
    void * target,
    void * selector,
    T1ObjcSet a,
    T1ObjcSet b);

uintptr_t T1_objc_msg_3arg_2set_4arg(
    void * target,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    T1ObjcSet set1,
    T1ObjcSet set2,
    uintptr_t arg4,
    uintptr_t arg5,
    uintptr_t arg6,
    uintptr_t arg7);

uintptr_t T1_objc_msg_4arg_1set_3arg_1set(
    void * target,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    uintptr_t arg4,
    T1ObjcSet set1,
    uintptr_t arg5,
    uintptr_t arg6,
    uintptr_t arg7,
    T1ObjcSet set2);

T1ObjcPairf64 T1_objc_msg_get_pairf64(
    void * target,
    void * selector);

T1ObjcQuadf64 T1_objc_msg_get_quadf64(
    void * target,
    void * selector);

void T1_objc_mtl_copy_texture_to_buffer(
    void *    command_encoder,
    void *    sel_copy_to_buf,
    void *    src_texture,
    uintptr_t src_slice,
    uintptr_t src_level,
    T1ObjcSet src_origin,
    T1ObjcSet src_size,
    void *    dst_buffer,
    uintptr_t dst_offset,
    uintptr_t dst_bytes_per_row,
    uintptr_t dst_bytes_per_image);

#endif // T1_OBJC_H
