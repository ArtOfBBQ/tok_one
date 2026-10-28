#ifndef T1_TOKEN_H
#define T1_TOKEN_H

#include "T1_stdint.h"

/*
A table-driven tokenizer, documented in T1.h (TOKENS). The rules are merged
into one state machine at T1_token_build(), so T1_token_run() costs one table
lookup per byte however many rules there are.
*/

#define T1_TOKEN_UNMATCHED UINT32_MAX
#define T1_TOKEN_END (UINT32_MAX - 1)

u32 T1_token_new(void);
void T1_token_delete(u32 tz);

void T1_token_add_literal(
    u32 tz,
    const c8 * text,
    u32 kind,
    u8 discard);
void T1_token_add_run(
    u32 tz,
    const c8 * first_set,
    const c8 * rest_set,
    u32 kind,
    u8 discard);
void T1_token_add_delimited(
    u32 tz,
    const c8 * start,
    const c8 * stop,
    c8 escape,
    u8 stop_in_token,
    u32 kind,
    u8 discard);
void T1_token_build(u32 tz);

void T1_token_run(
    u32 tz,
    const c8 * text,
    u64 text_size);

u32 T1_token_count(u32 tz);
u32 T1_token_kind(u32 tz, u32 i);
const c8 * T1_token_text(u32 tz, u32 i);
const c8 * T1_token_value(u32 tz, u32 i);
u32 T1_token_size(u32 tz, u32 i);
u32 T1_token_line(u32 tz, u32 i);
b8 T1_token_equals(u32 tz, u32 i, const c8 * str);
void T1_token_copy(u32 tz, u32 i, c8 * recip, u32 recip_cap);
f64 T1_token_parse_f64(u32 tz, u32 i, u8 * good);
s64 T1_token_parse_s64(u32 tz, u32 i, u8 * good);

#endif // T1_TOKEN_H
