#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <stddef.h>

#ifndef OBJ_PARSER_IGNORE_ASSERTS
#include <assert.h>
#endif

/*
Reminder: "line elements" are unsupported for now

Records starting with the letter "l" (lowercase L) specify the order of the
vertices which build a polyline.
l v1 v2 v3 v4 v5 v6 ...
*/

typedef struct MaterialName {
    char name[64];
} MaterialName;

typedef struct ParsedObj {
    MaterialName * material_names;
    float (* vertices)[6]; // contains either xyzw,, or xyzrgb (, means unused)
    float (* textures_vt_uv)[2]; // 'vt' in .obj files, 'uv' in 3d graphics
    float (* normals_vn)[3];  // xyz, 'vn' in .obj files
    
    // vert1, vert2, vert3, smooth, material_i
    unsigned int (* triangles)[5];
    // vert1, vert2, vert3, vert4, smooth, material_i
    unsigned int (* quads)[6];
    unsigned int (* triangle_textures)[3]; // 3 indexes into this->textures?
    unsigned int (* quad_textures)[4];
    unsigned int (* triangle_normals)[3]; // 3 indexes into this->normals
    unsigned int (* quad_normals)[4];
    
    unsigned int materials_count;
    unsigned int vertices_count;
    unsigned int textures_count;
    unsigned int normals_count;
    unsigned int triangles_count;
    unsigned int quads_count;
} ParsedObj;

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
void objparser_init(
    void * (* malloc_function)       (size_t),
    void   (* optional_free_function)(void *));

/*
-> init_obj_parser() must run before this
-> the recipient doesn't need to be initialized or malloc'd, the pointers
   are allocated by this function
-> raw_buffer must be a null-terminated string containing your .obj file
-> 'success' will be set to 0 on failure, 1 on success
*/
void parse_obj(
    ParsedObj * recipient,
    const char * raw_buffer,
    unsigned int * success);

/*
-> init_obj_parser() must run before this
-> this is just a convenience function to free all pointers in the struct,
   you don't need to use it if you don't want to
*/
void free_obj(ParsedObj * to_free);

#endif // OBJ_PARSER_H

