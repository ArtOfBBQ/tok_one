#include "T1_objc.h"

#include "T1_std.h"
#include "T1_log.h"

#ifdef NDEBUG
#else
#include "T1_os.h"
#endif

#include <assert.h>
#include <dlfcn.h>

#define T1NSASCIIStringEncoding 1
#define T1NSModalPanelWindowLevel 8

#define T1_OBJC_THREAD_CAP   20
#define T1_OBJC_PUSHPOP_CAP 128

typedef struct {
    u64 pushpop_stack[T1_OBJC_THREAD_CAP][T1_OBJC_PUSHPOP_CAP];
    u64 pushpop_stack_size[T1_OBJC_THREAD_CAP];
    const c8 * (* class_get_name)(void *); // class_getName()
    void * (* class_get_instance_method)(void *, void *);  // class_getInstanceMethod()
    void * (* object_get_class)(void *); // object_getClass()
    void * (* method_get_name)(void *); // method_getName()
    void   (* method_get_return_type)(void *, char *, size_t); // method_getReturnType
    unsigned int (* method_get_number_of_arguments)(void *); // method_getNumberOfArguments()
    void (* method_get_argument_type)(void *, unsigned int, char *, size_t); // method_getArgumentType
    const char * (* selector_get_name)(void *); // sel_getName
    uintptr_t (* msg)(void *, void *);
    uintptr_t (* msg_arg)(void *, void *, uintptr_t);
    uintptr_t (* msg_2arg)(void *, void *, uintptr_t, uintptr_t);
    uintptr_t (* msg_3arg)(void *, void *, uintptr_t, uintptr_t, uintptr_t);
    uintptr_t (* msg_4arg)(void *, void *, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
    uintptr_t (* msg_5arg)(void *, void *, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
    uintptr_t (* msg_1f64)(void *, void *, f64);
    f32    (* msg_get_f32)(void *, void *);
    f64    (* msg_get_f64)(void *, void *);
    void * (* get_class)(const char *);
    void (* register_class_pair)(void *);
    b8 (* class_add_method)(void *, void *, void *, const char *); // class_addMethod
    void * (* allocate_class_pair)(
        void * superclass,
        const char * name,
        size_t extra_bytes);
    void * (* reg_name)(const char *);
    void * class_nsstring;
    void * sel_run;
    void * sel_run_modal; // runModal
    void * sel_set_message_text; // setMessageText:
    void * sel_string_with_utf8_string;
    // void * sel_make_key_and_order_front;
    void * sel_c_string_using_encoding; // cStringUsingEncoding:
    u8 good;
} T1ObjCState;

static T1ObjCState * T1_objc_s = NULL;

void T1_objc_init(
    void * (* malloc_perma)(size_t),
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }
    
    T1_objc_s = malloc_perma(sizeof(T1ObjCState));
    if (!T1_objc_s) {
        *fatal_error = "Failed T1_objc_init() - no memory";
        return;
    }
    T1_std_memset(T1_objc_s, 0, sizeof(T1ObjCState));
    
    void * libobjc = dlopen(
        "/usr/lib/libobjc.A.dylib",
        RTLD_LAZY);
    if (!libobjc) {
        *fatal_error = "Failed to load libobjc.A.dylib";
        return;
    }
    
    void * libfoundation = dlopen(
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        RTLD_LAZY);
    if (!libfoundation) {
        *fatal_error = "Failed to load Foundation.framework";
        return;
    }
    
    T1_objc_s->class_get_name = dlsym(
        libobjc,
        "class_getName");
    if (!T1_objc_s->class_get_name) {
        *fatal_error = "Failed to load class_getName()";
        return;
    }
    
    T1_objc_s->class_get_instance_method = dlsym(
        libobjc,
        "class_getInstanceMethod");
    if (!T1_objc_s->class_get_instance_method) {
        *fatal_error = "Failed to load class_getInstanceMethod()";
        return;
    }
    
    T1_objc_s->object_get_class = dlsym(
        libobjc,
        "object_getClass");
    if (!T1_objc_s->object_get_class) {
        *fatal_error = "Failed to load object_getClass()";
        return;
    }
    
    T1_objc_s->method_get_name = dlsym(
        libobjc,
        "method_getName");
    if (!T1_objc_s->method_get_name) {
        *fatal_error = "Failed to load method_getName()";
        return;
    }
    
    T1_objc_s->method_get_return_type = dlsym(
        libobjc,
        "method_getReturnType");
    if (!T1_objc_s->method_get_return_type) {
        *fatal_error = "Failed to load method_getReturnType()";
        return;
    }
    
    T1_objc_s->method_get_number_of_arguments = dlsym(
        libobjc,
        "method_getNumberOfArguments");
    if (!T1_objc_s->method_get_number_of_arguments) {
        *fatal_error = "Failed to load method_getNumberOfArguments()";
        return;
    }
    
    T1_objc_s->method_get_argument_type = dlsym(
        libobjc,
        "method_getArgumentType");
    if (!T1_objc_s->method_get_argument_type) {
        *fatal_error = "Failed to load method_getArgumentType()";
        return;
    }
    
    T1_objc_s->selector_get_name = dlsym(
        libobjc,
        "sel_getName");
    if (!T1_objc_s->selector_get_name) {
        *fatal_error = "Failed to load sel_getName()";
        return;
    }
    
    T1_objc_s->msg = dlsym(
        libobjc,
        "objc_msgSend");
    if (!T1_objc_s->msg) {
        *fatal_error = "Failed to load objc_msgSend()";
        return;
    }
    
    T1_objc_s->msg_get_f32      = (f32       (*)(void *, void *))T1_objc_s->msg;
    T1_objc_s->msg_get_f64      = (f64       (*)(void *, void *))T1_objc_s->msg;
    T1_objc_s->msg_arg          = (uintptr_t (*)(void *, void *, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_2arg         = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_3arg         = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_4arg         = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t, uintptr_t, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_5arg         = (uintptr_t (*)(void *, void *, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t))T1_objc_s->msg;
    T1_objc_s->msg_1f64         = (uintptr_t (*)(void *, void *, f64))T1_objc_s->msg;
    
    T1_objc_s->reg_name = dlsym(
        libobjc,
        "sel_registerName");
    if (!T1_objc_s->reg_name) {
        *fatal_error = "Failed to load sel_registerName";
        return;
    }
    
    T1_objc_s->get_class = dlsym(
        libobjc,
        "objc_getClass");
    if (!T1_objc_s->get_class) {
        *fatal_error = "Failed to load objc_getClass";
        return;
    }
    
    T1_objc_s->register_class_pair = dlsym(
        libobjc,
        "objc_registerClassPair");
    if (!T1_objc_s->register_class_pair) {
        *fatal_error = "Failed to load objc_registerClassPair";
        return;
    }
    
    T1_objc_s->class_add_method = dlsym(
        libobjc,
        "class_addMethod");
    if (!T1_objc_s->class_add_method) {
        *fatal_error = "Failed to load class_addMethod";
        return;
    }
    
    T1_objc_s->allocate_class_pair = dlsym(
        libobjc,
        "objc_allocateClassPair");
    if (!T1_objc_s->allocate_class_pair) {
        *fatal_error = "Failed to load objc_allocateClassPair";
        return;
    }
    
    T1_objc_s->good = 1;
    
    T1_objc_s->class_nsstring = T1_objc_s->get_class(
        "NSString");
    if (!T1_objc_s->good) {
        *fatal_error = "Failed to load NSString";
        return;
    }
    
    T1_objc_s->sel_string_with_utf8_string = T1_objc_s->reg_name("stringWithUTF8String:");
    T1_objc_s->sel_c_string_using_encoding = T1_objc_s->reg_name("cStringUsingEncoding:");
    if (!T1_objc_s->good) {
        *fatal_error = "Failed to load NSString selectors.";
        return;
    }
    
    T1_objc_s->sel_run = T1_objc_reg_sel("run");
    T1_objc_s->sel_set_message_text = T1_objc_reg_sel(
        "setMessageText:");
    T1_objc_s->sel_run_modal = T1_objc_reg_sel(
        "runModal");
    if (!T1_objc_s->good) {
        *fatal_error = "Failed to load NSAlert selectors.";
        return;
    }
    
    return;
}

T1ObjcRawFn T1_objc_unsafe_get_raw_msg_send(void) {
    return (T1ObjcRawFn)T1_objc_s->msg;
}

b8 T1_objc_open_framework(
    const char * framework_name)
{
    T1_log_assert(T1_objc_s != NULL);
    T1_log_assert(framework_name != NULL);
    
    if (T1_objc_s == NULL) {
        return 0;
    }
    if (!T1_objc_s->good) {
        return 0;
    }
    
    return dlopen(
        framework_name,
        RTLD_LAZY) != 0;
}

#ifdef NDEBUG
#else
static u64 T1_objc_func_name_to_u64(
    const char * func_name)
{
    T1_log_assert(func_name != NULL);
    T1_log_assert(func_name[0] != '\0');
    
    u64 len = T1_std_strlen(func_name);
    
    u64 out = 0;
    
    for (u32 i = 0; i < 8; i++) {
        u64 cur_char = func_name[len - (i % len)];
        out |= (cur_char << (i * 8));
    }
    
    return out;
}
#endif

typedef void * (*objc_ar_push_fn_type)(void);
static objc_ar_push_fn_type objc_ar_push_fn = NULL;

typedef void (*objc_ar_pop_fn_type)(void *);
static objc_ar_pop_fn_type objc_ar_pop_fn = NULL;

void * T1_objc_autorelease_pool_push_internal(void) {
    // objc_autoreleasePoolPush()
    if (!objc_ar_push_fn) {
        objc_ar_push_fn =
            (objc_ar_push_fn_type)dlsym(
                RTLD_DEFAULT,
                "objc_autoreleasePoolPush");
    }
    
    return objc_ar_push_fn();
}

#ifdef NDEBUG
#else
void * T1_objc_autorelease_pool_push_internal_with_func(
    const char * func)
{
    u64 val = T1_objc_func_name_to_u64(func);
    char * dud_error = 0;
    u64 thread_id = T1_os_get_T1_thread_id(&dud_error);
    assert(dud_error == 0);
    assert(thread_id < T1_OBJC_THREAD_CAP);
    
    T1_objc_s->pushpop_stack[thread_id][
        T1_objc_s->pushpop_stack_size[thread_id]] = val;
    T1_objc_s->pushpop_stack_size[thread_id] += 1;
    
    return T1_objc_autorelease_pool_push_internal();
}
#endif

void T1_objc_autorelease_pool_pop_internal(void * pool) {
    if (!objc_ar_pop_fn) {
        objc_ar_pop_fn = (objc_ar_pop_fn_type)dlsym(
            RTLD_DEFAULT,
            "objc_autoreleasePoolPop");
    }
    
    T1_log_assert(pool != 0);
    objc_ar_pop_fn(pool);
}

#ifdef NDEBUG
#else
void T1_objc_autorelease_pool_pop_internal_with_func(
    void * pool,
    const char * func)
{
    u64 val = T1_objc_func_name_to_u64(func);
    char * dud_error = 0;
    u64 thread_id = T1_os_get_T1_thread_id(&dud_error);
    assert(dud_error == 0);
    assert(thread_id < T1_OBJC_THREAD_CAP);
    
    u64 sz = T1_objc_s->pushpop_stack_size[thread_id];
    assert(sz > 0);
    T1_objc_s->pushpop_stack_size[thread_id] -= 1;
    
    u64 prev_val = T1_objc_s->pushpop_stack[thread_id][sz-1];
    
    assert(val == prev_val);
    
    T1_objc_autorelease_pool_pop_internal(pool);
}
#endif

void * T1_objc_get_func(
    const char * func_name)
{
    if (
        !T1_objc_s ||
        !T1_objc_s->good ||
        !T1_objc_s->get_class)
    {
        return NULL;
    }
    
    void * out = dlsym(
        RTLD_DEFAULT,
        func_name);
    
    if (!out) {
        T1_objc_s->good = 0;
    }
    
    return out;
}

void * T1_objc_get_class(
    const char * class_name)
{
    if (
        !T1_objc_s ||
        !T1_objc_s->good ||
        !T1_objc_s->get_class)
    {
        return NULL;
    }
    
    void * out = T1_objc_s->get_class(class_name);
    
    if (!out) {
        T1_objc_s->good = 0;
    }
    
    return out;
}

void * T1_objc_inherit_from_class(
    const char * base_class_name,
    const char * new_class_name)
{
    void * base_class = T1_objc_s->get_class(
        base_class_name);
    
    void * new_class = T1_objc_s->allocate_class_pair(
        base_class,
        new_class_name,
        0);
    
    return new_class;
}

void T1_objc_add_method(
    void * to_class,
    const char * method_name,
    void * func_ptr,
    const char * types_magicstr)
{
    T1_objc_s->class_add_method(
        /* class: */ to_class,
        /* selector: */ T1_objc_s->reg_name(method_name),
        /* ? : */ func_ptr,
        types_magicstr);
}

void T1_objc_commit_child_class(void * class_ptr) {
    if (class_ptr) {
        T1_objc_s->register_class_pair(class_ptr);
    }
}

void * T1_objc_reg_sel(
    const char * selector_name)
{
    if (
        !T1_objc_s ||
        !T1_objc_s->good)
    {
        return NULL;
    }
    
    void * out = T1_objc_s->reg_name(selector_name);
    
    if (!out) {
        T1_objc_s->good = 0;
    }
    
    return out;
}

uintptr_t T1_objc_msg(
    void * recip,
    void * selector)
{
    T1_log_assert(recip != NULL);
    T1_log_assert(selector != NULL);
    
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    uintptr_t out = T1_objc_s->msg(recip, selector);
    
    return out;
}

static u8 T1_objc_value_fits_encoding_in_register(
    uintptr_t value,
    const char * t)
{
    // discard 'qualifier prefixes' like const
    while (*t == 'r' || *t == 'n' || *t == 'N' ||
           *t == 'o' || *t == 'O' || *t == 'R' || *t == 'V')
    {
        t++;
    }
    
    const int64_t signed_value = (int64_t)value;
    
    switch (t[0]) {
        // 64-bit
        case 'Q': case 'q': // NSUInteger / NSInteger (long also encodes as q/Q on LP64)
        case '*':           // char *
        case '#':           // Class
        case ':':           // SEL
        case '^':           // any pointer
        case '@':           // id, or '@?' for a block
            return 1;
        // 32-bit
        case 'i':
            return value <= UINT32_MAX ||
                   (signed_value >= INT32_MIN && signed_value < 0);
        case 'I':
            return value <= UINT32_MAX;
        // 16-bit
        case 's':
            return value <= UINT16_MAX || (signed_value >= INT16_MIN && signed_value < 0);
        case 'S':
            return value <= UINT16_MAX;
        // arm64 BOOL / C99 bool
        case 'B':
            return value <= 1;
        // 8-bit (on x86-64 BOOL is signed char, which encodes as 'c')
        case 'c':
            return value <= UINT8_MAX || (signed_value >= INT8_MIN && signed_value < 0);
        case 'C':
            return value <= UINT8_MAX;
        // 'f', 'd' (wrong register file), '{' structs, '(' unions,
        // '[' arrays, 'b' bitfields, 'v', '?' ...
    }
    
    return 0;
}

static u8 T1_objc_encoding_is_f64(
    const char * t)
{
    // discard 'qualifier prefixes' like const
    while (*t == 'r' || *t == 'n' || *t == 'N' ||
           *t == 'o' || *t == 'O' || *t == 'R' || *t == 'V')
    {
        t++;
    }
    
    // 'd' is double, which on 64-bit Apple platforms is also CGFloat.
    // 'f' (32-bit float) is NOT acceptable: the callee would read a
    // 32-bit value from the register, but you passed a 64-bit double.
    return t[0] == 'd' && t[1] == '\0';
}

static u8 T1_objc_encoding_is_representable_by_uintptr_t_in_register(
    const char * t)
{
    // discard 'qualifier prefixes' like const
    while (*t == 'r' || *t == 'n' || *t == 'N' ||
           *t == 'o' || *t == 'O' || *t == 'R' || *t == 'V')
    {
        t++;
    }
    
    switch (t[0]) {
        case 'Q': case 'q': // 64-bit ints (NSUInteger / NSInteger)
        case '*':           // char *
        case '#':           // Class
        case ':':           // SEL
        case '^':           // any pointer: ^v, ^@, ^{struct=...}
        case 'B':           // BOOL, also 64-bit in register
            return 1;
        case '@':           // id; also '@?' for a block
            return 1;
        default:
            return 0;
    }
}

#if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
static void assert_arg_f64(
    void * recip,
    void * selector,
    u32 args_count,
    u32 arg_i,
    uintptr_t arg)
{
    void * m = T1_objc_s->class_get_instance_method(
        T1_objc_s->object_get_class(recip), selector);
    unsigned int n = T1_objc_s->method_get_number_of_arguments(m);
    T1_log_assert(n == (args_count + 2)); // n includes self and _cmd
    char ret[16];
    T1_objc_s->method_get_return_type(
        m, ret, sizeof(ret));
    
    char t[64];
    T1_std_memset(t, 0, 64);
    T1_objc_s->method_get_argument_type(m, 2 + arg_i, t, sizeof(t));
    
    if (!T1_objc_encoding_is_f64(t))
    {
        char errmsg[128];
        T1_std_memset(errmsg, 0, 128);
        T1_std_strcat_cap(errmsg, 128, "Class ");
        T1_std_strcat_cap(errmsg, 128, T1_objc_s->class_get_name(T1_objc_s->object_get_class(recip)));
        T1_std_strcat_cap(errmsg, 128, " selector ");
        T1_std_strcat_cap(errmsg, 128, T1_objc_s->selector_get_name(selector));
        T1_std_strcat_cap(errmsg, 128, " can't take argument type as f64: ");
        T1_std_strcat_cap(errmsg, 128, t);
        T1_log_dump_and_crash(errmsg);
        T1_log_assert(0);
        return;
    }
    
    return;
}

static void assert_arg_uintptr_t(
    void * recip,
    void * selector,
    u32 args_count,
    u32 arg_i,
    uintptr_t arg)
{
    void * m = T1_objc_s->class_get_instance_method(
        T1_objc_s->object_get_class(recip), selector);
    unsigned int n = T1_objc_s->method_get_number_of_arguments(m);
    T1_log_assert(n == (args_count + 2)); // n includes self and _cmd
    char ret[16];
    T1_objc_s->method_get_return_type(
        m, ret, sizeof(ret));
    
    char t[64];
    T1_std_memset(t, 0, 64);
    T1_objc_s->method_get_argument_type(m, 2 + arg_i, t, sizeof(t));
    
    if (!T1_objc_encoding_is_representable_by_uintptr_t_in_register(t))
    {
        char errmsg[128];
        T1_std_memset(errmsg, 0, 128);
        T1_std_strcat_cap(errmsg, 128, "Class ");
        T1_std_strcat_cap(errmsg, 128, T1_objc_s->class_get_name(T1_objc_s->object_get_class(recip)));
        T1_std_strcat_cap(errmsg, 128, " selector ");
        T1_std_strcat_cap(errmsg, 128, T1_objc_s->selector_get_name(selector));
        T1_std_strcat_cap(errmsg, 128, " can't take argument type as uintptr_t: ");
        T1_std_strcat_cap(errmsg, 128, t);
        T1_log_dump_and_crash(errmsg);
        T1_log_assert(0);
        return;
    }
    
    if (!T1_objc_value_fits_encoding_in_register(arg, t)) {
        char errmsg[128];
        T1_std_memset(errmsg, 0, 128);
        T1_std_strcat_cap(errmsg, 128, "Class ");
        T1_std_strcat_cap(errmsg, 128, T1_objc_s->class_get_name(T1_objc_s->object_get_class(recip)));
        T1_std_strcat_cap(errmsg, 128, " selector ");
        T1_std_strcat_cap(errmsg, 128, T1_objc_s->selector_get_name(selector));
        T1_std_strcat_cap(errmsg, 128, " argument 1's value is out of boudns for: ");
        T1_std_strcat_cap(errmsg, 128, t);
        T1_log_dump_and_crash(errmsg);
        T1_log_assert(0);
    }
    return;
}
#elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

uintptr_t T1_objc_msg_1arg(
    void * recip,
    void * selector,
    uintptr_t arg1)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 1, /* arg_i: */ 0,
        arg1);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_log_assert(recip != NULL);
    T1_log_assert(selector != NULL);
    
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_arg(recip, selector, arg1);
}

uintptr_t T1_objc_msg_2arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 2, /* arg_i: */ 0,
        arg1);
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 2, /* arg_i: */ 1,
        arg2);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_2arg(recip, selector, arg1, arg2);
}

uintptr_t T1_objc_msg_3arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 3, /* arg_i: */ 0,
        arg1);
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 3, /* arg_i: */ 1,
        arg2);
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 3, /* arg_i: */ 2,
        arg3);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_3arg(recip, selector, arg1, arg2, arg3);
}

uintptr_t T1_objc_msg_4arg(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    uintptr_t arg4)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 4, /* arg_i: */ 0,
        arg1);
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 4, /* arg_i: */ 1,
        arg2);
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 4, /* arg_i: */ 2,
        arg3);
    assert_arg_uintptr_t(
        recip, selector,
        /* args_count: */ 4, /* arg_i: */ 3,
        arg4);
    #elif T1_LOG_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    return T1_objc_s->msg_4arg(recip, selector, arg1, arg2, arg3, arg4);
}

uintptr_t T1_objc_msg_1f64(
    void * recip,
    void * selector,
    f64 arg1)
{
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    T1_log_assert(recip != NULL);
    T1_log_assert(selector != NULL);
    
    assert_arg_f64(
        recip, selector,
        /* args_count: */ 1, /* arg_i: */ 0,
        arg1);
    #else
    #error
    #endif
    
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        T1_log_assert(0);
        return 0;
    }
    
    return T1_objc_s->msg_1f64(recip, selector, arg1);
}

f64 T1_objc_msg_get_f64(
    void * recip,
    void * selector)
{
    if (!T1_objc_s || !T1_objc_s->good || !recip || !selector) {
        return 0;
    }
    
    #if T1_LOG_ASSERTS_ACTIVE == T1_ACTIVE
    {
        T1_log_assert(recip != NULL);
        T1_log_assert(selector != NULL);
        
        void * m = T1_objc_s->class_get_instance_method(
            T1_objc_s->object_get_class(recip), selector);
        // use the class-method lookup if recip is a Class
        T1_log_assert(m != NULL);
        
        unsigned n = T1_objc_s->method_get_number_of_arguments(m);
        T1_log_assert(n == 2); // self and _cmd only
        
        char ret[16];
        T1_std_memset(ret, 0, 16);
        T1_objc_s->method_get_return_type(m, ret, sizeof(ret));
        
        if (!T1_objc_encoding_is_f64(ret)) {
            // build errmsg as before: class, selector, "return type is not double: ", ret
            char errmsg[256];
            T1_std_memset(errmsg, 0, 256);
            T1_std_strcat_cap(errmsg, 256, "T1_objc_msg_get_f64() but float not returned by selector: ");
            T1_std_strcat_cap(errmsg, 256, T1_objc_s->selector_get_name(selector));
            T1_log_dump_and_crash(errmsg);
            T1_log_assert(0);
            return 0;
        }
    }
    #endif
    
    f64 out = T1_objc_s->msg_get_f64(recip, selector);
    
    return out;
}

void * T1_objc_nsstring_construct(
    const char * from)
{
    T1_log_assert(T1_objc_s->class_nsstring != 0);
    T1_log_assert(T1_objc_s->sel_string_with_utf8_string != 0);
    T1_log_assert(from != 0);
    T1_log_assert(from[0] != '\0');
    
    void * out = (void *)T1_objc_msg_1arg(
        T1_objc_s->class_nsstring,
        T1_objc_s->sel_string_with_utf8_string,
        (uintptr_t)from);
    
    return out;
}

char * T1_objc_nsstring_to_cstring(
    void * nsstring)
{
    return (char *)T1_objc_msg_1arg(
        nsstring,
        T1_objc_s->sel_c_string_using_encoding,
        /* cStringUsingEncoding: */ T1NSASCIIStringEncoding);
}
