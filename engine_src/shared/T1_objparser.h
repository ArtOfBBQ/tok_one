#ifndef T1_OBJPARSER_H
#define T1_OBJPARSER_H

#include "T1_stdint.h"

#include <stdint.h>
#include <stddef.h>

/*
Reminder: "line elements" are unsupported for now

Records starting with the letter "l" (lowercase L) specify the order of the
vertices which build a polyline.
l v1 v2 v3 v4 v5 v6 ...
*/

typedef struct {
    char name[64];
} T1MaterialName;

typedef struct {
    T1MaterialName * material_names;
    f32 (* vertices)[6]; // contains either xyzw,, or xyzrgb (, means unused)
    f32 (* textures_vt_uv)[2]; // 'vt' in .obj files, 'uv' in 3d graphics
    f32 (* normals_vn)[3];  // xyz, 'vn' in .obj files
    
    // vert1, vert2, vert3, smooth, material_i
    u32 (* triangles)[5];
    // vert1, vert2, vert3, vert4, smooth, material_i
    u32 (* quads)[6];
    u32 (* triangle_textures)[3]; // 3 indexes into this->textures?
    u32 (* quad_textures)[4];
    u32 (* triangle_normals)[3]; // 3 indexes into this->normals
    u32 (* quad_normals)[4];
    
    u32 materials_count;
    u32 vertices_count;
    u32 textures_count;
    u32 normals_count;
    u32 triangles_count;
    u32 quads_count;
} T1ParsedObj;

/*
Pass your malloc() and free() functions. The parser doesn't work without
a malloc function, but you can pass NULL as a free() function if you wish.

If you pass NULL as your free function, you can still use the parser but 
free_obj() will no longer work.

Example:
#include <stdlib.h>

init_obj_parser(
    malloc,
    free);
*/
void T1_objparser_init(
    void * (* malloc_function) (u64),
    void   (* optional_free_function)(void *));

/*
-> init_obj_parser() must run before this
-> the recipient doesn't need to be initialized or malloc'd, the pointers
   are allocated by this function
-> raw_buffer must be a null-terminated string containing your .obj file
-> 'success' will be set to 0 on failure, 1 on success
*/
void T1_objparser_parse(
    T1ParsedObj * recipient,
    const char * raw_buffer,
    u8 * success);

/*
-> init_obj_parser() must run before this
-> this is just a convenience function to free all pointers in the struct,
   you don't need to use it if you don't want to
*/
void T1_objparser_deinit(T1ParsedObj * to_free);

#endif // T1_OBJPARSER_H

