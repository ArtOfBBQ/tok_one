#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <stdint.h>
#include <stddef.h>

#ifndef OBJ_PARSER_NO_ASSERTS
#include <assert.h>
#endif

/*
Reminder: "line elements" are unsupported for now

Records starting with the letter "l" (lowercase L) specify the order of the
vertices which build a polyline.
l v1 v2 v3 v4 v5 v6 ...
*/


/*
*/
typedef struct ParsedObj {
    float (* vertices)[6]; // contains either xyzw,, or xyzrgb (, means unused)
    float (* textures)[2]; // contains u and v
    float (* normals)[3];  // xyz
    
    uint32_t (* triangles)[3]; // 3 indexes into this->vertices
    uint32_t (* quads)[4];
    uint32_t (* triangle_textures)[3]; // 3 indexes into this->textures?
    uint32_t (* quad_textures)[4];
    uint32_t (* triangle_normals)[3]; // 3 indexes into this->normals
    uint32_t (* quad_normals)[4];
    
    uint32_t vertices_count;
    uint32_t textures_count;
    uint32_t normals_count;
    uint32_t triangles_count;
    uint32_t quads_count;
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
void init_obj_parser(
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
    char * raw_buffer,
    uint32_t * success);

/*
-> init_obj_parser() must run before this
-> this is just a convenience function to free all pointers in the struct,
   you don't need to use it if you don't want to
*/
void free_obj(ParsedObj * to_free);

#endif // OBJ_PARSER_H

