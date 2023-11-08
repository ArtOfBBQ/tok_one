#include "objmodel.h"

MeshSummary * all_mesh_summaries;
uint32_t all_mesh_summaries_size;

static void construct_mesh_summary(
    MeshSummary * to_construct,
    const int32_t id)
{
    to_construct->resource_name[0]          = '\0';
    to_construct->mesh_id                   = id;
    to_construct->vertices_head_i           = -1; // index in all_mesh_vertices
    to_construct->vertices_size             = 0;
    to_construct->base_width                = 0.0f;
    to_construct->base_height               = 0.0f;
    to_construct->base_depth                = 0.0f;
    to_construct->shattered_vertices_head_i = -1;
    to_construct->shattered_vertices_size   = 0;
    for (uint32_t mat_i = 0; mat_i < MAX_MATERIALS_SIZE; mat_i++) {
        to_construct->material_names[mat_i][0] = '\0';
    }
    to_construct->materials_size = 0;
}

GPULockedVertex * all_mesh_vertices;
uint32_t all_mesh_vertices_size = 0;

typedef struct BufferedNormal {
    float x;
    float y;
    float z;
} BufferedNormal;

#define PARSER_VERTEX_BUFFER_SIZE 64000
static zVertex * parser_vertex_buffer = NULL;
static BufferedNormal * parser_normals_buffer = NULL;
static float * parser_uv_u_buffer = NULL;
static float * parser_uv_v_buffer = NULL;

void init_all_meshes(void) {
    parser_vertex_buffer = (zVertex *)malloc_from_unmanaged(
        sizeof(zVertex) * PARSER_VERTEX_BUFFER_SIZE);
    
    parser_normals_buffer = (BufferedNormal *)malloc_from_unmanaged(
        sizeof(BufferedNormal) * PARSER_VERTEX_BUFFER_SIZE);
    
    parser_uv_u_buffer = (float *)malloc_from_unmanaged(
        sizeof(float) * PARSER_VERTEX_BUFFER_SIZE);
    
    parser_uv_v_buffer = (float *)malloc_from_unmanaged(
        sizeof(float) * PARSER_VERTEX_BUFFER_SIZE);
    
    all_mesh_summaries = (MeshSummary *)malloc_from_unmanaged(
        sizeof(MeshSummary) * ALL_MESHES_SIZE);
    
    for (uint32_t i = 0; i < ALL_MESHES_SIZE; i++) {
        construct_mesh_summary(&all_mesh_summaries[i], (int32_t)i);
    }
    
    assert(ALL_LOCKED_VERTICES_SIZE > 0);
    all_mesh_vertices = (GPULockedVertex *)malloc_from_unmanaged(
        sizeof(GPULockedVertex) * ALL_LOCKED_VERTICES_SIZE);
    
    for (uint32_t i = 0; i < ALL_LOCKED_VERTICES_SIZE; i++) {
        all_mesh_vertices[i].material_i = -1;
    }
    
    // Let's hardcode a basic quad since that's a mesh that will be used by
    // even the features inherent to the engine itself (the terminal, any
    // text labels, the FPS label, etc)
    strcpy_capped(
        all_mesh_summaries[0].resource_name,
        OBJ_STRING_SIZE,
        "basic_quad");
    all_mesh_summaries[0].mesh_id = 0;
    all_mesh_summaries[0].vertices_head_i = 0;
    all_mesh_summaries[0].vertices_size = 6;
    all_mesh_summaries[0].base_width = 2.0f;
    all_mesh_summaries[0].base_height = 2.0f;
    all_mesh_summaries[0].base_depth = 2.0f;
    all_mesh_summaries[0].materials_size = 1;
    all_mesh_summaries[0].shattered_vertices_head_i = -1;
    all_mesh_summaries[0].shattered_vertices_size = 0;
    
    const float left_vertex     = -1.0f;
    const float right_vertex    =  1.0f;
    const float top_vertex      =  1.0f;
    const float bottom_vertex   = -1.0f;
    const float left_uv_coord   = 0.0f;
    const float right_uv_coord  = 1.0f;
    const float bottom_uv_coord = 1.0f;
    const float top_uv_coord    = 0.0f;
    
    // basic quad, triangle 1
    // top left vertex
    all_mesh_vertices[0].xyz[0]                 = left_vertex;
    all_mesh_vertices[0].xyz[1]                 = top_vertex;
    all_mesh_vertices[0].xyz[2]                 = 0.0f;
    all_mesh_vertices[0].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[0].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[0].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[0].uv[0]                  = left_uv_coord;
    all_mesh_vertices[0].uv[1]                  = top_uv_coord;
    all_mesh_vertices[0].material_i      = 0;
    // top right vertex
    all_mesh_vertices[1].xyz[0]                 = right_vertex;
    all_mesh_vertices[1].xyz[1]                 = top_vertex;
    all_mesh_vertices[1].xyz[2]                 = 0.0f;
    all_mesh_vertices[1].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[1].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[1].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[1].uv[0]                  = right_uv_coord;
    all_mesh_vertices[1].uv[1]                  = top_uv_coord;
    all_mesh_vertices[1].material_i      = 0;
    // bottom left vertex
    all_mesh_vertices[2].xyz[0]                 = left_vertex;
    all_mesh_vertices[2].xyz[1]                 = bottom_vertex;
    all_mesh_vertices[2].xyz[2]                 = 0.0f;
    all_mesh_vertices[2].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[2].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[2].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[2].uv[0]             = left_uv_coord;
    all_mesh_vertices[2].uv[1]             = bottom_uv_coord;
    all_mesh_vertices[2].material_i = 0;
    
    // basic quad, triangle 2 
    // top right vertex
    all_mesh_vertices[3].xyz[0]                 = right_vertex;
    all_mesh_vertices[3].xyz[1]                 = top_vertex;
    all_mesh_vertices[3].xyz[2]                 = 0.0f;
    all_mesh_vertices[3].uv[0]             = right_uv_coord;
    all_mesh_vertices[3].uv[1]             = top_uv_coord;
    all_mesh_vertices[3].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[3].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[3].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[3].material_i = 0;
    // bottom right vertex
    all_mesh_vertices[4].xyz[0]                 = right_vertex;
    all_mesh_vertices[4].xyz[1]                 = bottom_vertex;
    all_mesh_vertices[4].xyz[2]                 = 0.0f;
    all_mesh_vertices[4].uv[0]             = right_uv_coord;
    all_mesh_vertices[4].uv[1]             = bottom_uv_coord;
    all_mesh_vertices[4].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[4].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[4].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[4].material_i = 0;
    // bottom left vertex
    all_mesh_vertices[5].xyz[0]                 = left_vertex;
    all_mesh_vertices[5].xyz[1]                 = bottom_vertex;
    all_mesh_vertices[5].xyz[2]                 = 0.0f;
    all_mesh_vertices[5].uv[0]             = left_uv_coord;
    all_mesh_vertices[5].uv[1]             = bottom_uv_coord;
    all_mesh_vertices[5].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[5].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[5].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[5].material_i = 0;
    
    // Let's hardcode a basic cube since that will be used by the particle
    // effects system
    strcpy_capped(
        all_mesh_summaries[1].resource_name,
        OBJ_STRING_SIZE,
        "basic_cube");
    all_mesh_summaries[1].vertices_head_i = 6;
    all_mesh_summaries[1].vertices_size = 36;
    all_mesh_summaries[1].mesh_id = 1;
    all_mesh_summaries[1].materials_size = 1;
    all_mesh_summaries[1].base_width = 1.0f;
    all_mesh_summaries[1].base_height = 1.0f;
    all_mesh_summaries[1].base_depth = 1.0f;
    all_mesh_summaries_size = 2;
    
    const float front_vertex =  -1.0f;
    const float back_vertex  =   1.0f;
    
    // basic cube, front face triangle 1
    all_mesh_vertices[6].xyz[0]                 = left_vertex;
    all_mesh_vertices[6].xyz[1]                 = top_vertex;
    all_mesh_vertices[6].xyz[2]                 = front_vertex;
    all_mesh_vertices[6].uv[0]             = left_uv_coord;
    all_mesh_vertices[6].uv[1]             = top_uv_coord;
    all_mesh_vertices[6].normal_xyz[0]          =  0.0f;
    all_mesh_vertices[6].normal_xyz[1]          =  0.0f;
    all_mesh_vertices[6].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[6].material_i = 0;
    all_mesh_vertices[7].xyz[0]                 = right_vertex;
    all_mesh_vertices[7].xyz[1]                 = top_vertex;
    all_mesh_vertices[7].xyz[2]                 = front_vertex;
    all_mesh_vertices[7].uv[0]             = right_uv_coord;
    all_mesh_vertices[7].uv[1]             = top_uv_coord;
    all_mesh_vertices[7].normal_xyz[0]          =  0.0f;
    all_mesh_vertices[7].normal_xyz[1]          =  0.0f;
    all_mesh_vertices[7].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[7].material_i = 0;
    all_mesh_vertices[8].xyz[0]                 = left_vertex;
    all_mesh_vertices[8].xyz[1]                 = bottom_vertex;
    all_mesh_vertices[8].xyz[2]                 = front_vertex;
    all_mesh_vertices[8].uv[0]             = left_uv_coord;
    all_mesh_vertices[8].uv[1]             = bottom_uv_coord;
    all_mesh_vertices[8].normal_xyz[0]          =  0.0f;
    all_mesh_vertices[8].normal_xyz[1]          =  0.0f;
    all_mesh_vertices[8].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[8].material_i = 0;
    
    // basic cube, front face triangle 2
    all_mesh_vertices[9].xyz[0]                 = right_vertex;
    all_mesh_vertices[9].xyz[1]                 = top_vertex;
    all_mesh_vertices[9].xyz[2]                 = front_vertex;
    all_mesh_vertices[9].uv[0]             = right_uv_coord;
    all_mesh_vertices[9].uv[1]             = top_uv_coord;
    all_mesh_vertices[9].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[9].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[9].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[9].material_i = 0;
    all_mesh_vertices[10].xyz[0]                 = right_vertex;
    all_mesh_vertices[10].xyz[1]                 = bottom_vertex;
    all_mesh_vertices[10].xyz[2]                 = front_vertex;
    all_mesh_vertices[10].uv[0]             = right_uv_coord;
    all_mesh_vertices[10].uv[1]             = bottom_uv_coord;
    all_mesh_vertices[10].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[10].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[10].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[10].material_i = 0;
    all_mesh_vertices[11].xyz[0]                 = left_vertex;
    all_mesh_vertices[11].xyz[1]                 = bottom_vertex;
    all_mesh_vertices[11].xyz[2]                 = front_vertex;
    all_mesh_vertices[11].uv[0]             = left_uv_coord;
    all_mesh_vertices[11].uv[1]             = bottom_uv_coord;
    all_mesh_vertices[11].normal_xyz[0]          = 0.0f;
    all_mesh_vertices[11].normal_xyz[1]          = 0.0f;
    all_mesh_vertices[11].normal_xyz[2]          = -1.0f;
    all_mesh_vertices[11].material_i = 0;
    
    // basic cube, back face triangle 1
    all_mesh_vertices[12].xyz[0]     = left_vertex;
    all_mesh_vertices[12].xyz[1]     = top_vertex;
    all_mesh_vertices[12].xyz[2]     = back_vertex;
    all_mesh_vertices[12].uv[0] = left_uv_coord;
    all_mesh_vertices[12].uv[1] = top_uv_coord;
    all_mesh_vertices[12].normal_xyz[0] = 0.0f;
    all_mesh_vertices[12].normal_xyz[1] = 0.0f;
    all_mesh_vertices[12].normal_xyz[2] = 1.0f;
    all_mesh_vertices[12].material_i = 0;
    all_mesh_vertices[13].xyz[0]     = right_vertex;
    all_mesh_vertices[13].xyz[1]     = top_vertex;
    all_mesh_vertices[13].xyz[2]     = back_vertex;
    all_mesh_vertices[13].uv[0] = right_uv_coord;
    all_mesh_vertices[13].uv[1] = top_uv_coord;
    all_mesh_vertices[13].normal_xyz[0] = 0.0f;
    all_mesh_vertices[13].normal_xyz[1] = 0.0f;
    all_mesh_vertices[13].normal_xyz[2] = 1.0f;
    all_mesh_vertices[13].material_i = 0;
    all_mesh_vertices[14].xyz[0]     = left_vertex;
    all_mesh_vertices[14].xyz[1]     = bottom_vertex;
    all_mesh_vertices[14].xyz[2]     = back_vertex;
    all_mesh_vertices[14].uv[0] = left_uv_coord;
    all_mesh_vertices[14].uv[1] = bottom_uv_coord;
    all_mesh_vertices[14].normal_xyz[0] = 0.0f;
    all_mesh_vertices[14].normal_xyz[1] = 0.0f;
    all_mesh_vertices[14].normal_xyz[2] = 1.0f;
    all_mesh_vertices[14].material_i = 0;
    
    // basic cube, back face triangle 2
    all_mesh_vertices[15].xyz[0] = right_vertex;
    all_mesh_vertices[15].xyz[1] = top_vertex;
    all_mesh_vertices[15].xyz[2] = back_vertex;
    all_mesh_vertices[15].uv[0] = right_uv_coord;
    all_mesh_vertices[15].uv[1] = top_uv_coord;
    all_mesh_vertices[15].normal_xyz[0] = 0.0f;
    all_mesh_vertices[15].normal_xyz[1] = 0.0f;
    all_mesh_vertices[15].normal_xyz[2] = 1.0f;
    all_mesh_vertices[15].material_i = 0;
    all_mesh_vertices[16].xyz[0] = right_vertex;
    all_mesh_vertices[16].xyz[1] = bottom_vertex;
    all_mesh_vertices[16].xyz[2] = back_vertex;
    all_mesh_vertices[16].uv[0] = right_uv_coord;
    all_mesh_vertices[16].uv[1] = bottom_uv_coord;
    all_mesh_vertices[16].normal_xyz[0] = 0.0f;
    all_mesh_vertices[16].normal_xyz[1] = 0.0f;
    all_mesh_vertices[16].normal_xyz[2] = 1.0f;
    all_mesh_vertices[16].material_i = 0;
    all_mesh_vertices[17].xyz[0] = left_vertex;
    all_mesh_vertices[17].xyz[1] = bottom_vertex;
    all_mesh_vertices[17].xyz[2] = back_vertex;
    all_mesh_vertices[17].uv[0] = left_uv_coord;
    all_mesh_vertices[17].uv[1] = bottom_uv_coord;
    all_mesh_vertices[17].normal_xyz[0] = 0.0f;
    all_mesh_vertices[17].normal_xyz[1] = 0.0f;
    all_mesh_vertices[17].normal_xyz[2] = 1.0f;
    all_mesh_vertices[17].material_i = 0;
    
    // basic cube, left face triangle 1
    all_mesh_vertices[18].xyz[0]     = left_vertex;
    all_mesh_vertices[18].xyz[1]     = top_vertex;
    all_mesh_vertices[18].xyz[2]     = back_vertex;
    all_mesh_vertices[18].uv[0] = left_uv_coord;
    all_mesh_vertices[18].uv[1] = top_uv_coord;
    all_mesh_vertices[18].normal_xyz[0] = -1.0f;
    all_mesh_vertices[18].normal_xyz[1] = 0.0f;
    all_mesh_vertices[18].normal_xyz[2] = 0.0f;
    all_mesh_vertices[18].material_i = 0;
    all_mesh_vertices[19].xyz[0]     = left_vertex;
    all_mesh_vertices[19].xyz[1]     = top_vertex;
    all_mesh_vertices[19].xyz[2]     = front_vertex;
    all_mesh_vertices[19].uv[0] = right_uv_coord;
    all_mesh_vertices[19].uv[1] = top_uv_coord;
    all_mesh_vertices[19].normal_xyz[0] = -1.0f;
    all_mesh_vertices[19].normal_xyz[1] = 0.0f;
    all_mesh_vertices[19].normal_xyz[2] = 0.0f;
    all_mesh_vertices[19].material_i = 0;
    all_mesh_vertices[20].xyz[0]     = left_vertex;
    all_mesh_vertices[20].xyz[1]     = bottom_vertex;
    all_mesh_vertices[20].xyz[2]     = back_vertex;
    all_mesh_vertices[20].uv[0] = left_uv_coord;
    all_mesh_vertices[20].uv[1] = bottom_uv_coord;
    all_mesh_vertices[20].normal_xyz[0] = -1.0f;
    all_mesh_vertices[20].normal_xyz[1] = 0.0f;
    all_mesh_vertices[20].normal_xyz[2] = 0.0f;
    all_mesh_vertices[20].material_i = 0;
    
    // basic cube, left face triangle 2
    all_mesh_vertices[21].xyz[0] = left_vertex;
    all_mesh_vertices[21].xyz[1] = top_vertex;
    all_mesh_vertices[21].xyz[2] = front_vertex;
    all_mesh_vertices[21].uv[0] = right_uv_coord;
    all_mesh_vertices[21].uv[1] = top_uv_coord;
    all_mesh_vertices[21].normal_xyz[0] = -1.0f;
    all_mesh_vertices[21].normal_xyz[1] = 0.0f;
    all_mesh_vertices[21].normal_xyz[2] = 0.0f;
    all_mesh_vertices[21].material_i = 0;
    all_mesh_vertices[22].xyz[0] = left_vertex;
    all_mesh_vertices[22].xyz[1] = bottom_vertex;
    all_mesh_vertices[22].xyz[2] = front_vertex;
    all_mesh_vertices[22].uv[0] = right_uv_coord;
    all_mesh_vertices[22].uv[1] = bottom_uv_coord;
    all_mesh_vertices[22].normal_xyz[0] = -1.0f;
    all_mesh_vertices[22].normal_xyz[1] = 0.0f;
    all_mesh_vertices[22].normal_xyz[2] = 0.0f;
    all_mesh_vertices[22].material_i = 0;
    all_mesh_vertices[23].xyz[0] = left_vertex;
    all_mesh_vertices[23].xyz[1] = bottom_vertex;
    all_mesh_vertices[23].xyz[2] = back_vertex;
    all_mesh_vertices[23].uv[0] = left_uv_coord;
    all_mesh_vertices[23].uv[1] = bottom_uv_coord;
    all_mesh_vertices[23].normal_xyz[0] = -1.0f;
    all_mesh_vertices[23].normal_xyz[1] = 0.0f;
    all_mesh_vertices[23].normal_xyz[2] = 0.0f;
    all_mesh_vertices[23].material_i = 0;
    
    // basic cube, right face triangle 1
    all_mesh_vertices[24].xyz[0]     = right_vertex;
    all_mesh_vertices[24].xyz[1]     = top_vertex;
    all_mesh_vertices[24].xyz[2]     = back_vertex;
    all_mesh_vertices[24].uv[0] = left_uv_coord;
    all_mesh_vertices[24].uv[1] = top_uv_coord;
    all_mesh_vertices[24].normal_xyz[0] = 1.0f;
    all_mesh_vertices[24].normal_xyz[1] = 0.0f;
    all_mesh_vertices[24].normal_xyz[2] = 0.0f;
    all_mesh_vertices[24].material_i = 0;
    all_mesh_vertices[25].xyz[0]     = right_vertex;
    all_mesh_vertices[25].xyz[1]     = top_vertex;
    all_mesh_vertices[25].xyz[2]     = front_vertex;
    all_mesh_vertices[25].uv[0] = right_uv_coord;
    all_mesh_vertices[25].uv[1] = top_uv_coord;
    all_mesh_vertices[25].normal_xyz[0] = 1.0f;
    all_mesh_vertices[25].normal_xyz[1] = 0.0f;
    all_mesh_vertices[25].normal_xyz[2] = 0.0f;
    all_mesh_vertices[25].material_i = 0;
    all_mesh_vertices[26].xyz[0]     = right_vertex;
    all_mesh_vertices[26].xyz[1]     = bottom_vertex;
    all_mesh_vertices[26].xyz[2]     = back_vertex;
    all_mesh_vertices[26].uv[0] = left_uv_coord;
    all_mesh_vertices[26].uv[1] = bottom_uv_coord;
    all_mesh_vertices[26].normal_xyz[0] = 1.0f;
    all_mesh_vertices[26].normal_xyz[1] = 0.0f;
    all_mesh_vertices[26].normal_xyz[2] = 0.0f;
    all_mesh_vertices[26].material_i = 0;
    
    // basic cube, right face triangle 2
    all_mesh_vertices[27].xyz[0] = right_vertex;
    all_mesh_vertices[27].xyz[1] = top_vertex;
    all_mesh_vertices[27].xyz[2] = front_vertex;
    all_mesh_vertices[27].uv[0] = right_uv_coord;
    all_mesh_vertices[27].uv[1] = top_uv_coord;
    all_mesh_vertices[27].normal_xyz[0] = 1.0f;
    all_mesh_vertices[27].normal_xyz[1] = 0.0f;
    all_mesh_vertices[27].normal_xyz[2] = 0.0f;
    all_mesh_vertices[27].material_i = 0;
    all_mesh_vertices[28].xyz[0] = right_vertex;
    all_mesh_vertices[28].xyz[1] = bottom_vertex;
    all_mesh_vertices[28].xyz[2] = front_vertex;
    all_mesh_vertices[28].uv[0] = right_uv_coord;
    all_mesh_vertices[28].uv[1] = bottom_uv_coord;
    all_mesh_vertices[28].normal_xyz[0] = 1.0f;
    all_mesh_vertices[28].normal_xyz[1] = 0.0f;
    all_mesh_vertices[28].normal_xyz[2] = 0.0f;
    all_mesh_vertices[28].material_i = 0;
    all_mesh_vertices[29].xyz[0] = right_vertex;
    all_mesh_vertices[29].xyz[1] = bottom_vertex;
    all_mesh_vertices[29].xyz[2] = back_vertex;
    all_mesh_vertices[29].uv[0] = left_uv_coord;
    all_mesh_vertices[29].uv[1] = bottom_uv_coord;
    all_mesh_vertices[29].normal_xyz[0] = 1.0f;
    all_mesh_vertices[29].normal_xyz[1] = 0.0f;
    all_mesh_vertices[29].normal_xyz[2] = 0.0f;
    all_mesh_vertices[29].material_i = 0;
    
    // basic cube, top face triangle 1
    all_mesh_vertices[30].xyz[0]     = left_vertex;
    all_mesh_vertices[30].xyz[1]     = top_vertex;
    all_mesh_vertices[30].xyz[2]     = back_vertex;
    all_mesh_vertices[30].uv[0] = left_uv_coord;
    all_mesh_vertices[30].uv[1] = top_uv_coord;
    all_mesh_vertices[30].normal_xyz[0] = 0.0f;
    all_mesh_vertices[30].normal_xyz[1] = 1.0f;
    all_mesh_vertices[30].normal_xyz[2] = 0.0f;
    all_mesh_vertices[30].material_i = 0;
    all_mesh_vertices[31].xyz[0]     = right_vertex;
    all_mesh_vertices[31].xyz[1]     = top_vertex;
    all_mesh_vertices[31].xyz[2]     = back_vertex;
    all_mesh_vertices[31].uv[0] = right_uv_coord;
    all_mesh_vertices[31].uv[1] = top_uv_coord;
    all_mesh_vertices[31].normal_xyz[0] = 0.0f;
    all_mesh_vertices[31].normal_xyz[1] = 1.0f;
    all_mesh_vertices[31].normal_xyz[2] = 0.0f;
    all_mesh_vertices[31].material_i = 0;
    all_mesh_vertices[32].xyz[0]     = left_vertex;
    all_mesh_vertices[32].xyz[1]     = top_vertex;
    all_mesh_vertices[32].xyz[2]     = front_vertex;
    all_mesh_vertices[32].uv[0] = left_uv_coord;
    all_mesh_vertices[32].uv[1] = bottom_uv_coord;
    all_mesh_vertices[32].normal_xyz[0] = 0.0f;
    all_mesh_vertices[32].normal_xyz[1] = 1.0f;
    all_mesh_vertices[32].normal_xyz[2] = 0.0f;
    all_mesh_vertices[32].material_i = 0;
    
    // basic cube, top face triangle 2
    all_mesh_vertices[33].xyz[0] = right_vertex;
    all_mesh_vertices[33].xyz[1] = top_vertex;
    all_mesh_vertices[33].xyz[2] = back_vertex;
    all_mesh_vertices[33].uv[0] = right_uv_coord;
    all_mesh_vertices[33].uv[1] = top_uv_coord;
    all_mesh_vertices[33].normal_xyz[0] = 0.0f;
    all_mesh_vertices[33].normal_xyz[1] = 1.0f;
    all_mesh_vertices[33].normal_xyz[2] = 0.0f;
    all_mesh_vertices[33].material_i = 0;
    all_mesh_vertices[34].xyz[0] = right_vertex;
    all_mesh_vertices[34].xyz[1] = top_vertex;
    all_mesh_vertices[34].xyz[2] = front_vertex;
    all_mesh_vertices[34].uv[0] = right_uv_coord;
    all_mesh_vertices[34].uv[1] = bottom_uv_coord;
    all_mesh_vertices[34].normal_xyz[0] = 0.0f;
    all_mesh_vertices[34].normal_xyz[1] = 1.0f;
    all_mesh_vertices[34].normal_xyz[2] = 0.0f;
    all_mesh_vertices[34].material_i = 0;
    all_mesh_vertices[35].xyz[0] = left_vertex;
    all_mesh_vertices[35].xyz[1] = top_vertex;
    all_mesh_vertices[35].xyz[2] = front_vertex;
    all_mesh_vertices[35].uv[0] = left_uv_coord;
    all_mesh_vertices[35].uv[1] = bottom_uv_coord;
    all_mesh_vertices[35].normal_xyz[0] = 0.0f;
    all_mesh_vertices[35].normal_xyz[1] = 1.0f;
    all_mesh_vertices[35].normal_xyz[2] = 0.0f;
    all_mesh_vertices[35].material_i = 0;
    
    // basic cube, bottom face triangle 1
    all_mesh_vertices[36].xyz[0]     = left_vertex;
    all_mesh_vertices[36].xyz[1]     = bottom_vertex;
    all_mesh_vertices[36].xyz[2]     = back_vertex;
    all_mesh_vertices[36].uv[0] = left_uv_coord;
    all_mesh_vertices[36].uv[1] = top_uv_coord;
    all_mesh_vertices[36].normal_xyz[0] = 0.0f;
    all_mesh_vertices[36].normal_xyz[1] = -1.0f;
    all_mesh_vertices[36].normal_xyz[2] = 0.0f;
    all_mesh_vertices[36].material_i = 0;
    all_mesh_vertices[37].xyz[0]     = right_vertex;
    all_mesh_vertices[37].xyz[1]     = bottom_vertex;
    all_mesh_vertices[37].xyz[2]     = back_vertex;
    all_mesh_vertices[37].uv[0] = right_uv_coord;
    all_mesh_vertices[37].uv[1] = top_uv_coord;
    all_mesh_vertices[37].normal_xyz[0] = 0.0f;
    all_mesh_vertices[37].normal_xyz[1] = -1.0f;
    all_mesh_vertices[37].normal_xyz[2] = 0.0f;
    all_mesh_vertices[37].material_i = 0;
    all_mesh_vertices[38].xyz[0]     = left_vertex;
    all_mesh_vertices[38].xyz[1]     = bottom_vertex;
    all_mesh_vertices[38].xyz[2]     = front_vertex;
    all_mesh_vertices[38].uv[0] = left_uv_coord;
    all_mesh_vertices[38].uv[1] = bottom_uv_coord;
    all_mesh_vertices[38].normal_xyz[0] = 0.0f;
    all_mesh_vertices[38].normal_xyz[1] = -1.0f;
    all_mesh_vertices[38].normal_xyz[2] = 0.0f;
    all_mesh_vertices[38].material_i = 0;
    
    // basic cube, bottom face triangle 2
    all_mesh_vertices[39].xyz[0] = right_vertex;
    all_mesh_vertices[39].xyz[1] = bottom_vertex;
    all_mesh_vertices[39].xyz[2] = back_vertex;
    all_mesh_vertices[39].uv[0] = right_uv_coord;
    all_mesh_vertices[39].uv[1] = top_uv_coord;
    all_mesh_vertices[39].normal_xyz[0] = 0.0f;
    all_mesh_vertices[39].normal_xyz[1] = -1.0f;
    all_mesh_vertices[39].normal_xyz[2] = 0.0f;
    all_mesh_vertices[39].material_i = 0;
    all_mesh_vertices[40].xyz[0] = right_vertex;
    all_mesh_vertices[40].xyz[1] = bottom_vertex;
    all_mesh_vertices[40].xyz[2] = front_vertex;
    all_mesh_vertices[40].uv[0] = right_uv_coord;
    all_mesh_vertices[40].uv[1] = bottom_uv_coord;
    all_mesh_vertices[40].normal_xyz[0] = 0.0f;
    all_mesh_vertices[40].normal_xyz[1] = -1.0f;
    all_mesh_vertices[40].normal_xyz[2] = 0.0f;
    all_mesh_vertices[40].material_i = 0;
    all_mesh_vertices[41].xyz[0] = left_vertex;
    all_mesh_vertices[41].xyz[1] = bottom_vertex;
    all_mesh_vertices[41].xyz[2] = front_vertex;
    all_mesh_vertices[41].uv[0] = left_uv_coord;
    all_mesh_vertices[41].uv[1] = bottom_uv_coord;
    all_mesh_vertices[41].normal_xyz[0] = 0.0f;
    all_mesh_vertices[41].normal_xyz[1] = -1.0f;
    all_mesh_vertices[41].normal_xyz[2] = 0.0f;
    all_mesh_vertices[41].material_i = 0;
    all_mesh_vertices_size = 42;
}

static void assert_objmodel_validity(int32_t mesh_id) {
    log_assert(mesh_id >= 0);
    log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
    log_assert(all_mesh_summaries[mesh_id].vertices_head_i >= 0);
    log_assert(
        all_mesh_summaries[mesh_id].vertices_size < ALL_LOCKED_VERTICES_SIZE);
    int32_t all_vertices_tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
        all_mesh_summaries[mesh_id].vertices_size;
    log_assert(all_vertices_tail_i <= (int32_t)all_mesh_vertices_size);
    
    // get all materials mentioned in any triangle
    int32_t materials_mentioned[MAX_MATERIALS_SIZE];
    uint32_t materials_mentioned_size = 0;
    for (
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        vert_i < all_vertices_tail_i;
        vert_i++)
    {
        int32_t new_mat_id = all_mesh_vertices[vert_i].material_i;
        
        bool32_t already_in = false;
        for (uint32_t i = 0; i < materials_mentioned_size; i++) {
            if (materials_mentioned[i] == new_mat_id) {
                already_in = true;
                break;
            }
        }
        
        if (!already_in) {
            log_assert(materials_mentioned_size + 1 < MAX_MATERIALS_SIZE);
            assert(materials_mentioned_size + 1 < MAX_MATERIALS_SIZE);
            materials_mentioned[materials_mentioned_size++] = new_mat_id;
            log_assert(
                new_mat_id <
                    (int32_t)all_mesh_summaries[mesh_id].materials_size);
        }
    }
    
    // assert each mentioned material is in the summary
    for (
        uint32_t mentioned_i = 0;
        mentioned_i < materials_mentioned_size;
        mentioned_i++)
    {
        log_assert(mentioned_i >= 0);
        log_assert(mentioned_i < all_mesh_summaries[mesh_id].materials_size);
    }
}

static void guess_ztriangle_normal(zTriangle * input) {
    zVertex vector1;
    zVertex vector2;
    
    vector1.x = input->vertices[1].x - input->vertices[0].x;
    vector1.y = input->vertices[1].y - input->vertices[0].y;
    vector1.z = input->vertices[1].z - input->vertices[0].z;
    
    vector2.x = input->vertices[2].x - input->vertices[0].x;
    vector2.y = input->vertices[2].y - input->vertices[0].y;
    vector2.z = input->vertices[2].z - input->vertices[0].z;
    
    input->normal.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
    input->normal.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
    input->normal.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);
}

static float get_vertex_magnitude(float input_xyz[3]) {
    float x = (input_xyz[0] * input_xyz[0]);
    float y = (input_xyz[1] * input_xyz[1]);
    float z = (input_xyz[2] * input_xyz[2]);
    
    float sum_squares = x + y + z;
    
    sum_squares = isnan(sum_squares) || !isfinite(sum_squares) ?
        FLOAT32_MAX : sum_squares;
    
    float return_value = sqrtf(sum_squares);
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

static void normalize_gpu_triangle_normals(GPULockedVertex * input) {
    float magnitude = get_vertex_magnitude(input->xyz);

    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    log_assert(!isnan(input->xyz[0]));
    input->xyz[0] /= magnitude;
    log_assert(!isnan(input->xyz[0]));
    
    log_assert(!isnan(input->xyz[1]));
    input->xyz[1] /= magnitude;
    log_assert(!isnan(input->xyz[1]));
    
    log_assert(!isnan(input->xyz[2]));
    input->xyz[2] /= magnitude;
    log_assert(!isnan(input->xyz[2]));
}

static void guess_gpu_triangle_normal(GPULockedVertex * input) {
    float vec1_x = input[1].xyz[0] - input[0].xyz[0];
    float vec1_y = input[1].xyz[1] - input[0].xyz[1];
    float vec1_z = input[1].xyz[2] - input[0].xyz[2];
    
    float vec2_x = input[2].xyz[0] - input[0].xyz[0];
    float vec2_y = input[2].xyz[1] - input[0].xyz[1];
    float vec2_z = input[2].xyz[2] - input[0].xyz[2];
    
    input[0].normal_xyz[0] = (vec1_y * vec2_z) - (vec1_z * vec2_y);
    input[0].normal_xyz[1] = (vec1_z * vec2_x) - (vec1_x * vec2_z);
    input[0].normal_xyz[2] = (vec1_x * vec2_y) - (vec1_y * vec2_x);
    input[1].normal_xyz[0] = input[0].normal_xyz[0];
    input[1].normal_xyz[1] = input[0].normal_xyz[1];
    input[1].normal_xyz[2] = input[0].normal_xyz[2];
    input[2].normal_xyz[0] = input[0].normal_xyz[0];
    input[2].normal_xyz[1] = input[0].normal_xyz[1];
    input[2].normal_xyz[2] = input[0].normal_xyz[2];
}

static void populate_new_triangle_with_parser_buffers(
    GPULockedVertex * triangle_recipient,
    int32_t vertex_i_0,
    int32_t vertex_i_1,
    int32_t vertex_i_2,
    int32_t uv_coord_i_0,
    int32_t uv_coord_i_1,
    int32_t uv_coord_i_2,
    int32_t normals_i_0,
    int32_t normals_i_1,
    int32_t normals_i_2,
    int32_t using_material_1,
    int32_t using_material_2,
    int32_t using_material_3)
{
    log_assert(vertex_i_0 != vertex_i_1);
    log_assert(vertex_i_0 != vertex_i_2);
    log_assert(vertex_i_0 > 0);
    log_assert(vertex_i_1 > 0);
    log_assert(vertex_i_2 > 0);
    log_assert(uv_coord_i_0 < PARSER_VERTEX_BUFFER_SIZE);
    log_assert(uv_coord_i_1 < PARSER_VERTEX_BUFFER_SIZE);
    log_assert(uv_coord_i_2 < PARSER_VERTEX_BUFFER_SIZE);
    log_assert(normals_i_0 < PARSER_VERTEX_BUFFER_SIZE);
    log_assert(normals_i_1 < PARSER_VERTEX_BUFFER_SIZE);
    log_assert(normals_i_2 < PARSER_VERTEX_BUFFER_SIZE);
    
    uint32_t target_vertex_0 = 0;
    uint32_t target_vertex_1 = 1;
    uint32_t target_vertex_2 = 2;
    
    triangle_recipient[target_vertex_0].xyz[0] =
        parser_vertex_buffer[vertex_i_0 - 1].x;
    triangle_recipient[target_vertex_0].xyz[1] =
        parser_vertex_buffer[vertex_i_0 - 1].y;
    triangle_recipient[target_vertex_0].xyz[2] =
        parser_vertex_buffer[vertex_i_0 - 1].z;
    triangle_recipient[target_vertex_1].xyz[0] =
        parser_vertex_buffer[vertex_i_1 - 1].x;
    triangle_recipient[target_vertex_1].xyz[1] =
        parser_vertex_buffer[vertex_i_1 - 1].y;
    triangle_recipient[target_vertex_1].xyz[2] =
        parser_vertex_buffer[vertex_i_1 - 1].z;
    triangle_recipient[target_vertex_2].xyz[0] =
        parser_vertex_buffer[vertex_i_2 - 1].x;
    triangle_recipient[target_vertex_2].xyz[1] =
        parser_vertex_buffer[vertex_i_2 - 1].y;
    triangle_recipient[target_vertex_2].xyz[2] =
        parser_vertex_buffer[vertex_i_2 - 1].z;
    
    if (
        uv_coord_i_0 > 0 &&
        uv_coord_i_1 > 0 &&
        uv_coord_i_2 > 0)
    {
        triangle_recipient[target_vertex_0].uv[0] =
            parser_uv_u_buffer[uv_coord_i_0 - 1];
        triangle_recipient[target_vertex_0].uv[1] =
            parser_uv_v_buffer[uv_coord_i_0 - 1];
        triangle_recipient[target_vertex_1].uv[0] =
            parser_uv_u_buffer[uv_coord_i_1 - 1];
        triangle_recipient[target_vertex_1].uv[1] =
            parser_uv_v_buffer[uv_coord_i_1 - 1];
        triangle_recipient[target_vertex_2].uv[0] =
            parser_uv_u_buffer[uv_coord_i_2 - 1];
        triangle_recipient[target_vertex_2].uv[1] =
            parser_uv_v_buffer[uv_coord_i_2 - 1];
    } else {
        triangle_recipient[target_vertex_0].uv[0] =
            1.0f;
        triangle_recipient[target_vertex_0].uv[1] =
            0.0f;
        triangle_recipient[target_vertex_1].uv[0] =
            1.0f;
        triangle_recipient[target_vertex_1].uv[1] =
            1.0f;
        triangle_recipient[target_vertex_2].uv[0] =
            0.0f;
        triangle_recipient[target_vertex_2].uv[1] =
            1.0f;
    }
    
    if (
        normals_i_0 > 0 &&
        normals_i_1 > 0 &&
        normals_i_2 > 0)
    {
        triangle_recipient[target_vertex_0].normal_xyz[0] =
            parser_normals_buffer[normals_i_0 - 1].x;
        triangle_recipient[target_vertex_0].normal_xyz[1] =
            parser_normals_buffer[normals_i_0 - 1].y;
        triangle_recipient[target_vertex_0].normal_xyz[2] =
            parser_normals_buffer[normals_i_0 - 1].z;
        triangle_recipient[target_vertex_1].normal_xyz[0] =
            parser_normals_buffer[normals_i_1 - 1].x;
        triangle_recipient[target_vertex_1].normal_xyz[1] =
            parser_normals_buffer[normals_i_1 - 1].y;
        triangle_recipient[target_vertex_1].normal_xyz[2] =
            parser_normals_buffer[normals_i_1 - 1].z;
        triangle_recipient[target_vertex_2].normal_xyz[0] =
            parser_normals_buffer[normals_i_2 - 1].x;
        triangle_recipient[target_vertex_2].normal_xyz[1] =
            parser_normals_buffer[normals_i_2 - 1].y;
        triangle_recipient[target_vertex_2].normal_xyz[2] =
            parser_normals_buffer[normals_i_2 - 1].z;
    } else {
        guess_gpu_triangle_normal(triangle_recipient);
    }
    
    triangle_recipient[0].material_i = using_material_1;
    triangle_recipient[1].material_i = using_material_2;
    triangle_recipient[2].material_i = using_material_3;
    
    normalize_gpu_triangle_normals(triangle_recipient);
}

static uint32_t chars_till_next_space_or_slash(
    const char * buffer)
{
    uint32_t i = 0;
    
    while (
        buffer[i] != '\n' &&
        buffer[i] != ' ' &&
        buffer[i] != '/' &&
        buffer[i] != '\r')
    {
        i++;
    }
    
    return i;
}

static uint32_t chars_till_next_nonspace(
    const char * buffer)
{
    uint32_t i = 0;

    while (buffer[i] == ' ') {
        i++;
    }
    
    return i;
}

static void parse_obj(
    const char * rawdata,
    const uint64_t rawdata_size,
    MeshSummary * summary_recipient,
    GPULockedVertex * triangles_recipient,
    uint32_t * triangles_recipient_size)
{
    log_assert(rawdata != NULL);
    log_assert(rawdata_size > 0);
    
    uint32_t i = 0;
    uint32_t first_material_or_face_i = UINT32_MAX;
    
    uint32_t next_verrtex_i = 0;
    uint32_t next_uv_i = 0;
    uint32_t next_normal_i = 0;
    
    // first pass
    while (i < rawdata_size) {
        // read the 1st character, which denominates the type
        // of information
        
        char dbg_newline[30];
        strcpy_capped(dbg_newline, 30, rawdata + i);
        uint32_t dbg_i = 0;
        while (dbg_i < 29 && dbg_newline[dbg_i] != '\n') {
            dbg_i++;
        }
        dbg_newline[dbg_i] = '\0';
        
        if (
            rawdata[i] == 'v' &&
            rawdata[i+1] == ' ')
        {
            // discard the 'v'
            i++;
            
            // read vertex data
            zVertex new_vertex;
            
            // skip the space(s) after the 'v'
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read vertex x
            new_vertex.x = string_to_float(rawdata + i);
            
            // discard vertex x
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            
            // discard the spaces after vertex x
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read vertex y
            new_vertex.y = string_to_float(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read vertex z
            new_vertex.z = string_to_float(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == '\n' || rawdata[i] == '\r');
            i++;
            
            parser_vertex_buffer[next_verrtex_i] = new_vertex;
            log_assert(
                parser_vertex_buffer[next_verrtex_i].x == new_vertex.x);
            log_assert(
                parser_vertex_buffer[next_verrtex_i].y
                    == new_vertex.y);
            log_assert(
                parser_vertex_buffer[next_verrtex_i].z
                    == new_vertex.z);
            next_verrtex_i++;
        } else if (
            rawdata[i] == 'v'
            && rawdata[i+1] == 't')
        {
            // discard the 'vt'
            i += 2;
            
            // skip the space(s) after the 'vt'
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read the u coordinate
            parser_uv_u_buffer[next_uv_i] = string_to_float(rawdata + i);
            if (parser_uv_u_buffer[next_uv_i] < 0.0f) {
                parser_uv_u_buffer[next_uv_i] = 0.0f;
            }
            if (parser_uv_u_buffer[next_uv_i] > 1.0f) {
                parser_uv_u_buffer[next_uv_i] = 1.0f;
            }
            
            // discard the u coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            
            // skip the space(s) after the u coord
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read the v coordinate
            parser_uv_v_buffer[next_uv_i] = string_to_float(rawdata + i);
            if (parser_uv_v_buffer[next_uv_i] < 0.0f) {
                parser_uv_v_buffer[next_uv_i] = 0.0f;
            }
            if (parser_uv_v_buffer[next_uv_i] > 1.0f) {
                parser_uv_v_buffer[next_uv_i] = 1.0f;
            }
            
            next_uv_i += 1;
            
            // discard the v coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            
            // there may be a 3rd 'w' entry in a 'vt' line, skip if so
            if (rawdata[i] == ' ') {
                i++;
                i += chars_till_next_space_or_slash(rawdata + i);
            }
            
            log_assert(rawdata[i] == '\n' || rawdata[i] == '\r');
            
            // discard the line break
            while (rawdata[i] == '\n' || rawdata[i] == '\r') {
                i++;
            }
            
        } else if (
            rawdata[i] == 'v'
            && rawdata[i+1] == 'n')
        {
            // discard the 'vn'
            i += 2;
            
            // skip the space(s) after the 'vt'
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            parser_normals_buffer[next_normal_i].x =
                string_to_float(rawdata + i);
            
            // discard the normal x
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
            
            // skip the space(s) after the normal x
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read the normal y
            parser_normals_buffer[next_normal_i].y =
                string_to_float(rawdata + i);
            
            // discard the normal y
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == ' ');
                
            // skip the space(s) after the normal y
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            // read the normal z
            parser_normals_buffer[next_normal_i].z =
                string_to_float(rawdata + i);
            
            // discard the normal z
            i += chars_till_next_space_or_slash(
                rawdata + i);
            log_assert(rawdata[i] == '\n' || rawdata[i] == '\r');
            // discard the line break
            while (rawdata[i] == '\n' || rawdata[i] == '\r') {
                i++;
            }
            
            next_normal_i += 1;
        } else {
            if (
                rawdata[i] == 'f' ||
                (
                    rawdata[i] == 'u' &&
                    rawdata[i+1] == 's' &&
                    rawdata[i+2] == 'e' &&
                    rawdata[i+3] == 'm'))
            {
                if (i < first_material_or_face_i) {
                    first_material_or_face_i = i;
                }
            }
            
            if (rawdata[i] == 'f') {
                *triangles_recipient_size += 1;
            }
            // skip until the next line break character
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }
            
            // skip the line break character
            while (rawdata[i] == '\n' || rawdata[i] == '\r') {
                i++;
            }
        }
    }
    
    log_assert(*triangles_recipient_size > 0);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    if (*triangles_recipient_size >= ALL_LOCKED_VERTICES_SIZE) {
        char error_msg[100];
        strcpy_capped(error_msg, 100, "Error: ALL_LOCKED_VERTICES_SIZE was ");
        strcat_uint_capped(error_msg, 100, ALL_LOCKED_VERTICES_SIZE);
        strcat_capped(error_msg, 100, ", but recipient->triangles_size is ");
        strcat_uint_capped(error_msg, 100, *triangles_recipient_size);
        log_dump_and_crash(error_msg);
        assert(0);
    }
    #endif
    
    // second pass starts at material or face specifications
    i = first_material_or_face_i;
    uint32_t new_triangle_i = 0;
    int32_t using_material_i = 0;
    
    while (i < rawdata_size) {
        if (
            rawdata[i+0] == 'u' &&
            rawdata[i+1] == 's' &&
            rawdata[i+2] == 'e' &&
            rawdata[i+3] == 'm' &&
            rawdata[i+4] == 't' &&
            rawdata[i+5] == 'l' &&
            rawdata[i+6] == ' ')
        {
            uint32_t j = i + 7;
            
            char material_name[OBJ_STRING_SIZE];
            uint32_t material_name_size = 0;
            while (
                rawdata[j] != '\0' &&
                rawdata[j] != ' ' &&
                rawdata[j] != '\n' &&
                rawdata[j] != '\r')
            {
                material_name[material_name_size++] = rawdata[j++];
            }
            material_name[material_name_size] = '\0';
            
            bool32_t already_existed = false;
            
            for (
                int32_t mat_i = 0;
                mat_i < (int32_t)summary_recipient->materials_size;
                mat_i++)
            {
                if (
                    are_equal_strings(
                        summary_recipient->material_names[mat_i],
                        material_name))
                {
                    already_existed = true;
                    using_material_i = mat_i;
                    break;
                }
            }
            
            if (!already_existed) {
                strcpy_capped(
                    summary_recipient->material_names[
                        summary_recipient->materials_size],
                    OBJ_STRING_SIZE,
                    material_name);
                summary_recipient->materials_size += 1;
                using_material_i =
                    (int32_t)summary_recipient->materials_size - 1;
            }
            
            // skip until the next line break character
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }
            // skip the line break character
            while (rawdata[i] == '\n' || rawdata[i] == '\r') {
                i++;
            }
        } else if (rawdata[i] == 'f') {
            // discard the 'f'
            i++;
            log_assert(rawdata[i] == ' ');

            // skip the space(s) after the 'f'
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            int32_t vertex_i_0 = string_to_int32(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            int32_t uv_coord_i_0 = -1;
            int32_t normals_i_0 = 0;
            
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                
                // could be another slash, sometimes there is no uv coordinate
                if (rawdata[i] != '/') {
                    // must be a uv coord then
                    uv_coord_i_0 = string_to_int32(rawdata + i);
                    i += chars_till_next_space_or_slash(rawdata + i);
                }
            }
            // add index to normal if any
            if (rawdata[i] == '/') {
                i++;
                normals_i_0 = string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(rawdata + i);
            }
            
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            int32_t vertex_i_1 = string_to_int32(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            int32_t uv_coord_i_1 = -1;
            int32_t normals_i_1 = 0;
            
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                
                // could be another slash, sometimes there is no uv coordinate
                if (rawdata[i] != '/') {
                    // must be a uv coord then
                    uv_coord_i_1 =
                        string_to_int32(rawdata + i);
                    i += chars_till_next_space_or_slash(
                        rawdata + i);
                }
            }
            // add index to normal if any
            if (rawdata[i] == '/') {
                i++;
                log_assert(rawdata[i] != ' ');
                normals_i_1 = string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(rawdata + i);
            }
            
            log_assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            log_assert(rawdata[i] != ' ');
            
            int32_t vertex_i_2 = string_to_int32(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            int32_t uv_coord_i_2 = -1;
            int32_t normals_i_2 = 0;
            
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                
                // could be another slash, sometimes there is no uv coordinate
                if (rawdata[i] != '/') {
                    // must be a uv coord then
                    uv_coord_i_2 =
                        string_to_int32(rawdata + i);
                    i += chars_till_next_space_or_slash(
                        rawdata + i);
                }
            }
            
            // add index to normal if any
            if (rawdata[i] == '/') {
                i++;
                log_assert(rawdata[i] != ' ');
                normals_i_2 = string_to_int32(rawdata + i);
                while (rawdata[i] <= '9' && rawdata[i] >= '0') {
                    i++;
                }
            }
            
            while (rawdata[i] == ' ') { i++; }
            
            if (rawdata[i] != '\n' && rawdata[i] != '\r') {
                // int32_t vertex_i_3 = string_to_int32(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
                int32_t uv_coord_i_3 = -1;
                // int32_t normals_i_3 = 0;
                if (rawdata[i] == '/')
                {
                    // skip the slash
                    i++;
                    
                    // could be another slash, sometimes there is no uv coordinate
                    if (rawdata[i] != '/') {
                        // must be a uv coord then
                        uv_coord_i_3 =
                            string_to_int32(rawdata + i);
                        i += chars_till_next_space_or_slash(
                            rawdata + i);
                    }
                }
                
                // add normals
                if (rawdata[i] == '/') {
                    i++;
                    // normals_i_3 = string_to_int32(rawdata + i);
                    while (rawdata[i] <= '9' && rawdata[i] >= '0') {
                        i++;
                    }
                }
                
                // there were 2 triangles in this face
                // the 1st triangle will be added anyway later, but
                // we do need to add the extra 2nd triangle here
                GPULockedVertex new_triangle[3];
                
                log_assert(vertex_i_0 != vertex_i_1);
                log_assert(vertex_i_0 != vertex_i_2);
                log_assert(vertex_i_0 > 0);
                log_assert(vertex_i_1 > 0);
                log_assert(vertex_i_2 > 0);
                log_assert(uv_coord_i_0 < PARSER_VERTEX_BUFFER_SIZE);
                log_assert(uv_coord_i_1 < PARSER_VERTEX_BUFFER_SIZE);
                log_assert(uv_coord_i_2 < PARSER_VERTEX_BUFFER_SIZE);
                log_assert(normals_i_0 < PARSER_VERTEX_BUFFER_SIZE);
                log_assert(normals_i_1 < PARSER_VERTEX_BUFFER_SIZE);
                log_assert(normals_i_2 < PARSER_VERTEX_BUFFER_SIZE);
                
                populate_new_triangle_with_parser_buffers(
                    /* GPULockedVertex * triangle_recipient: */
                        new_triangle,
                    /* int32_t vertex_i_0: */
                        vertex_i_0,
                    /* int32_t vertex_i_1: */
                        vertex_i_1,
                    /* int32_t vertex_i_2: */
                        vertex_i_2,
                    /* int32_t uv_coord_i_0: */
                        uv_coord_i_0,
                    /* int32_t uv_coord_i_1: */
                        uv_coord_i_1,
                    /* int32_t uv_coord_i_2: */
                        uv_coord_i_2,
                    /* int32_t normals_i_0: */
                        normals_i_0,
                    /* int32_t normals_i_1: */
                        normals_i_1,
                    /* int32_t normals_i_2: */
                        normals_i_2,
                    /* int32_t using_material_1: */
                        using_material_i,
                    /* int32_t using_material_2: */
                        using_material_i,
                    /* int32_t using_material_3: */
                        using_material_i);
                
                *triangles_recipient_size += 1;
                log_assert(new_triangle_i < ALL_LOCKED_VERTICES_SIZE);
                
                triangles_recipient[(new_triangle_i * 3) + 0] = new_triangle[0];
                triangles_recipient[(new_triangle_i * 3) + 1] = new_triangle[1];
                triangles_recipient[(new_triangle_i * 3) + 2] = new_triangle[2];
                
                new_triangle_i++;
                summary_recipient->vertices_size += 3;
            } else {
                // there was only 1 triangle
            }

            // if you get here there was only 1 triangle OR
            // there were 2 triangles and you already did the other one
            GPULockedVertex new_triangle[3];
            
            populate_new_triangle_with_parser_buffers(
                /* GPULockedVertex * triangle_recipient: */
                    new_triangle,
                /* int32_t vertex_i_0: */
                    vertex_i_0,
                /* int32_t vertex_i_1: */
                    vertex_i_1,
                /* int32_t vertex_i_2: */
                    vertex_i_2,
                /* int32_t uv_coord_i_0: */
                    uv_coord_i_0,
                /* int32_t uv_coord_i_1: */
                    uv_coord_i_1,
                /* int32_t uv_coord_i_2: */
                    uv_coord_i_2,
                /* int32_t normals_i_0: */
                    normals_i_0,
                /* int32_t normals_i_1: */
                    normals_i_1,
                /* int32_t normals_i_2: */
                    normals_i_2,
                /* int32_t using_material_1: */
                    using_material_i,
                /* int32_t using_material_2: */
                    using_material_i,
                /* int32_t using_material_3: */
                    using_material_i);
            
            triangles_recipient[(new_triangle_i * 3) + 0] = new_triangle[0];
            triangles_recipient[(new_triangle_i * 3) + 1] = new_triangle[1];
            triangles_recipient[(new_triangle_i * 3) + 2] = new_triangle[2];
            
            new_triangle_i++;
            summary_recipient->vertices_size += 3;
            
            // some objs have trailing spaces here
            while (rawdata[i] == ' ') { i++; }
            
            log_assert(rawdata[i] == '\n' || rawdata[i] == '\r');
            i++;

        } else {
            // skip until the next line break character
            while (rawdata[i] != '\n' && rawdata[i] != '\0') {
                i++;
            }

            // skip the line break character
            while (rawdata[i] == '\n' || rawdata[i] == '\r') {
                i++;
            }
        }
    }
    
    free_from_managed((uint8_t *)parser_vertex_buffer);
}


int32_t new_mesh_id_from_resource(
    const char * filename)
{
    int32_t new_mesh_head_id = (int32_t)all_mesh_vertices_size;
    log_assert(all_mesh_summaries_size < ALL_MESHES_SIZE);
    
    FileBuffer obj_file;
    
    obj_file.size = platform_get_resource_size(filename);
    log_assert(obj_file.size > 0);
    obj_file.contents = (char *)malloc_from_managed(obj_file.size);
    obj_file.good = false;
    
    platform_read_resource_file(
        /* char * filename: */
            filename,
        /* FileBuffer *out_preallocatedbuffer: */
            &obj_file);
    
    log_assert(obj_file.good);
    
    all_mesh_summaries[all_mesh_summaries_size].vertices_head_i =
        new_mesh_head_id;
    all_mesh_summaries[all_mesh_summaries_size].mesh_id =
        (int32_t)all_mesh_summaries_size;
    
    parse_obj(
        /* const char * rawdata: */
            obj_file.contents,
        /* const uint64_t rawdata_size: */
            obj_file.size,
        /* MeshSummary * summary_recipient: */
            all_mesh_summaries + all_mesh_summaries_size,
        /* zTriangle * triangles_recipient: */
            all_mesh_vertices + new_mesh_head_id,
        /* uint32_t * triangles_recipient_size: */
            &all_mesh_vertices_size);
    
    log_assert((int32_t)all_mesh_vertices_size > new_mesh_head_id);
    
    all_mesh_summaries[all_mesh_summaries_size].vertices_size =
        (int32_t)all_mesh_vertices_size - new_mesh_head_id;
    
    // fetch base width/height/depth and store
    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;
    
    for (
        int32_t tri_i = new_mesh_head_id;
        tri_i < (int32_t)all_mesh_vertices_size;
        tri_i += 3)
    {
        for (uint32_t m = 0; m < 3; m++) {
            if (min_x > all_mesh_vertices[tri_i + m].xyz[0]) {
                min_x = all_mesh_vertices[tri_i + m].xyz[0];
            }
            if (min_y > all_mesh_vertices[tri_i + m].xyz[1]) {
                min_y = all_mesh_vertices[tri_i + m].xyz[1];
            }
            if (min_z > all_mesh_vertices[tri_i + m].xyz[2]) {
                min_z = all_mesh_vertices[tri_i + m].xyz[2];
            }
            if (max_x < all_mesh_vertices[tri_i + m].xyz[0]) {
                max_x = all_mesh_vertices[tri_i + m].xyz[0];
            }
            if (max_y < all_mesh_vertices[tri_i + m].xyz[1]) {
                max_y = all_mesh_vertices[tri_i + m].xyz[1];
            }
            if (max_z < all_mesh_vertices[tri_i + m].xyz[2]) {
                max_z = all_mesh_vertices[tri_i + m].xyz[2];
            }
        }
    }
    
    log_assert(max_x >= min_x);
    log_assert(max_y >= min_z);
    log_assert(max_z >= min_z);
    all_mesh_summaries[all_mesh_summaries_size].base_width =
        max_x - min_x;
    all_mesh_summaries[all_mesh_summaries_size].base_height =
        max_y - min_y;
    all_mesh_summaries[all_mesh_summaries_size].base_depth =
        max_z - min_z;
    
    strcpy_capped(
        all_mesh_summaries[all_mesh_summaries_size].resource_name,
        OBJ_STRING_SIZE,
        filename);
    all_mesh_summaries_size += 1;
    log_assert(all_mesh_summaries_size <= ALL_MESHES_SIZE);
    
    assert_objmodel_validity((int32_t)all_mesh_summaries_size - 1);
    
    return (int32_t)all_mesh_summaries_size - 1;
}

void center_mesh_offsets(
    const int32_t mesh_id)
{
    log_assert(mesh_id < (int32_t)all_mesh_summaries_size);
    
    float smallest_y = FLOAT32_MAX;
    float largest_y = FLOAT32_MIN;
    float smallest_x = FLOAT32_MAX;
    float largest_x = FLOAT32_MIN;
    float smallest_z = FLOAT32_MAX;
    float largest_z = FLOAT32_MIN;
    
    int32_t tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
            all_mesh_summaries[mesh_id].vertices_size;
    
    for (
        int32_t tri_i = all_mesh_summaries[mesh_id].vertices_head_i;
        tri_i < tail_i;
        tri_i += 3)
    {
        for (uint32_t m = 0; m < 3; m++) {
            if (smallest_x > all_mesh_vertices[tri_i + m].xyz[0]) {
                smallest_x = all_mesh_vertices[tri_i + m].xyz[0];
            }
            if (largest_x < all_mesh_vertices[tri_i + m].xyz[0]) {
                largest_x = all_mesh_vertices[tri_i + m].xyz[0];
            }
            if (smallest_y > all_mesh_vertices[tri_i + m].xyz[1]) {
                smallest_y = all_mesh_vertices[tri_i + m].xyz[1];
            }
            if (largest_y < all_mesh_vertices[tri_i + m].xyz[1]) {
                largest_y = all_mesh_vertices[tri_i + m].xyz[1];
            }
            if (smallest_z > all_mesh_vertices[tri_i + m].xyz[2]) {
                smallest_z = all_mesh_vertices[tri_i + m].xyz[2];
            }
            if (largest_z < all_mesh_vertices[tri_i + m].xyz[2]) {
                largest_z = all_mesh_vertices[tri_i + m].xyz[2];
            }
        }
    }
    
    // if smallest x is -6 and largest x is -2, we want to apply +4 to
    // everything then smallest x will be -2 and largest 2
    
    // if smallest x is 2 and largest x is 6, we want to apply -4 to
    // everything then smallest x will be -2 and largest 2
    float x_delta = (smallest_x + largest_x) / 2.0f;
    float y_delta = (smallest_y + largest_y) / 2.0f;
    float z_delta = (smallest_z + largest_z) / 2.0f;
    
    #ifndef LOGGER_IGNORE_ASSERTS
    float new_smallest_x = smallest_x - x_delta;
    float new_largest_x = largest_x - x_delta;
    log_assert(new_smallest_x + new_largest_x == 0.0f);
    #endif
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        all_mesh_vertices[vert_i].xyz[0] -= x_delta;
        all_mesh_vertices[vert_i].xyz[1] -= y_delta;
        all_mesh_vertices[vert_i].xyz[2] -= z_delta;
    }
}

void create_shattered_version_of_mesh(
    const int32_t mesh_id,
    const uint32_t triangles_multiplier)
{
    // TODO: re-implement after the gpu buffer refactoring
    #if 0
    if (triangles_multiplier == 1) {
        all_mesh_summaries[mesh_id].shattered_vertices_size =
            all_mesh_summaries[mesh_id].vertices_size;
        all_mesh_summaries[mesh_id].shattered_vertices_head_i =
            all_mesh_summaries[mesh_id].vertices_head_i;
        return;
    }
    
    int32_t orig_head_i =
        all_mesh_summaries[mesh_id].vertices_head_i;
    #ifndef LOGGER_IGNORE_ASSERTS
    int32_t orig_tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
        all_mesh_summaries[mesh_id].vertices_size;
    #endif
    int32_t orig_vertices_size =
        all_mesh_summaries[mesh_id].vertices_size;
    
    int32_t new_head_i = (int32_t)all_mesh_vertices_size;
    
    all_mesh_summaries[mesh_id].shattered_vertices_head_i = new_head_i;
    all_mesh_summaries[mesh_id].shattered_vertices_size =
        all_mesh_summaries[mesh_id].vertices_size *
            (int32_t)triangles_multiplier;
    
    int32_t goal_new_tail_i =
        (int32_t)all_mesh_vertices_size +
        (int32_t)all_mesh_summaries[mesh_id].shattered_vertices_size;
    
    // first, copy all of the original triangle vertices as they are
    int32_t temp_new_tail_i = new_head_i + orig_vertices_size - 1;
    for (int32_t i = 0; i < orig_vertices_size; i++) {
        log_assert(orig_head_i + i <= orig_tail_i);
        all_mesh_vertices[new_head_i + i] =
            all_mesh_vertices[orig_head_i + i];
        
        #ifndef LOGGER_IGNORE_ASSERTS
        log_assert(new_head_i + i <= temp_new_tail_i);
        
        float tri_length = get_squared_triangle_length(
            &all_mesh_vertices[new_head_i + i]);
        log_assert(tri_length > 0);
        #endif
    }
    
    while (temp_new_tail_i <= goal_new_tail_i) {
        
        // find the biggest triangle to split in 2
        float biggest_area = FLOAT32_MIN;
        int32_t biggest_area_i = -1;
        
        for (int32_t i = new_head_i; i <= temp_new_tail_i; i++) {
            float area =
                get_squared_triangle_length(&all_mesh_vertices[i]);
            if (area > biggest_area) {
                biggest_area = area;
                biggest_area_i = i;
            }
        }
        
        log_assert(biggest_area >= 0);
        log_assert(biggest_area_i >= 0);
        
        // split the triangle at biggest_area_i into 2
        zTriangle first_tri;
        zTriangle second_tri;
        
        int32_t midline_start_vx_i = 0;
        int32_t midline_end_vx_i = 1;
        
        #define USE_MIDLINE -1
        int32_t first_new_triangle_vertices[3];
        int32_t second_new_triangle_vertices[3];
        
        float distance_0_to_1 =
            get_squared_distance(
                all_mesh_vertices[biggest_area_i + 0],
                all_mesh_vertices[biggest_area_i + 1]);
        float distance_1_to_2 =
            get_squared_distance(
                all_mesh_vertices[biggest_area_i + 1],
                all_mesh_vertices[biggest_area_i + 2]);
        float distance_2_to_0 =
            get_squared_distance(
                all_mesh_vertices[biggest_area_i + 2],
                all_mesh_vertices[biggest_area_i + 0]);
        
        log_assert(distance_0_to_1 > 0.0f);
        log_assert(distance_1_to_2 > 0.0f);
        log_assert(distance_2_to_0 > 0.0f);
        
        if (
            distance_0_to_1 > distance_1_to_2 &&
            distance_0_to_1 > distance_2_to_0)
        {
            /*
            Our triangle's with vertices 0,1, and 2, with 0-1 being the
            biggest line and 'M' splitting that line in the middle
            0    M     1
            ...........
            .      .
            .   .
            . .
            .
            2
            */
            midline_start_vx_i = 0;
            midline_end_vx_i = 1;
            
            // first new triangle will be 0-M-2
            first_new_triangle_vertices[0] = 0;
            first_new_triangle_vertices[1] = USE_MIDLINE;
            first_new_triangle_vertices[2] = 2;
            
            // and the second triangle will be M-1-2
            second_new_triangle_vertices[0] = USE_MIDLINE;
            second_new_triangle_vertices[1] = 1;
            second_new_triangle_vertices[2] = 2;
        } else if (
            distance_1_to_2 > distance_2_to_0 &&
            distance_1_to_2 > distance_0_to_1)
        {
            /*
            0          1
            ...........
            .      .
            .   .
            . .   M
            .
            2
            */
            
            midline_start_vx_i = 1;
            midline_end_vx_i = 2;
            
            first_new_triangle_vertices[0] = 0;
            first_new_triangle_vertices[1] = 1;
            first_new_triangle_vertices[2] = USE_MIDLINE;
            
            second_new_triangle_vertices[0] = 0;
            second_new_triangle_vertices[1] = USE_MIDLINE;
            second_new_triangle_vertices[2] = 2;
        } else {
            /*
            0          1
            ...........
            .      .
            M   .
            . .
            .
            2
            */
            
            log_assert(distance_2_to_0 >= distance_1_to_2);
            log_assert(distance_2_to_0 >= distance_0_to_1);
            
            midline_start_vx_i = 0;
            midline_end_vx_i = 2;
            
            first_new_triangle_vertices[0] = 0;
            first_new_triangle_vertices[1] = 1;
            first_new_triangle_vertices[2] = USE_MIDLINE;
            
            second_new_triangle_vertices[0] = USE_MIDLINE;
            second_new_triangle_vertices[1] = 1;
            second_new_triangle_vertices[2] = 2;
        }
        
        zVertex mid_of_line;
        mid_of_line.x =
            (all_mesh_vertices[biggest_area_i].
                vertices[midline_start_vx_i].x +
            all_mesh_vertices[biggest_area_i].
                vertices[midline_end_vx_i].x) / 2;
        mid_of_line.y =
            (all_mesh_vertices[biggest_area_i].
                vertices[midline_start_vx_i].y +
            all_mesh_vertices[biggest_area_i].
                vertices[midline_end_vx_i].y) / 2;
        mid_of_line.z =
            (all_mesh_vertices[biggest_area_i].
                vertices[midline_start_vx_i].z +
            all_mesh_vertices[biggest_area_i].
                vertices[midline_end_vx_i].z) / 2;
        mid_of_line.uv[0] =
            (all_mesh_vertices[biggest_area_i].
                vertices[midline_start_vx_i].uv[0] +
            all_mesh_vertices[biggest_area_i].
                vertices[midline_end_vx_i].uv[0]) / 2;
        mid_of_line.uv[1] =
            (all_mesh_vertices[biggest_area_i].
                vertices[midline_start_vx_i].uv[1] +
            all_mesh_vertices[biggest_area_i].
                vertices[midline_end_vx_i].uv[1]) / 2;
        
        first_tri.normal = all_mesh_vertices[biggest_area_i].normal;
        second_tri.normal = all_mesh_vertices[biggest_area_i].normal;
        first_tri.parent_material_i =
            all_mesh_vertices[biggest_area_i].material_i;
        second_tri.parent_material_i =
            all_mesh_vertices[biggest_area_i].material_i;
        for (uint32_t m = 0; m < 3; m++) {
            
            if (first_new_triangle_vertices[m] == USE_MIDLINE) {
                first_tri.vertices[m] = mid_of_line;
            } else {
                log_assert(first_new_triangle_vertices[m] >= 0);
                log_assert(first_new_triangle_vertices[m] < 3);
                first_tri.vertices[m] =
                    all_mesh_vertices[biggest_area_i].
                        vertices[first_new_triangle_vertices[m]];
            }
            
            if (second_new_triangle_vertices[m] == USE_MIDLINE) {
                second_tri.vertices[m] = mid_of_line;
            } else {
                log_assert(second_new_triangle_vertices[m] >= 0);
                log_assert(second_new_triangle_vertices[m] < 3);
                second_tri.vertices[m] =
                    all_mesh_vertices[biggest_area_i].
                        vertices[second_new_triangle_vertices[m]];
            }
        }
        
        #ifndef LOGGER_IGNORE_ASSERTS
        float orig_area =
            get_squared_triangle_length(
                &all_mesh_triangles[biggest_area_i]);
        float first_tri_area =
            get_squared_triangle_length(&first_tri);
        float second_tri_area =
            get_squared_triangle_length(&second_tri);
        log_assert(orig_area > 0.0f);
        log_assert(first_tri_area > 0.0f);
        log_assert(second_tri_area > 0.0f);
        #endif
        
        all_mesh_vertices[biggest_area_i] = first_tri;
        all_mesh_vertices[temp_new_tail_i + 1] = second_tri;
        
        temp_new_tail_i++;
    }
    
    log_assert(all_mesh_vertices_size < (uint32_t)goal_new_tail_i);
    
    all_mesh_vertices_size = (uint32_t)goal_new_tail_i + 1;
    #endif
}

