#include "T1_objc.h"

#include "T1_std.h"
#include "T1_log.h"

#include <dlfcn.h>

typedef struct {
    uintptr_t (* msg)(void *, void *);
    uintptr_t (* msg_with_arg)(void *, void *, uintptr_t);
    uintptr_t (* msg_with_2arg)(void *, void *, uintptr_t, uintptr_t);
    uintptr_t (* msg_with_3arg)(void *, void *, uintptr_t, uintptr_t, uintptr_t);
    uintptr_t (* msg_with_4arg)(void *, void *, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
    f32    (* msg_f32)(void *, void *);
    void * (* get_class)(const char *);
    void * (* reg_name)(const char *);
    void * class_nsstring;
    void * sel_string_with_utf8_string;
    void * active_framework;
    u8 *   linked_good;
    u8 good;
} T1ObjCState;

static T1ObjCState * T1_objc_s = NULL;

void T1_objc_init(
    void * (* malloc_perma)(size_t))
{
    T1_objc_s = malloc_perma(sizeof(T1ObjCState));
    T1_std_memset(T1_objc_s, 0, sizeof(T1ObjCState));
    
    void * libobjc = dlopen(
        "/usr/lib/libobjc.A.dylib",
        RTLD_LAZY);
    if (!libobjc) { return; }
    
    T1_objc_s->msg = dlsym(
        libobjc,
        "objc_msgSend");
    if (!T1_objc_s->msg) {
        dlclose(libobjc);
        return;
    }
    
    T1_objc_s->msg_f32             = (f32       (*)(void *, void *))T1_objc_s->msg;
    T1_objc_s->msg_with_arg        = (uintptr_t (*)(void *, void *, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_with_2arg       = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_with_3arg       = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_with_4arg       = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t, uintptr_t, uintptr_t))T1_objc_s->msg;
    
    T1_objc_s->reg_name = dlsym(
        libobjc,
        "sel_registerName");
    if (!T1_objc_s->reg_name) {
        return;
    }
    
    T1_objc_s->get_class = dlsym(
        libobjc,
        "objc_getClass");
    if (!T1_objc_s->get_class) {
        return;
    }
    
    T1_objc_s->class_nsstring = T1_objc_s->get_class("NSString");
    T1_objc_s->sel_string_with_utf8_string = T1_objc_s->reg_name("stringWithUTF8String:");
    
    // dlclose(libobjc);
    
    T1_objc_s->good = 1;
}

void T1_objc_open_framework_and_link_perma_good_val(
    const char * framework_name,
    u8 * perma_good_checker)
{
    T1_log_assert(T1_objc_s != NULL);
    T1_log_assert(framework_name != NULL);
    T1_log_assert(perma_good_checker != NULL);
    
    if (perma_good_checker == NULL) {
        return;
    }
    *perma_good_checker = 0;
    
    if (T1_objc_s == NULL) {
        return;
    }
    T1_objc_s->linked_good = perma_good_checker;
    
    if (T1_objc_s->active_framework) {
        dlclose(T1_objc_s->active_framework);
    }
    
    T1_objc_s->active_framework = dlopen(
        framework_name,
        RTLD_LAZY);
    if (!T1_objc_s->active_framework) {
        return;
    }
    
    *T1_objc_s->linked_good = 1;
}

void T1_objc_close_current_framework(void)
{
    if (T1_objc_s->active_framework) {
        dlclose(T1_objc_s->active_framework);
        T1_objc_s->active_framework = NULL;
    }
}

void * T1_objc_autorelease_pool_push(void) {
    // objc_autoreleasePoolPush()
    typedef void * (*push_fn)(void);
    static push_fn fn = NULL;
    if (!fn) fn = (push_fn)dlsym(RTLD_DEFAULT, "objc_autoreleasePoolPush");
    return fn ? fn() : NULL;
}

void T1_objc_autorelease_pool_pop(void * pool) {
    typedef void (*pop_fn)(void *);
    static pop_fn fn = NULL;
    if (!fn) fn = (pop_fn)dlsym(RTLD_DEFAULT, "objc_autoreleasePoolPop");
    if (fn && pool) fn(pool);
}

void * T1_objc_get_func(
    const char * func_name)
{
    if (
        !T1_objc_s ||
        !T1_objc_s->good ||
        !T1_objc_s->active_framework ||
        !T1_objc_s->get_class ||
        !T1_objc_s->linked_good ||
        !*T1_objc_s->linked_good)
    {
        return NULL;
    }
    
    void * out = dlsym(
        T1_objc_s->active_framework,
        func_name);
    
    if (!out) {
        *T1_objc_s->linked_good = 0;
    }
    
    return out;
}

void * T1_objc_get_class(
    const char * class_name)
{
    if (
        !T1_objc_s ||
        !T1_objc_s->good ||
        !T1_objc_s->active_framework ||
        !T1_objc_s->get_class ||
        !T1_objc_s->linked_good ||
        !*T1_objc_s->linked_good)
    {
        return NULL;
    }
    
    void * out = T1_objc_s->get_class(class_name);
    
    if (!out) {
        *T1_objc_s->linked_good = 0;
    }
    
    return out;
}

void * T1_objc_reg_sel(
    const char * selector_name)
{
    if (
        !T1_objc_s ||
        !T1_objc_s->good ||
        !T1_objc_s->active_framework ||
        !T1_objc_s->linked_good ||
        !*T1_objc_s->linked_good)
    {
        return NULL;
    }
    
    void * out = T1_objc_s->reg_name(selector_name);
    
    if (!out) {
        *T1_objc_s->linked_good = 0;
    }
    
    return out;
}

uintptr_t T1_objc_msg(
    void * recip,
    void * selector)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    uintptr_t out = T1_objc_s->msg(recip, selector);
    
    return out;
}

uintptr_t T1_objc_msg_with_1arg(
    void * recip,
    void * selector,
    uintptr_t arg1)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_with_arg(recip, selector, arg1);
}

uintptr_t T1_objc_msg_with_1bigstructarg(
    void * recip,
    void * selector,
    void * struct_16bytesplus_arg)
{
    #if defined(__x86_64__)
    #error "x86-64 ABI not implemented for objc-messaging"
    T1_log_assert(0);
    return NULL;
    #elif defined(__arm64__) || defined(__aarch64__)
    return T1_objc_msg_with_1arg(
        recip,
        selector,
        (uintptr_t)struct_16bytesplus_arg);
    #else
    #error "unexpected CPU architecture"
    T1_log_assert(0);
    return NULL;
    #endif
}

uintptr_t T1_objc_msg_with_2arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_with_2arg(recip, selector, arg1, arg2);
}

uintptr_t T1_objc_msg_with_3arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_with_3arg(recip, selector, arg1, arg2, arg3);
}

uintptr_t T1_objc_msg_with_4arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    uintptr_t arg4)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_with_4arg(recip, selector, arg1, arg2, arg3, arg4);
}

void * T1_objc_msgx2_expect_ptr(
    void * recip,
    void * selector_1,
    void * selector_2)
{
    void * step1 = (void *)T1_objc_msg(
        recip,
        selector_1);
    
    return (void *)T1_objc_msg(
        step1,
        selector_2);
}

T1ObjcSet T1_objc_set_make(
    uintptr_t x,
    uintptr_t y,
    uintptr_t z)
{
    return (T1ObjcSet){x, y, z};
}

T1Objc6Doubles T1_objc_6doubles_make(
    f64 a, f64 b, f64 c, f64 d, f64 e, f64 f)
{
    return (T1Objc6Doubles){a, b, c, d, e, f};
}

void T1_cmd_copy_texture_to_buffer(
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
    uintptr_t dst_bytes_per_image)
{
    // TODO: fix intel ABI
    #if defined(__x86_64__)
    #error
    #else
    // for ARM64 ABI:
    typedef void (*copy_fn)(
        void *,
        void *,
        void *, 
        uintptr_t,
        uintptr_t, 
        T1ObjcSet,
        T1ObjcSet, 
        void *,
        uintptr_t,
        uintptr_t,
        uintptr_t);
    
    ((copy_fn)T1_objc_s->msg)(
        command_encoder,
        sel_copy_to_buf,
        src_texture,
        src_slice,
        src_level,
        src_origin,
        src_size,
        dst_buffer,
        dst_offset,
        dst_bytes_per_row,
        dst_bytes_per_image);
    #endif
}

f32 T1_objc_msg_expect_f32(
    void * recip,
    void * selector)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    f32 out = T1_objc_s->msg_f32(recip, selector);
    
    return out;
}

void * T1_objc_nsstring_construct(
    const char * from)
{
    void * out = (void *)T1_objc_msg_with_1arg(
        T1_objc_s->class_nsstring,
        T1_objc_s->sel_string_with_utf8_string,
        (uintptr_t)from);
    
    return out;
}
