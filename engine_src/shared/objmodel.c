#include "objmodel.h"

MeshSummary * all_mesh_summaries;
uint32_t all_mesh_summaries_size;

static void construct_mesh_summary(
    MeshSummary * to_construct,
    const int32_t id)
{
    to_construct->resource_name[0]          = '\0';
    to_construct->mesh_id                   =   id;
    to_construct->vertices_head_i           =   -1; // index @ all_mesh_vertices
    to_construct->vertices_size             =    0;
    to_construct->base_width                = 0.0f;
    to_construct->base_height               = 0.0f;
    to_construct->base_depth                = 0.0f;
    to_construct->shattered_vertices_head_i =   -1;
    to_construct->shattered_vertices_size   =    0;
}

GPULockedVertexWithMaterial * all_mesh_vertices;
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
    parser_vertex_buffer = (zVertex *)malloc_from_managed(
        sizeof(zVertex) * PARSER_VERTEX_BUFFER_SIZE);
    
    parser_normals_buffer = (BufferedNormal *)malloc_from_managed(
        sizeof(BufferedNormal) * PARSER_VERTEX_BUFFER_SIZE);
    
    parser_uv_u_buffer = (float *)malloc_from_managed(
        sizeof(float) * PARSER_VERTEX_BUFFER_SIZE);
    
    parser_uv_v_buffer = (float *)malloc_from_managed(
        sizeof(float) * PARSER_VERTEX_BUFFER_SIZE);
    
    all_mesh_summaries = (MeshSummary *)malloc_from_unmanaged(
        sizeof(MeshSummary) * ALL_MESHES_SIZE);
    
    for (uint32_t i = 0; i < ALL_MESHES_SIZE; i++) {
        construct_mesh_summary(&all_mesh_summaries[i], (int32_t)i);
    }
    
    assert(ALL_LOCKED_VERTICES_SIZE > 0);
    all_mesh_vertices = (GPULockedVertexWithMaterial *)malloc_from_unmanaged(
        sizeof(GPULockedVertexWithMaterial) * ALL_LOCKED_VERTICES_SIZE);
    
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
    all_mesh_vertices[0].gpu_data.xyz[0]                 = left_vertex;
    all_mesh_vertices[0].gpu_data.xyz[1]                 = top_vertex;
    all_mesh_vertices[0].gpu_data.xyz[2]                 = 0.0f;
    all_mesh_vertices[0].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[0].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[0].gpu_data.normal_xyz[2]          = -1.0f;
    all_mesh_vertices[0].gpu_data.uv[0]                  = left_uv_coord;
    all_mesh_vertices[0].gpu_data.uv[1]                  = top_uv_coord;
    // top right vertex
    all_mesh_vertices[1].gpu_data.xyz[0]                 = right_vertex;
    all_mesh_vertices[1].gpu_data.xyz[1]                 = top_vertex;
    all_mesh_vertices[1].gpu_data.xyz[2]                 = 0.0f;
    all_mesh_vertices[1].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[1].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[1].gpu_data.normal_xyz[2]          = -1.0f;
    all_mesh_vertices[1].gpu_data.uv[0]                  = right_uv_coord;
    all_mesh_vertices[1].gpu_data.uv[1]                  = top_uv_coord;
    // bottom left vertex
    all_mesh_vertices[2].gpu_data.xyz[0]                 = left_vertex;
    all_mesh_vertices[2].gpu_data.xyz[1]                 = bottom_vertex;
    all_mesh_vertices[2].gpu_data.xyz[2]                 = 0.0f;
    all_mesh_vertices[2].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[2].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[2].gpu_data.normal_xyz[2]          = -1.0f;
    all_mesh_vertices[2].gpu_data.uv[0]                  = left_uv_coord;
    all_mesh_vertices[2].gpu_data.uv[1]                  = bottom_uv_coord;
    
    // basic quad, triangle 2 
    // top right vertex
    all_mesh_vertices[3].gpu_data.xyz[0]                 = right_vertex;
    all_mesh_vertices[3].gpu_data.xyz[1]                 = top_vertex;
    all_mesh_vertices[3].gpu_data.xyz[2]                 = 0.0f;
    all_mesh_vertices[3].gpu_data.uv[0]                  = right_uv_coord;
    all_mesh_vertices[3].gpu_data.uv[1]                  = top_uv_coord;
    all_mesh_vertices[3].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[3].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[3].gpu_data.normal_xyz[2]          = -1.0f;
    // bottom right vertex
    all_mesh_vertices[4].gpu_data.xyz[0]                 = right_vertex;
    all_mesh_vertices[4].gpu_data.xyz[1]                 = bottom_vertex;
    all_mesh_vertices[4].gpu_data.xyz[2]                 = 0.0f;
    all_mesh_vertices[4].gpu_data.uv[0]                  = right_uv_coord;
    all_mesh_vertices[4].gpu_data.uv[1]                  = bottom_uv_coord;
    all_mesh_vertices[4].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[4].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[4].gpu_data.normal_xyz[2]          = -1.0f;
    // bottom left vertex
    all_mesh_vertices[5].gpu_data.xyz[0]                 = left_vertex;
    all_mesh_vertices[5].gpu_data.xyz[1]                 = bottom_vertex;
    all_mesh_vertices[5].gpu_data.xyz[2]                 = 0.0f;
    all_mesh_vertices[5].gpu_data.uv[0]                  = left_uv_coord;
    all_mesh_vertices[5].gpu_data.uv[1]                  = bottom_uv_coord;
    all_mesh_vertices[5].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[5].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[5].gpu_data.normal_xyz[2]          = -1.0f;
    
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
    
    const float front_vertex =  -1.0f;
    const float back_vertex  =   1.0f;
    
    // basic cube, front face triangle 1
    all_mesh_vertices[6].gpu_data.xyz[0]                 = left_vertex;
    all_mesh_vertices[6].gpu_data.xyz[1]                 = top_vertex;
    all_mesh_vertices[6].gpu_data.xyz[2]                 = front_vertex;
    all_mesh_vertices[6].gpu_data.uv[0]                  = left_uv_coord;
    all_mesh_vertices[6].gpu_data.uv[1]                  = top_uv_coord;
    all_mesh_vertices[6].gpu_data.normal_xyz[0]          =  0.0f;
    all_mesh_vertices[6].gpu_data.normal_xyz[1]          =  0.0f;
    all_mesh_vertices[6].gpu_data.normal_xyz[2]          = -1.0f;
    all_mesh_vertices[7].gpu_data.xyz[0]                 = right_vertex;
    all_mesh_vertices[7].gpu_data.xyz[1]                 = top_vertex;
    all_mesh_vertices[7].gpu_data.xyz[2]                 = front_vertex;
    all_mesh_vertices[7].gpu_data.uv[0]                  = right_uv_coord;
    all_mesh_vertices[7].gpu_data.uv[1]                  = top_uv_coord;
    all_mesh_vertices[7].gpu_data.normal_xyz[0]          =  0.0f;
    all_mesh_vertices[7].gpu_data.normal_xyz[1]          =  0.0f;
    all_mesh_vertices[7].gpu_data.normal_xyz[2]          = -1.0f;
    all_mesh_vertices[8].gpu_data.xyz[0]                 = left_vertex;
    all_mesh_vertices[8].gpu_data.xyz[1]                 = bottom_vertex;
    all_mesh_vertices[8].gpu_data.xyz[2]                 = front_vertex;
    all_mesh_vertices[8].gpu_data.uv[0]                  = left_uv_coord;
    all_mesh_vertices[8].gpu_data.uv[1]                  = bottom_uv_coord;
    all_mesh_vertices[8].gpu_data.normal_xyz[0]          =  0.0f;
    all_mesh_vertices[8].gpu_data.normal_xyz[1]          =  0.0f;
    all_mesh_vertices[8].gpu_data.normal_xyz[2]          = -1.0f;
    
    // basic cube, front face triangle 2
    all_mesh_vertices[9].gpu_data.xyz[0]                 = right_vertex;
    all_mesh_vertices[9].gpu_data.xyz[1]                 = top_vertex;
    all_mesh_vertices[9].gpu_data.xyz[2]                 = front_vertex;
    all_mesh_vertices[9].gpu_data.uv[0]                  = right_uv_coord;
    all_mesh_vertices[9].gpu_data.uv[1]                  = top_uv_coord;
    all_mesh_vertices[9].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[9].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[9].gpu_data.normal_xyz[2]          = -1.0f;
    all_mesh_vertices[10].gpu_data.xyz[0]                = right_vertex;
    all_mesh_vertices[10].gpu_data.xyz[1]                = bottom_vertex;
    all_mesh_vertices[10].gpu_data.xyz[2]                = front_vertex;
    all_mesh_vertices[10].gpu_data.uv[0]                 = right_uv_coord;
    all_mesh_vertices[10].gpu_data.uv[1]                 = bottom_uv_coord;
    all_mesh_vertices[10].gpu_data.normal_xyz[0]         = 0.0f;
    all_mesh_vertices[10].gpu_data.normal_xyz[1]         = 0.0f;
    all_mesh_vertices[10].gpu_data.normal_xyz[2]         = -1.0f;
    all_mesh_vertices[11].gpu_data.xyz[0]                 = left_vertex;
    all_mesh_vertices[11].gpu_data.xyz[1]                 = bottom_vertex;
    all_mesh_vertices[11].gpu_data.xyz[2]                 = front_vertex;
    all_mesh_vertices[11].gpu_data.uv[0]                  = left_uv_coord;
    all_mesh_vertices[11].gpu_data.uv[1]                  = bottom_uv_coord;
    all_mesh_vertices[11].gpu_data.normal_xyz[0]          = 0.0f;
    all_mesh_vertices[11].gpu_data.normal_xyz[1]          = 0.0f;
    all_mesh_vertices[11].gpu_data.normal_xyz[2]          = -1.0f;
    
    // basic cube, back face triangle 1
    all_mesh_vertices[12].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[12].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[12].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[12].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[12].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[12].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[12].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[12].gpu_data.normal_xyz[2] = 1.0f;
    all_mesh_vertices[13].gpu_data.xyz[0]     = right_vertex;
    all_mesh_vertices[13].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[13].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[13].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[13].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[13].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[13].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[13].gpu_data.normal_xyz[2] = 1.0f;
    all_mesh_vertices[14].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[14].gpu_data.xyz[1]     = bottom_vertex;
    all_mesh_vertices[14].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[14].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[14].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[14].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[14].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[14].gpu_data.normal_xyz[2] = 1.0f;
    
    // basic cube, back face triangle 2
    all_mesh_vertices[15].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[15].gpu_data.xyz[1] = top_vertex;
    all_mesh_vertices[15].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[15].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[15].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[15].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[15].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[15].gpu_data.normal_xyz[2] = 1.0f;
    all_mesh_vertices[16].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[16].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[16].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[16].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[16].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[16].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[16].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[16].gpu_data.normal_xyz[2] = 1.0f;
    all_mesh_vertices[17].gpu_data.xyz[0] = left_vertex;
    all_mesh_vertices[17].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[17].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[17].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[17].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[17].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[17].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[17].gpu_data.normal_xyz[2] = 1.0f;
    
    // basic cube, left face triangle 1
    all_mesh_vertices[18].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[18].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[18].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[18].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[18].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[18].gpu_data.normal_xyz[0] = -1.0f;
    all_mesh_vertices[18].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[18].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[19].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[19].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[19].gpu_data.xyz[2]     = front_vertex;
    all_mesh_vertices[19].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[19].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[19].gpu_data.normal_xyz[0] = -1.0f;
    all_mesh_vertices[19].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[19].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[20].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[20].gpu_data.xyz[1]     = bottom_vertex;
    all_mesh_vertices[20].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[20].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[20].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[20].gpu_data.normal_xyz[0] = -1.0f;
    all_mesh_vertices[20].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[20].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, left face triangle 2
    all_mesh_vertices[21].gpu_data.xyz[0] = left_vertex;
    all_mesh_vertices[21].gpu_data.xyz[1] = top_vertex;
    all_mesh_vertices[21].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[21].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[21].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[21].gpu_data.normal_xyz[0] = -1.0f;
    all_mesh_vertices[21].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[21].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[22].gpu_data.xyz[0] = left_vertex;
    all_mesh_vertices[22].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[22].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[22].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[22].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[22].gpu_data.normal_xyz[0] = -1.0f;
    all_mesh_vertices[22].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[22].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[23].gpu_data.xyz[0] = left_vertex;
    all_mesh_vertices[23].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[23].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[23].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[23].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[23].gpu_data.normal_xyz[0] = -1.0f;
    all_mesh_vertices[23].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[23].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, right face triangle 1
    all_mesh_vertices[24].gpu_data.xyz[0]     = right_vertex;
    all_mesh_vertices[24].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[24].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[24].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[24].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[24].gpu_data.normal_xyz[0] = 1.0f;
    all_mesh_vertices[24].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[24].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[25].gpu_data.xyz[0]     = right_vertex;
    all_mesh_vertices[25].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[25].gpu_data.xyz[2]     = front_vertex;
    all_mesh_vertices[25].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[25].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[25].gpu_data.normal_xyz[0] = 1.0f;
    all_mesh_vertices[25].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[25].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[26].gpu_data.xyz[0]     = right_vertex;
    all_mesh_vertices[26].gpu_data.xyz[1]     = bottom_vertex;
    all_mesh_vertices[26].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[26].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[26].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[26].gpu_data.normal_xyz[0] = 1.0f;
    all_mesh_vertices[26].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[26].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, right face triangle 2
    all_mesh_vertices[27].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[27].gpu_data.xyz[1] = top_vertex;
    all_mesh_vertices[27].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[27].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[27].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[27].gpu_data.normal_xyz[0] = 1.0f;
    all_mesh_vertices[27].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[27].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[28].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[28].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[28].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[28].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[28].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[28].gpu_data.normal_xyz[0] = 1.0f;
    all_mesh_vertices[28].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[28].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[29].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[29].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[29].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[29].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[29].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[29].gpu_data.normal_xyz[0] = 1.0f;
    all_mesh_vertices[29].gpu_data.normal_xyz[1] = 0.0f;
    all_mesh_vertices[29].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, top face triangle 1
    all_mesh_vertices[30].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[30].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[30].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[30].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[30].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[30].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[30].gpu_data.normal_xyz[1] = 1.0f;
    all_mesh_vertices[30].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[31].gpu_data.xyz[0]     = right_vertex;
    all_mesh_vertices[31].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[31].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[31].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[31].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[31].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[31].gpu_data.normal_xyz[1] = 1.0f;
    all_mesh_vertices[31].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[32].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[32].gpu_data.xyz[1]     = top_vertex;
    all_mesh_vertices[32].gpu_data.xyz[2]     = front_vertex;
    all_mesh_vertices[32].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[32].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[32].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[32].gpu_data.normal_xyz[1] = 1.0f;
    all_mesh_vertices[32].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, top face triangle 2
    all_mesh_vertices[33].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[33].gpu_data.xyz[1] = top_vertex;
    all_mesh_vertices[33].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[33].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[33].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[33].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[33].gpu_data.normal_xyz[1] = 1.0f;
    all_mesh_vertices[33].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[34].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[34].gpu_data.xyz[1] = top_vertex;
    all_mesh_vertices[34].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[34].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[34].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[34].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[34].gpu_data.normal_xyz[1] = 1.0f;
    all_mesh_vertices[34].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[35].gpu_data.xyz[0] = left_vertex;
    all_mesh_vertices[35].gpu_data.xyz[1] = top_vertex;
    all_mesh_vertices[35].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[35].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[35].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[35].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[35].gpu_data.normal_xyz[1] = 1.0f;
    all_mesh_vertices[35].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, bottom face triangle 1
    all_mesh_vertices[36].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[36].gpu_data.xyz[1]     = bottom_vertex;
    all_mesh_vertices[36].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[36].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[36].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[36].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[36].gpu_data.normal_xyz[1] = -1.0f;
    all_mesh_vertices[36].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[37].gpu_data.xyz[0]     = right_vertex;
    all_mesh_vertices[37].gpu_data.xyz[1]     = bottom_vertex;
    all_mesh_vertices[37].gpu_data.xyz[2]     = back_vertex;
    all_mesh_vertices[37].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[37].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[37].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[37].gpu_data.normal_xyz[1] = -1.0f;
    all_mesh_vertices[37].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[38].gpu_data.xyz[0]     = left_vertex;
    all_mesh_vertices[38].gpu_data.xyz[1]     = bottom_vertex;
    all_mesh_vertices[38].gpu_data.xyz[2]     = front_vertex;
    all_mesh_vertices[38].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[38].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[38].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[38].gpu_data.normal_xyz[1] = -1.0f;
    all_mesh_vertices[38].gpu_data.normal_xyz[2] = 0.0f;
    
    // basic cube, bottom face triangle 2
    all_mesh_vertices[39].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[39].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[39].gpu_data.xyz[2] = back_vertex;
    all_mesh_vertices[39].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[39].gpu_data.uv[1] = top_uv_coord;
    all_mesh_vertices[39].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[39].gpu_data.normal_xyz[1] = -1.0f;
    all_mesh_vertices[39].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[40].gpu_data.xyz[0] = right_vertex;
    all_mesh_vertices[40].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[40].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[40].gpu_data.uv[0] = right_uv_coord;
    all_mesh_vertices[40].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[40].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[40].gpu_data.normal_xyz[1] = -1.0f;
    all_mesh_vertices[40].gpu_data.normal_xyz[2] = 0.0f;
    all_mesh_vertices[41].gpu_data.xyz[0] = left_vertex;
    all_mesh_vertices[41].gpu_data.xyz[1] = bottom_vertex;
    all_mesh_vertices[41].gpu_data.xyz[2] = front_vertex;
    all_mesh_vertices[41].gpu_data.uv[0] = left_uv_coord;
    all_mesh_vertices[41].gpu_data.uv[1] = bottom_uv_coord;
    all_mesh_vertices[41].gpu_data.normal_xyz[0] = 0.0f;
    all_mesh_vertices[41].gpu_data.normal_xyz[1] = -1.0f;
    all_mesh_vertices[41].gpu_data.normal_xyz[2] = 0.0f;
    
    strcpy_capped(
        all_mesh_summaries[2].resource_name,
        OBJ_STRING_SIZE,
        "basic_point");
    all_mesh_summaries[2].mesh_id = 2;
    all_mesh_summaries[2].vertices_head_i = 42;
    all_mesh_summaries[2].vertices_size = 1;
    all_mesh_summaries[2].base_width = 1.0f;
    all_mesh_summaries[2].base_height = 1.0f;
    all_mesh_summaries[2].base_depth = 1.0f;
    all_mesh_summaries[2].materials_size = 1;
    all_mesh_summaries[2].shattered_vertices_head_i = -1;
    all_mesh_summaries[2].shattered_vertices_size = 0;
    
    // basic point (only 1 vertex)
    all_mesh_vertices[42].gpu_data.xyz[0] = 0;
    all_mesh_vertices[42].gpu_data.xyz[1] = 0;
    all_mesh_vertices[42].gpu_data.xyz[2] = 0;
    all_mesh_vertices[42].gpu_data.uv[0]  = 0;
    all_mesh_vertices[42].gpu_data.uv[1]  = 0;
    all_mesh_vertices[42].gpu_data.normal_xyz[0] =  0.0f;
    all_mesh_vertices[42].gpu_data.normal_xyz[1] =  0.0f;
    all_mesh_vertices[42].gpu_data.normal_xyz[2] = -1.0f;
    
    all_mesh_summaries_size = 3;
    all_mesh_vertices_size = 43;
    
    free_from_managed(parser_vertex_buffer);
    
    free_from_managed(parser_normals_buffer);
    
    free_from_managed(parser_uv_u_buffer);
    
    free_from_managed(parser_uv_v_buffer);
}

#ifndef LOGGER_IGNORE_ASSERTS
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
}
#endif

//static void guess_ztriangle_normal(zTriangle * input) {
//    zVertex vector1;
//    zVertex vector2;
//
//    vector1.x = input->vertices[1].x - input->vertices[0].x;
//    vector1.y = input->vertices[1].y - input->vertices[0].y;
//    vector1.z = input->vertices[1].z - input->vertices[0].z;
//
//    vector2.x = input->vertices[2].x - input->vertices[0].x;
//    vector2.y = input->vertices[2].y - input->vertices[0].y;
//    vector2.z = input->vertices[2].z - input->vertices[0].z;
//
//    input->normal.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
//    input->normal.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
//    input->normal.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);
//}

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

static void guess_gpu_triangle_normal(GPULockedVertex * to_change) {
    float vec1_x = to_change[1].xyz[0] - to_change[0].xyz[0];
    float vec1_y = to_change[1].xyz[1] - to_change[0].xyz[1];
    float vec1_z = to_change[1].xyz[2] - to_change[0].xyz[2];
    
    float vec2_x = to_change[2].xyz[0] - to_change[0].xyz[0];
    float vec2_y = to_change[2].xyz[1] - to_change[0].xyz[1];
    float vec2_z = to_change[2].xyz[2] - to_change[0].xyz[2];
    
    to_change[0].normal_xyz[0] = (vec1_y * vec2_z) - (vec1_z * vec2_y);
    to_change[0].normal_xyz[1] = (vec1_z * vec2_x) - (vec1_x * vec2_z);
    to_change[0].normal_xyz[2] = (vec1_x * vec2_y) - (vec1_y * vec2_x);
    to_change[1].normal_xyz[0] = to_change[0].normal_xyz[0];
    to_change[1].normal_xyz[1] = to_change[0].normal_xyz[1];
    to_change[1].normal_xyz[2] = to_change[0].normal_xyz[2];
    to_change[2].normal_xyz[0] = to_change[0].normal_xyz[0];
    to_change[2].normal_xyz[1] = to_change[0].normal_xyz[1];
    to_change[2].normal_xyz[2] = to_change[0].normal_xyz[2];
}

//static void populate_new_triangle_with_parser_buffers(
//    GPULockedVertex * triangle_recipient,
//    int32_t vertex_i_0,
//    int32_t vertex_i_1,
//    int32_t vertex_i_2,
//    int32_t uv_coord_i_0,
//    int32_t uv_coord_i_1,
//    int32_t uv_coord_i_2,
//    int32_t normals_i_0,
//    int32_t normals_i_1,
//    int32_t normals_i_2)
//{
//    log_assert(vertex_i_0 != vertex_i_1);
//    log_assert(vertex_i_0 != vertex_i_2);
//    log_assert(vertex_i_0 > 0);
//    log_assert(vertex_i_1 > 0);
//    log_assert(vertex_i_2 > 0);
//    log_assert(uv_coord_i_0 < PARSER_VERTEX_BUFFER_SIZE);
//    log_assert(uv_coord_i_1 < PARSER_VERTEX_BUFFER_SIZE);
//    log_assert(uv_coord_i_2 < PARSER_VERTEX_BUFFER_SIZE);
//    log_assert(normals_i_0 < PARSER_VERTEX_BUFFER_SIZE);
//    log_assert(normals_i_1 < PARSER_VERTEX_BUFFER_SIZE);
//    log_assert(normals_i_2 < PARSER_VERTEX_BUFFER_SIZE);
//
//    uint32_t target_vertex_0 = 0;
//    uint32_t target_vertex_1 = 1;
//    uint32_t target_vertex_2 = 2;
//
//    triangle_recipient[target_vertex_0].xyz[0] =
//        parser_vertex_buffer[vertex_i_0 - 1].x;
//    triangle_recipient[target_vertex_0].xyz[1] =
//        parser_vertex_buffer[vertex_i_0 - 1].y;
//    triangle_recipient[target_vertex_0].xyz[2] =
//        parser_vertex_buffer[vertex_i_0 - 1].z;
//    triangle_recipient[target_vertex_1].xyz[0] =
//        parser_vertex_buffer[vertex_i_1 - 1].x;
//    triangle_recipient[target_vertex_1].xyz[1] =
//        parser_vertex_buffer[vertex_i_1 - 1].y;
//    triangle_recipient[target_vertex_1].xyz[2] =
//        parser_vertex_buffer[vertex_i_1 - 1].z;
//    triangle_recipient[target_vertex_2].xyz[0] =
//        parser_vertex_buffer[vertex_i_2 - 1].x;
//    triangle_recipient[target_vertex_2].xyz[1] =
//        parser_vertex_buffer[vertex_i_2 - 1].y;
//    triangle_recipient[target_vertex_2].xyz[2] =
//        parser_vertex_buffer[vertex_i_2 - 1].z;
//
//    if (
//        uv_coord_i_0 > 0 &&
//        uv_coord_i_1 > 0 &&
//        uv_coord_i_2 > 0)
//    {
//        triangle_recipient[target_vertex_0].uv[0] =
//            parser_uv_u_buffer[uv_coord_i_0 - 1];
//        triangle_recipient[target_vertex_0].uv[1] =
//            parser_uv_v_buffer[uv_coord_i_0 - 1];
//        triangle_recipient[target_vertex_1].uv[0] =
//            parser_uv_u_buffer[uv_coord_i_1 - 1];
//        triangle_recipient[target_vertex_1].uv[1] =
//            parser_uv_v_buffer[uv_coord_i_1 - 1];
//        triangle_recipient[target_vertex_2].uv[0] =
//            parser_uv_u_buffer[uv_coord_i_2 - 1];
//        triangle_recipient[target_vertex_2].uv[1] =
//            parser_uv_v_buffer[uv_coord_i_2 - 1];
//    } else {
//        triangle_recipient[target_vertex_0].uv[0] =
//            1.0f;
//        triangle_recipient[target_vertex_0].uv[1] =
//            0.0f;
//        triangle_recipient[target_vertex_1].uv[0] =
//            1.0f;
//        triangle_recipient[target_vertex_1].uv[1] =
//            1.0f;
//        triangle_recipient[target_vertex_2].uv[0] =
//            0.0f;
//        triangle_recipient[target_vertex_2].uv[1] =
//            1.0f;
//    }
//
//    if (
//        normals_i_0 > 0 &&
//        normals_i_1 > 0 &&
//        normals_i_2 > 0)
//    {
//        triangle_recipient[target_vertex_0].normal_xyz[0] =
//            parser_normals_buffer[normals_i_0 - 1].x;
//        triangle_recipient[target_vertex_0].normal_xyz[1] =
//            parser_normals_buffer[normals_i_0 - 1].y;
//        triangle_recipient[target_vertex_0].normal_xyz[2] =
//            parser_normals_buffer[normals_i_0 - 1].z;
//        triangle_recipient[target_vertex_1].normal_xyz[0] =
//            parser_normals_buffer[normals_i_1 - 1].x;
//        triangle_recipient[target_vertex_1].normal_xyz[1] =
//            parser_normals_buffer[normals_i_1 - 1].y;
//        triangle_recipient[target_vertex_1].normal_xyz[2] =
//            parser_normals_buffer[normals_i_1 - 1].z;
//        triangle_recipient[target_vertex_2].normal_xyz[0] =
//            parser_normals_buffer[normals_i_2 - 1].x;
//        triangle_recipient[target_vertex_2].normal_xyz[1] =
//            parser_normals_buffer[normals_i_2 - 1].y;
//        triangle_recipient[target_vertex_2].normal_xyz[2] =
//            parser_normals_buffer[normals_i_2 - 1].z;
//    } else {
//        guess_gpu_triangle_normal(triangle_recipient);
//    }
//
//    normalize_gpu_triangle_normals(triangle_recipient);
//}

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
        
    ParsedObj parsed_obj;
    uint32_t good = 0;
    parse_obj(
        /* ParsedObj * recipient: */
            &parsed_obj,
        /* char * raw_buffer: */
            obj_file.contents,
        /* uint32_t * success: */
            &good);
    log_assert(good);
    
    if (
        parsed_obj.vertices_count < 1 ||
        (parsed_obj.triangles_count + parsed_obj.quads_count) < 1)
    {
        good = 0;
    }
    log_assert(good);
    
    if (!good) {
        return -1;
    }
    
    all_mesh_summaries[all_mesh_summaries_size].vertices_head_i =
        (int32_t)all_mesh_vertices_size;
    
    log_assert(all_mesh_vertices_size < ALL_LOCKED_VERTICES_SIZE);
    
    all_mesh_summaries[all_mesh_summaries_size].materials_size =
        parsed_obj.materials_count;
    for (uint32_t i = 0; i < parsed_obj.materials_count; i++) {
        strcpy_capped(
            all_mesh_summaries[all_mesh_summaries_size].material_names[i],
            OBJ_STRING_SIZE,
            parsed_obj.materials[i].name);
    }
    
    for (
        uint32_t triangle_i = 0;
        triangle_i < parsed_obj.triangles_count;
        triangle_i++)
    {
        uint32_t cur_material_i = parsed_obj.triangles[triangle_i][4];
        
        for (uint32_t _ = 0; _ < 3; _++) {
            uint32_t vert_i = parsed_obj.triangles[triangle_i][_];
            
            log_assert(vert_i >= 1);
            log_assert(vert_i <= parsed_obj.vertices_count);
            log_assert(all_mesh_vertices_size < ALL_LOCKED_VERTICES_SIZE);
            
            all_mesh_vertices[all_mesh_vertices_size].gpu_data.xyz[0] =
                parsed_obj.vertices[vert_i - 1][0];
            all_mesh_vertices[all_mesh_vertices_size].gpu_data.xyz[1] =
                parsed_obj.vertices[vert_i - 1][1];
            all_mesh_vertices[all_mesh_vertices_size].gpu_data.xyz[2] =
                parsed_obj.vertices[vert_i - 1][2];
            
            all_mesh_vertices[all_mesh_vertices_size].parent_material_i =
                cur_material_i;
            
            #ifndef LOGGER_IGNORE_ASSERTS
            if (all_mesh_vertices[all_mesh_vertices_size].parent_material_i > 0)
            {
                log_assert(
                    all_mesh_vertices[all_mesh_vertices_size].
                        parent_material_i <
                            parsed_obj.materials_count);
                log_assert(
                    all_mesh_vertices[all_mesh_vertices_size].
                        parent_material_i <
                            all_mesh_summaries[all_mesh_summaries_size].
                                materials_size);
            }
            #endif
            
            if (parsed_obj.normals_count > 0) {
                uint32_t norm_i = parsed_obj.triangle_normals[triangle_i][_];
                
                log_assert(norm_i >= 1);
                log_assert(norm_i <= parsed_obj.normals_count);
                
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.
                    normal_xyz[0] =
                        parsed_obj.normals[norm_i - 1][0];
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.
                    normal_xyz[1] =
                        parsed_obj.normals[norm_i - 1][1];
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.
                    normal_xyz[2] =
                        parsed_obj.normals[norm_i - 1][2];
            } else {
                guess_gpu_triangle_normal(
                    /* GPULockedVertex * to_change: */
                        &all_mesh_vertices[all_mesh_vertices_size].gpu_data);
            }
            
            if (parsed_obj.textures_count > 0) {
                uint32_t text_i = parsed_obj.triangle_textures[triangle_i][_];
                
                log_assert(text_i >= 1);
                log_assert(text_i <= parsed_obj.textures_count);
                
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[0] =
                    parsed_obj.textures[text_i - 1][0];
                
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[1] =
                    parsed_obj.textures[text_i - 1][1];
            } else {
                // No uv data in .obj file, gotta guess
                // TODO: Maybe should be part of the obj parser?
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[0] =
                    all_mesh_vertices_size % 2 == 0 ? 0.0f : 1.0f;
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[1] =
                    all_mesh_vertices_size % 4 == 0 ? 0.0f : 1.0f;
            }
            
            all_mesh_vertices_size += 1;
        }
    }
    
    for (
        uint32_t quad_i = 0;
        quad_i < parsed_obj.quads_count;
        quad_i++)
    {
        uint32_t cur_material_i = parsed_obj.quads[quad_i][5];

        for (uint32_t offset = 0; offset < 2; offset++) {
            for (uint32_t _ = 0; _ < 3; _++) {
                uint32_t vert_i = parsed_obj.quads[quad_i][_ + offset];

                log_assert(vert_i >= 1);
                log_assert(vert_i <= parsed_obj.vertices_count);
                
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.xyz[0] =
                    parsed_obj.vertices[vert_i - 1][0];
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.xyz[1] =
                    parsed_obj.vertices[vert_i - 1][1];
                all_mesh_vertices[all_mesh_vertices_size].gpu_data.xyz[2] =
                    parsed_obj.vertices[vert_i - 1][2];
                
                all_mesh_vertices[all_mesh_vertices_size].parent_material_i =
                    cur_material_i;

                #ifndef LOGGER_IGNORE_ASSERTS
                if (all_mesh_vertices[all_mesh_vertices_size].
                    parent_material_i > 0)
                {
                    log_assert(
                        all_mesh_vertices[all_mesh_vertices_size].
                            parent_material_i <
                                parsed_obj.materials_count);
                    log_assert(
                        all_mesh_vertices[all_mesh_vertices_size].
                            parent_material_i <
                                all_mesh_summaries[all_mesh_summaries_size].
                                    materials_size);
                }
                #endif

                if (parsed_obj.normals_count > 0) {
                    uint32_t norm_i = parsed_obj.quad_normals[quad_i]
                        [_ + offset];

                    log_assert(norm_i >= 1);
                    log_assert(norm_i <= parsed_obj.normals_count);

                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.
                        normal_xyz[0] =
                            parsed_obj.normals[norm_i - 1][0];
                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.
                        normal_xyz[1] =
                            parsed_obj.normals[norm_i - 1][1];
                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.
                        normal_xyz[2] =
                            parsed_obj.normals[norm_i - 1][2];
                } else {
                    guess_gpu_triangle_normal(
                        /* GPULockedVertex * to_change: */
                            &all_mesh_vertices[all_mesh_vertices_size].gpu_data);
                }
                
                if (parsed_obj.textures_count > 0) {
                    uint32_t text_i = parsed_obj.quad_textures[quad_i]
                        [_ + offset];
                    
                    log_assert(text_i >= 1);
                    log_assert(text_i <= parsed_obj.textures_count);
                    
                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[0] =
                        parsed_obj.textures[text_i - 1][0];

                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[1] =
                        parsed_obj.textures[text_i - 1][1];
                } else {
                    // No uv data in .obj file, gotta guess
                    // TODO: Maybe should be part of the obj parser?
                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[0] =
                        all_mesh_vertices_size % 2 == 0 ? 0.0f : 1.0f;
                    all_mesh_vertices[all_mesh_vertices_size].gpu_data.uv[1] =
                        all_mesh_vertices_size % 4 == 0 ? 0.0f : 1.0f;
                }

                all_mesh_vertices_size += 1;
            }
        }
    }
    
    all_mesh_summaries[all_mesh_summaries_size].mesh_id =
        (int32_t)all_mesh_summaries_size;
    all_mesh_summaries[all_mesh_summaries_size].vertices_size =
        (int32_t)all_mesh_vertices_size -
        all_mesh_summaries[all_mesh_summaries_size].vertices_head_i;
    log_assert(all_mesh_summaries[all_mesh_summaries_size].vertices_size > 0);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t new_tail_i = (uint32_t)(
        all_mesh_summaries[all_mesh_summaries_size].vertices_head_i +
        all_mesh_summaries[all_mesh_summaries_size].vertices_size -
        1);
    log_assert(new_tail_i < all_mesh_vertices_size);
    #endif
    
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
        for (int32_t m = 0; m < 3; m++) {
            if (min_x > all_mesh_vertices[tri_i + m].gpu_data.xyz[0]) {
                min_x = all_mesh_vertices[tri_i + m].gpu_data.xyz[0];
            }
            if (min_y > all_mesh_vertices[tri_i + m].gpu_data.xyz[1]) {
                min_y = all_mesh_vertices[tri_i + m].gpu_data.xyz[1];
            }
            if (min_z > all_mesh_vertices[tri_i + m].gpu_data.xyz[2]) {
                min_z = all_mesh_vertices[tri_i + m].gpu_data.xyz[2];
            }
            if (max_x < all_mesh_vertices[tri_i + m].gpu_data.xyz[0]) {
                max_x = all_mesh_vertices[tri_i + m].gpu_data.xyz[0];
            }
            if (max_y < all_mesh_vertices[tri_i + m].gpu_data.xyz[1]) {
                max_y = all_mesh_vertices[tri_i + m].gpu_data.xyz[1];
            }
            if (max_z < all_mesh_vertices[tri_i + m].gpu_data.xyz[2]) {
                max_z = all_mesh_vertices[tri_i + m].gpu_data.xyz[2];
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
    
    #ifndef LOGGER_IGNORE_ASSERTS
    assert_objmodel_validity((int32_t)all_mesh_summaries_size - 1);
    #endif
    
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
        for (int32_t m = 0; m < 3; m++) {
            if (smallest_x > all_mesh_vertices[tri_i + m].gpu_data.xyz[0]) {
                smallest_x = all_mesh_vertices[tri_i + m].gpu_data.xyz[0];
            }
            if (largest_x < all_mesh_vertices[tri_i + m].gpu_data.xyz[0]) {
                largest_x = all_mesh_vertices[tri_i + m].gpu_data.xyz[0];
            }
            if (smallest_y > all_mesh_vertices[tri_i + m].gpu_data.xyz[1]) {
                smallest_y = all_mesh_vertices[tri_i + m].gpu_data.xyz[1];
            }
            if (largest_y < all_mesh_vertices[tri_i + m].gpu_data.xyz[1]) {
                largest_y = all_mesh_vertices[tri_i + m].gpu_data.xyz[1];
            }
            if (smallest_z > all_mesh_vertices[tri_i + m].gpu_data.xyz[2]) {
                smallest_z = all_mesh_vertices[tri_i + m].gpu_data.xyz[2];
            }
            if (largest_z < all_mesh_vertices[tri_i + m].gpu_data.xyz[2]) {
                largest_z = all_mesh_vertices[tri_i + m].gpu_data.xyz[2];
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
        all_mesh_vertices[vert_i].gpu_data.xyz[0] -= x_delta;
        all_mesh_vertices[vert_i].gpu_data.xyz[1] -= y_delta;
        all_mesh_vertices[vert_i].gpu_data.xyz[2] -= z_delta;
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
