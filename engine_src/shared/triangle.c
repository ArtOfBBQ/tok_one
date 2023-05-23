#include "triangle.h"

static float get_magnitude(zVertex input) {
    float sum_squares =
        (input.x * input.x) +
        (input.y * input.y) +
        (input.z * input.z);
    
    // TODO: this square root is a performance bottleneck
    return sqrtf(sum_squares);
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    to_normalize->x /= magnitude;
    to_normalize->y /= magnitude;
    to_normalize->z /= magnitude;
}

zVertex crossproduct_of_zvertices(
    const zVertex * a,
    const zVertex * b)
{
    /*
    cx = aybz − azby
    cy = azbx − axbz
    cz = axby − aybx
    */
    zVertex result;
    
    result.x = (a->y * b->z) - (a->z * b->y);
    result.y = (a->z * b->x) - (a->x * b->z);
    result.z = (a->x * b->y) - (a->y * b->x);
    
    return result;
}

float get_triangle_area(
    zTriangle * subject)
{
    zVertex zero_to_one;
    zero_to_one.x = subject->vertices[1].x - subject->vertices[0].x;
    zero_to_one.y = subject->vertices[1].y - subject->vertices[0].y;
    zero_to_one.z = subject->vertices[1].z - subject->vertices[0].z;
    
    zVertex zero_to_two;
    zero_to_two.x = subject->vertices[2].x - subject->vertices[0].x;
    zero_to_two.y = subject->vertices[2].y - subject->vertices[0].y;
    zero_to_two.z = subject->vertices[2].z - subject->vertices[0].z;
    
    zVertex crossproduct = crossproduct_of_zvertices(
        &zero_to_one,
        &zero_to_two);
    
    float return_value = get_magnitude(crossproduct) * 0.5f;
    
    return return_value;
}
