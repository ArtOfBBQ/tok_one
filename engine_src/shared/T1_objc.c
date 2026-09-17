#include "T1_objc.h"

#include "T1_std.h"
#include "T1_log.h"

#include <dlfcn.h>

typedef struct {
    void * (* msg)(void *, void *);
    u32    (* msg_u32)(void *, void *);
    f32    (* msg_f32)(void *, void *);
    u8     (* msg_u8)(void *, void *);
    void * (* get_class)(const char *);
    void * (* reg_name)(const char *);
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
    
    T1_objc_s->msg_u32 = (u32 (*)(void *, void *))T1_objc_s->msg;
    T1_objc_s->msg_f32 = (f32 (*)(void *, void *))T1_objc_s->msg;
    T1_objc_s->msg_u8 = (u8 (*)(void *, void *))T1_objc_s->msg;
    
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

void * T1_objc_msg_expect_ptr(
    void * recip,
    void * selector)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return NULL;
    }
    
    void * out = T1_objc_s->msg(recip, selector);
    
    return out;
}

void * T1_objc_msgx2_expect_ptr(
    void * recip,
    void * selector_1,
    void * selector_2)
{
    void * step1 = T1_objc_msg_expect_ptr(
        recip,
        selector_1);
    
    return T1_objc_msg_expect_ptr(
        step1,
        selector_2);
}

u32 T1_objc_msg_expect_u32(
    void * recip,
    void * selector)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    u32 out = T1_objc_s->msg_u32(recip, selector);
    
    return out;
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
