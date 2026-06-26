#include "T1_tex.h"

#include "T1_log.h"

void T1_tex_set_array_i(
    T1Tex * recip,
    const s16 newval)
{
    // 31 itself is possible but reserved for
    // "write to depth array"
    T1_log_assert(newval <= 31);
    *recip &= 0x07FF;
    *recip |= (newval << 11);
}

void T1_tex_set_slice_i(
    T1Tex * recip,
    const s16 newval)
{
    T1_log_assert(newval <= 2047);
    *recip &= 0xF800;
    *recip |= newval;
}
