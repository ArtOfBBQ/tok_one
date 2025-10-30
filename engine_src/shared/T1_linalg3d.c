#include "T1_linalg3d.h"

void T1_linalg3d_float4x4_construct_from_ptr(
    T1float4x4 * to_construct,
    float * vals)
{
    #if defined(__ARM_NEON)
    to_construct->rows[0].neon_f4 = vld1q_f32(vals + 0);
    to_construct->rows[1].neon_f4 = vld1q_f32(vals + 4);
    to_construct->rows[2].neon_f4 = vld1q_f32(vals + 8);
    to_construct->rows[3].neon_f4 = vld1q_f32(vals + 12);
    
    #elif defined(__SSE2__)
    to_construct->rows[0].sse_f4 = _mm_load_ps(vals +  0);
    to_construct->rows[1].sse_f4 = _mm_load_ps(vals +  4);
    to_construct->rows[2].sse_f4 = _mm_load_ps(vals +  8);
    to_construct->rows[3].sse_f4 = _mm_load_ps(vals + 12);
    
    #else
    to_construct->rows[0].data[0] = vals[ 0];
    to_construct->rows[0].data[1] = vals[ 1];
    to_construct->rows[0].data[2] = vals[ 2];
    to_construct->rows[0].data[3] = vals[ 3];
    
    to_construct->rows[1].data[0] = vals[ 4];
    to_construct->rows[1].data[1] = vals[ 5];
    to_construct->rows[1].data[2] = vals[ 6];
    to_construct->rows[1].data[3] = vals[ 7];
    
    to_construct->rows[2].data[0] = vals[ 8];
    to_construct->rows[2].data[1] = vals[ 9];
    to_construct->rows[2].data[2] = vals[10];
    to_construct->rows[2].data[3] = vals[11];
    
    to_construct->rows[3].data[0] = vals[12];
    to_construct->rows[3].data[1] = vals[13];
    to_construct->rows[3].data[2] = vals[14];
    to_construct->rows[3].data[3] = vals[15];
    
    #endif
}

void T1_linalg3d_construct_identity(
    T1float4x4 * to_construct)
{
    T1_linalg3d_float4x4_construct(
        to_construct,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

void T1_linalg3d_float4x4_construct(
    T1float4x4 * to_construct,
    const float row1val1,
    const float row1val2,
    const float row1val3,
    const float row1val4,
    const float row2val1,
    const float row2val2,
    const float row2val3,
    const float row2val4,
    const float row3val1,
    const float row3val2,
    const float row3val3,
    const float row3val4,
    const float row4val1,
    const float row4val2,
    const float row4val3,
    const float row4val4)
{
    #if defined(__ARM_NEON)
    float vals[4];
    
    vals[0] = row1val1;
    vals[1] = row1val2;
    vals[2] = row1val3;
    vals[3] = row1val4;
    to_construct->rows[0].neon_f4 = vld1q_f32(vals);

    vals[0] = row2val1;
    vals[1] = row2val2;
    vals[2] = row2val3;
    vals[3] = row2val4;
    to_construct->rows[1].neon_f4 = vld1q_f32(vals);
    
    vals[0] = row3val1;
    vals[1] = row3val2;
    vals[2] = row3val3;
    vals[3] = row3val4;
    to_construct->rows[2].neon_f4 = vld1q_f32(vals);
    
    vals[0] = row4val1;
    vals[1] = row4val2;
    vals[2] = row4val3;
    vals[3] = row4val4;
    to_construct->rows[3].neon_f4 = vld1q_f32(vals);
    
    #elif defined(__SSE2__)
    float vals[4];
    
    vals[0] = row1val1;
    vals[1] = row1val2;
    vals[2] = row1val3;
    vals[3] = row1val4;
    to_construct->rows[0].sse_f4 = _mm_load_ps(vals);
    
    vals[0] = row2val1;
    vals[1] = row2val2;
    vals[2] = row2val3;
    vals[3] = row2val4;
    to_construct->rows[1].sse_f4 = _mm_load_ps(vals);
    
    vals[0] = row3val1;
    vals[1] = row3val2;
    vals[2] = row3val3;
    vals[3] = row3val4;
    to_construct->rows[2].sse_f4 = _mm_load_ps(vals);
    
    vals[0] = row4val1;
    vals[1] = row4val2;
    vals[2] = row4val3;
    vals[3] = row4val4;
    to_construct->rows[3].sse_f4 = _mm_load_ps(vals);
    
    #else
    to_construct->rows[0].data[0] = row1val1;
    to_construct->rows[0].data[1] = row1val2;
    to_construct->rows[0].data[2] = row1val3;
    to_construct->rows[0].data[3] = row1val4;
    
    to_construct->rows[1].data[0] = row2val1;
    to_construct->rows[1].data[1] = row2val2;
    to_construct->rows[1].data[2] = row2val3;
    to_construct->rows[1].data[3] = row2val4;
    
    to_construct->rows[2].data[0] = row3val1;
    to_construct->rows[2].data[1] = row3val2;
    to_construct->rows[2].data[2] = row3val3;
    to_construct->rows[2].data[3] = row3val4;
    
    to_construct->rows[3].data[0] = row4val1;
    to_construct->rows[3].data[1] = row4val2;
    to_construct->rows[3].data[2] = row4val3;
    to_construct->rows[3].data[3] = row4val4;
    #endif
}

void T1_linalg3d_float4x4_construct_x_rotation(
    T1float4x4 * to_construct,
    const float x_angle)
{
    float cost = cosf(x_angle);
    float sint = sinf(x_angle);
    
    T1_linalg3d_float4x4_construct(
        /* T1float4x4 * to_construct: */
            to_construct,
        /* row 1 vals: */
            1.0f, 0.0f, 0.0f, 0.0f,
        /* row 2 vals: */
            0.0f, cost, -sint, 0.0f,
        /* row 3 vals: */
            0.0f, sint, cost, 0.0f,
        /* row 4 vals: */
            0.0f, 0.0f, 0.0f, 1.0f);
}

void T1_linalg3d_float4x4_construct_y_rotation(
    T1float4x4 * to_construct,
    const float y_angle)
{
    float cost = cosf(y_angle);
    float sint = sinf(y_angle);
    
    T1_linalg3d_float4x4_construct(
        /* T1float4x4 * to_construct: */
            to_construct,
        /* row 1 vals: */
            cost, 0.0f, sint, 0.0f,
        /* row 2 vals: */
            0.0f, 1.0f, 0.0f, 0.0f,
        /* row 3 vals: */
            -sint, 0.0f, cost, 0.0f,
        /* row 4 vals: */
            0.0f, 0.0f, 0.0f, 1.0f);
}

void T1_linalg3d_float4x4_construct_z_rotation(
    T1float4x4 * to_construct,
    const float z_angle)
{
    float cost = cosf(z_angle);
    float sint = sinf(z_angle);
    
    T1_linalg3d_float4x4_construct(
        /* T1float4x4 * to_construct: */
            to_construct,
        /* row 1 vals: */
            cost, -sint, 0.0f, 0.0f,
        /* row 2 vals: */
            sint, cost, 0.0f, 0.0f,
        /* row 3 vals: */
            0.0f, 0.0f, 1.0f, 0.0f,
        /* row 4 vals: */
            0.0f, 0.0f, 0.0f, 1.0f);
}

void T1_linalg3d_float4x4_construct_xyz_rotation(
    T1float4x4 * to_construct,
    const float x_angle,
    const float y_angle,
    const float z_angle)
{
    const float cx = cosf(x_angle), sx = sinf(x_angle);
    const float cy = cosf(y_angle), sy = sinf(y_angle);
    const float cz = cosf(z_angle), sz = sinf(z_angle);
    
    const float rx00 = 1.0f,  rx01 = 0.0f,  rx02 = 0.0f;
    const float rx10 = 0.0f,  rx11 =  cx,   rx12 = -sx;
    const float rx20 = 0.0f,  rx21 =  sx,   rx22 =  cx;
    
    const float ry00 =  cy,   ry01 = 0.0f,  ry02 =  sy;
    const float ry10 = 0.0f,  ry11 = 1.0f,  ry12 = 0.0f;
    const float ry20 = -sy,   ry21 = 0.0f,  ry22 =  cy;
    
    const float rz_ry_00 = cz*ry00 + sz*ry10;
    const float rz_ry_01 = cz*ry01 + sz*ry11;
    const float rz_ry_02 = cz*ry02 + sz*ry12;
    
    const float rz_ry_10 = -sz*ry00 + cz*ry10;
    const float rz_ry_11 = -sz*ry01 + cz*ry11;
    const float rz_ry_12 = -sz*ry02 + cz*ry12;
    
    const float rz_ry_20 = ry20;
    const float rz_ry_21 = ry21;
    const float rz_ry_22 = ry22;
    
    const float r00 = rz_ry_00*rx00 + rz_ry_01*rx10 + rz_ry_02*rx20;
    const float r01 = rz_ry_00*rx01 + rz_ry_01*rx11 + rz_ry_02*rx21;
    const float r02 = rz_ry_00*rx02 + rz_ry_01*rx12 + rz_ry_02*rx22;
    
    const float r10 = rz_ry_10*rx00 + rz_ry_11*rx10 + rz_ry_12*rx20;
    const float r11 = rz_ry_10*rx01 + rz_ry_11*rx11 + rz_ry_12*rx21;
    const float r12 = rz_ry_10*rx02 + rz_ry_11*rx12 + rz_ry_12*rx22;
    
    const float r20 = rz_ry_20*rx00 + rz_ry_21*rx10 + rz_ry_22*rx20;
    const float r21 = rz_ry_20*rx01 + rz_ry_21*rx11 + rz_ry_22*rx21;
    const float r22 = rz_ry_20*rx02 + rz_ry_21*rx12 + rz_ry_22*rx22;
    
    T1_linalg3d_float4x4_construct(to_construct,
        r00,  r01,  r02, 0.0f,
        r10,  r11,  r12, 0.0f,
        r20,  r21,  r22, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

static float T1_linealg3d_float4_dot(
    const T1float4 a,
    const T1float4 b)
{
    #if defined(__ARM_NEON)
        float32x4_t products = vmulq_f32(a.neon_f4, b.neon_f4);
        float32x2_t sum = vadd_f32(
            vget_high_f32(products),
            vget_low_f32(products));
        sum = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
    #elif defined(__SSE2__)
        __m128 mul = _mm_mul_ps(a.sse_f4, b.sse_f4);
        __m128 sum = _mm_add_ps(
            mul,
            _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2,3,0,1)));
        sum = _mm_add_ps(
            sum,
            _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(1,0,2,3)));
        return _mm_cvtss_f32(sum);
    #else
        return
            a.data[0] * b.data[0] +
            a.data[1] * b.data[1] +
            a.data[2] * b.data[2] +
            a.data[3] * b.data[3];
    #endif
}

static T1float4 T1_linalg3d_float4x4_get_column(
    const T1float4x4 * m,
    int col_i)
{
    T1float4 out;
    #if defined(__ARM_NEON)
        float32x4x4_t t = vld4q_f32(&m->rows[0].data[0]);
        out.neon_f4 = t.val[col_i];
    #elif defined(__SSE__)
        __m128 r0 = m->rows[0].sse_f4;
        __m128 r1 = m->rows[1].sse_f4;
        __m128 r2 = m->rows[2].sse_f4;
        __m128 r3 = m->rows[3].sse_f4;
        
        __m128 x, y;
        switch (col_i) {
            case 0:
                x = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(0,0,0,0));
                y = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(0,0,0,0));
            break;
            case 1:
                x = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(1,1,1,1));
                y = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(1,1,1,1));
                break;
            case 2:
                x = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(2,2,2,2));
                y = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(2,2,2,2));
                break;
            case 3:
                x = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(3,3,3,3));
                y = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(3,3,3,3));
                break;
            default: __builtin_unreachable();
        }
        
        out.sse_f4 = _mm_shuffle_ps(x, y, _MM_SHUFFLE(2,0,2,0));
    #else
        out.data[0] = m->rows[0].data[col_i];
        out.data[1] = m->rows[1].data[col_i];
        out.data[2] = m->rows[2].data[col_i];
        out.data[3] = m->rows[3].data[col_i];
    #endif
    
    return out;
}

void T1_linalg3d_float4x4_mul_float4x4(
    const T1float4x4 * a,
    const T1float4x4 * b,
    T1float4x4 * out)
{
    uint32_t row_i;
    for (row_i = 0; row_i < 4; row_i++) {
        for (int32_t col_i = 0; col_i < 4; col_i++)
        {
            out->rows[row_i].data[col_i] = T1_linealg3d_float4_dot(
                a->rows[row_i],
                T1_linalg3d_float4x4_get_column(b, col_i));
        }
    }
}

