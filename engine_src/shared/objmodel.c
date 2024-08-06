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

LockedVertexWithMaterialCollection * all_mesh_vertices;

typedef struct BufferedNormal {
    float x;
    float y;
    float z;
} BufferedNormal;

static float get_vector_magnitude(float input[3]) {
    float sum_squares =
        (input[0] * input[0]) +
        (input[1] * input[1]) +
        (input[2] * input[2]);
    
    sum_squares = isnan(sum_squares) || !isfinite(sum_squares) ?
        FLOAT32_MAX : sum_squares;
    
    float return_value = sqrtf(sum_squares);
    
    log_assert(!isnan(return_value));
    
    return return_value;
}

static void normalize_vector_inplace(
    float vector[3])
{
    float magnitude = get_vector_magnitude(vector);
    if (magnitude < 0.0001f && magnitude > -0.0001f) {
        magnitude = 0.0001f;
    }
    
    vector[0] /= magnitude;
    vector[1] /= magnitude;
    vector[2] /= magnitude;
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
    normalize_vector_inplace(to_change[0].normal_xyz);
    to_change[1].normal_xyz[0] = to_change[0].normal_xyz[0];
    to_change[1].normal_xyz[1] = to_change[0].normal_xyz[1];
    to_change[1].normal_xyz[2] = to_change[0].normal_xyz[2];
    to_change[2].normal_xyz[0] = to_change[0].normal_xyz[0];
    to_change[2].normal_xyz[1] = to_change[0].normal_xyz[1];
    to_change[2].normal_xyz[2] = to_change[0].normal_xyz[2];
}

void init_all_meshes(void) {
    all_mesh_summaries = (MeshSummary *)malloc_from_unmanaged(
        sizeof(MeshSummary) * ALL_MESHES_SIZE);
    
    for (uint32_t i = 0; i < ALL_MESHES_SIZE; i++) {
        construct_mesh_summary(&all_mesh_summaries[i], (int32_t)i);
    }
    
    assert(ALL_LOCKED_VERTICES_SIZE > 0);
    all_mesh_vertices = (LockedVertexWithMaterialCollection *)
        malloc_from_unmanaged(sizeof(LockedVertexWithMaterialCollection));
    memset_char(
        all_mesh_vertices,
        0,
        sizeof(LockedVertexWithMaterialCollection));
    
    // Let's hardcode a basic quad since that's a mesh that will be used by
    // even the features inherent to the engine itself (the terminal, any
    // text labels, the FPS label, etc)
    strcpy_capped(
        all_mesh_summaries[0].resource_name,
        OBJ_STRING_SIZE,
        "basic_quad");
    all_mesh_summaries[0].mesh_id = BASIC_QUAD_MESH_ID;
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
    const float left_uv_coord   =  0.0f;
    const float right_uv_coord  =  1.0f;
    const float bottom_uv_coord =  1.0f;
    const float top_uv_coord    =  0.0f;
    
    // basic quad, triangle 1
    // top left vertex
    all_mesh_vertices->gpu_data[0].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[0].xyz[1]                 = top_vertex;
    all_mesh_vertices->gpu_data[0].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[0].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[0].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[0].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[0].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[0].uv[1]                  = top_uv_coord;
    all_mesh_vertices->gpu_data[0].parent_material_i      = 0;
    // top right vertex
    all_mesh_vertices->gpu_data[1].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[1].xyz[1]                 = top_vertex;
    all_mesh_vertices->gpu_data[1].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[1].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[1].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[1].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[1].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[1].uv[1]                  = top_uv_coord;
    all_mesh_vertices->gpu_data[1].parent_material_i      = 0;
    // bottom left vertex
    all_mesh_vertices->gpu_data[2].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[2].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[2].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[2].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[2].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[2].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[2].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[2].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[2].parent_material_i      = 0;
    
    // basic quad, triangle 2 
    // top right vertex
    all_mesh_vertices->gpu_data[3].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[3].xyz[1]                 = top_vertex;
    all_mesh_vertices->gpu_data[3].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[3].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[3].uv[1]                  = top_uv_coord;
    all_mesh_vertices->gpu_data[3].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[3].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[3].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[3].parent_material_i      = 0;
    // bottom right vertex
    all_mesh_vertices->gpu_data[4].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[4].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[4].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[4].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[4].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[4].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[4].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[4].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[4].parent_material_i      = 0;
    // bottom left vertex
    all_mesh_vertices->gpu_data[5].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[5].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[5].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[5].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[5].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[5].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[5].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[5].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[5].parent_material_i      = 0;
    
    // Let's hardcode a basic cube since that will be used by the particle
    // effects system
    strcpy_capped(
        all_mesh_summaries[1].resource_name,
        OBJ_STRING_SIZE,
        "basic_cube");
    all_mesh_summaries[1].vertices_head_i = 6;
    all_mesh_summaries[1].vertices_size = 36;
    all_mesh_summaries[1].mesh_id = BASIC_CUBE_MESH_ID;
    all_mesh_summaries[1].materials_size = 1;
    all_mesh_summaries[1].base_width = 1.0f;
    all_mesh_summaries[1].base_height = 1.0f;
    all_mesh_summaries[1].base_depth = 1.0f;
    
    const float front_vertex =  -1.0f;
    const float back_vertex  =   1.0f;
    
    // basic cube, front face triangle 1
    all_mesh_vertices->gpu_data[6].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[6].xyz[1]                 = top_vertex;
    all_mesh_vertices->gpu_data[6].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[6].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[6].uv[1]                  = top_uv_coord;
    all_mesh_vertices->gpu_data[6].normal_xyz[0]          =  0.0f;
    all_mesh_vertices->gpu_data[6].normal_xyz[1]          =  0.0f;
    all_mesh_vertices->gpu_data[6].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[6].parent_material_i      = 0;
    
    all_mesh_vertices->gpu_data[7].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[7].xyz[1]                 = top_vertex;
    all_mesh_vertices->gpu_data[7].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[7].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[7].uv[1]                  = top_uv_coord;
    all_mesh_vertices->gpu_data[7].normal_xyz[0]          =  0.0f;
    all_mesh_vertices->gpu_data[7].normal_xyz[1]          =  0.0f;
    all_mesh_vertices->gpu_data[7].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[7].parent_material_i      = 0;
    
    all_mesh_vertices->gpu_data[8].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[8].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[8].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[8].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[8].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[8].normal_xyz[0]          =  0.0f;
    all_mesh_vertices->gpu_data[8].normal_xyz[1]          =  0.0f;
    all_mesh_vertices->gpu_data[8].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[8].parent_material_i      = 0;
    
    // basic cube, front face triangle  2
    all_mesh_vertices->gpu_data[9].xyz[0]                  = right_vertex;
    all_mesh_vertices->gpu_data[9].xyz[1]                  = top_vertex;
    all_mesh_vertices->gpu_data[9].xyz[2]                  = front_vertex;
    all_mesh_vertices->gpu_data[9].uv[0]                   = right_uv_coord;
    all_mesh_vertices->gpu_data[9].uv[1]                   = top_uv_coord;
    all_mesh_vertices->gpu_data[9].normal_xyz[0]           = 0.0f;
    all_mesh_vertices->gpu_data[9].normal_xyz[1]           = 0.0f;
    all_mesh_vertices->gpu_data[9].normal_xyz[2]           = -1.0f;
    all_mesh_vertices->gpu_data[9].parent_material_i       = 0;
    
    all_mesh_vertices->gpu_data[10].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[10].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[10].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[10].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[10].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[10].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[10].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[10].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[10].parent_material_i      = 0;
    
    all_mesh_vertices->gpu_data[11].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[11].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[11].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[11].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[11].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[11].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[11].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[11].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[11].parent_material_i      = 0;
    
    // basic cube, back face triangle 1
    all_mesh_vertices->gpu_data[12].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[12].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[12].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[12].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[12].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[12].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[12].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[12].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[12].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[13].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[13].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[13].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[13].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[13].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[13].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[13].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[13].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[13].parent_material_i = 0;

    all_mesh_vertices->gpu_data[14].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[14].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[14].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[14].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[14].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[14].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[14].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[14].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[14].parent_material_i = 0;
    
    // basic cube, back face triangle 2
    all_mesh_vertices->gpu_data[15].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[15].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[15].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[15].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[15].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[15].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[15].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[15].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[15].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[16].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[16].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[16].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[16].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[16].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[16].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[16].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[16].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[16].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[17].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[17].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[17].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[17].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[17].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[17].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[17].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[17].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[17].parent_material_i = 0;
    
    // basic cube, left face triangle 1
    all_mesh_vertices->gpu_data[18].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[18].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[18].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[18].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[18].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[18].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[18].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[18].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[18].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[19].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[19].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[19].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[19].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[19].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[19].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[19].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[19].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[19].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[20].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[20].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[20].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[20].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[20].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[20].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[20].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[20].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[20].parent_material_i = 0;
    
    // basic cube, left face triangle 2
    all_mesh_vertices->gpu_data[21].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[21].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[21].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[21].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[21].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[21].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[21].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[21].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[21].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[22].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[22].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[22].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[22].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[22].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[22].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[22].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[22].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[22].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[23].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[23].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[23].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[23].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[23].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[23].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[23].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[23].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[23].parent_material_i = 0;
    
    // basic cube, right face triangle 1
    all_mesh_vertices->gpu_data[24].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[24].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[24].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[24].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[24].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[24].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[24].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[24].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[24].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[25].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[25].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[25].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[25].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[25].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[25].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[25].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[25].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[25].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[26].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[26].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[26].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[26].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[26].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[26].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[26].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[26].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[26].parent_material_i = 0;
    
    // basic cube, right face triangle 2
    all_mesh_vertices->gpu_data[27].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[27].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[27].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[27].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[27].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[27].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[27].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[27].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[27].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[28].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[28].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[28].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[28].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[28].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[28].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[28].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[28].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[28].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[29].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[29].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[29].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[29].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[29].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[29].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[29].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[29].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[29].parent_material_i = 0;
    
    // basic cube, top face triangle 1
    all_mesh_vertices->gpu_data[30].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[30].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[30].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[30].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[30].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[30].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[30].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[30].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[30].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[31].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[31].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[31].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[31].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[31].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[31].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[31].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[31].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[31].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[32].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[32].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[32].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[32].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[32].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[32].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[32].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[32].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[32].parent_material_i = 0;
    
    // basic cube, top face triangle 2
    all_mesh_vertices->gpu_data[33].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[33].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[33].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[33].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[33].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[33].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[33].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[33].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[33].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[34].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[34].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[34].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[34].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[34].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[34].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[34].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[34].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[34].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[35].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[35].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[35].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[35].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[35].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[35].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[35].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[35].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[35].parent_material_i = 0;
    
    // basic cube, bottom face triangle 1
    all_mesh_vertices->gpu_data[36].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[36].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[36].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[36].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[36].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[36].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[36].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[36].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[36].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[37].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[37].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[37].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[37].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[37].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[37].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[37].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[37].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[37].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[38].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[38].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[38].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[38].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[38].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[38].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[38].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[38].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[38].parent_material_i = 0;
    
    // basic cube, bottom face triangle 2
    all_mesh_vertices->gpu_data[39].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[39].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[39].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[39].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[39].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[39].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[39].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[39].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[39].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[40].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[40].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[40].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[40].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[40].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[40].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[40].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[40].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[40].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[41].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[41].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[41].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[41].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[41].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[41].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[41].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[41].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[41].parent_material_i = 0;
    
    strcpy_capped(
        all_mesh_summaries[2].resource_name,
        OBJ_STRING_SIZE,
        "basic_point");
    all_mesh_summaries[2].mesh_id = BASIC_POINT_MESH_ID;
    all_mesh_summaries[2].vertices_head_i = 42;
    all_mesh_summaries[2].vertices_size = 1;
    all_mesh_summaries[2].base_width = 1.0f;
    all_mesh_summaries[2].base_height = 1.0f;
    all_mesh_summaries[2].base_depth = 1.0f;
    all_mesh_summaries[2].materials_size = 1;
    all_mesh_summaries[2].shattered_vertices_head_i = 42;
    all_mesh_summaries[2].shattered_vertices_size = 1;
    
    // basic point (only 1 vertex)
    all_mesh_vertices->gpu_data[42].xyz[0]            = 0;
    all_mesh_vertices->gpu_data[42].xyz[1]            = 0;
    all_mesh_vertices->gpu_data[42].xyz[2]            = 0;
    all_mesh_vertices->gpu_data[42].uv[0]             = 0;
    all_mesh_vertices->gpu_data[42].uv[1]             = 0;
    all_mesh_vertices->gpu_data[42].normal_xyz[0]     =  0.0f;
    all_mesh_vertices->gpu_data[42].normal_xyz[1]     =  0.0f;
    all_mesh_vertices->gpu_data[42].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[42].parent_material_i = 0;
    
    strcpy_capped(
        all_mesh_summaries[3].resource_name,
        OBJ_STRING_SIZE,
        "basic_line");
    all_mesh_summaries[3].mesh_id = BASIC_LINE_MESH_ID;
    all_mesh_summaries[3].vertices_head_i = 42;
    all_mesh_summaries[3].vertices_size = 1;
    all_mesh_summaries[3].base_width = 1.0f;
    all_mesh_summaries[3].base_height = 1.0f;
    all_mesh_summaries[3].base_depth = 1.0f;
    all_mesh_summaries[3].materials_size = 1;
    all_mesh_summaries[3].shattered_vertices_head_i = 42;
    all_mesh_summaries[3].shattered_vertices_size = 1;
    
    // basic line (only 2 vertices)
    all_mesh_vertices->gpu_data[43].xyz[0]            = 0;
    all_mesh_vertices->gpu_data[43].xyz[1]            = 0;
    all_mesh_vertices->gpu_data[43].xyz[2]            = 0;
    all_mesh_vertices->gpu_data[43].uv[0]             = 0;
    all_mesh_vertices->gpu_data[43].uv[1]             = 0;
    all_mesh_vertices->gpu_data[43].normal_xyz[0]     =  0.0f;
    all_mesh_vertices->gpu_data[43].normal_xyz[1]     =  0.0f;
    all_mesh_vertices->gpu_data[43].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[43].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[44].xyz[0]            = 0;
    all_mesh_vertices->gpu_data[44].xyz[1]            = 0;
    all_mesh_vertices->gpu_data[44].xyz[2]            = 1;
    all_mesh_vertices->gpu_data[44].uv[0]             = 0;
    all_mesh_vertices->gpu_data[44].uv[1]             = 0;
    all_mesh_vertices->gpu_data[44].normal_xyz[0]     =  0.0f;
    all_mesh_vertices->gpu_data[44].normal_xyz[1]     =  0.0f;
    all_mesh_vertices->gpu_data[44].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[44].parent_material_i = 0;
    
    all_mesh_summaries_size = 4;
    all_mesh_vertices->size = 45;
    
    #define COLTEST 1
    #ifdef COLTEST
    // Let's hardcode a basic triangle for testing
    strcpy_capped(
        all_mesh_summaries[4].resource_name,
        OBJ_STRING_SIZE,
        "coltest_triangle");
    all_mesh_summaries[4].vertices_head_i = 45;
    all_mesh_summaries[4].vertices_size = 6;
    all_mesh_summaries[4].mesh_id = 4;
    all_mesh_summaries[4].materials_size = 1;
    all_mesh_summaries[4].base_width = 1.0f;
    all_mesh_summaries[4].base_height = 1.0f;
    all_mesh_summaries[4].base_depth = 1.0f;
    
    all_mesh_vertices->gpu_data[45].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[45].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[45].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[45].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[45].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[45].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[45].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[45].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[45].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[46].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[46].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[46].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[46].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[46].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[46].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[46].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[46].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[46].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[47].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[47].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[47].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[47].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[47].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[47].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[47].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[47].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[47].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[48].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[48].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[48].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[48].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[48].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[48].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[48].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[48].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[48].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[49].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[49].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[49].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[49].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[49].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[49].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[49].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[49].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[49].parent_material_i = 0;
    
    all_mesh_vertices->gpu_data[50].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[50].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[50].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[50].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[50].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[50].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[50].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[50].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[50].parent_material_i = 0;
    
    all_mesh_summaries_size = 5;
    all_mesh_vertices->size = 51;
    #endif
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
    log_assert(all_vertices_tail_i <= (int32_t)all_mesh_vertices->size);
}
#endif

static ParsedObj * parsed_obj = NULL;

int32_t new_mesh_id_from_obj_text(
    const char * obj_text,
    const uint32_t expected_materials_count,
    const char expected_materials_names[MAX_MATERIALS_PER_POLYGON][256])
{
    if (parsed_obj == NULL) {
        parsed_obj = malloc_from_unmanaged(sizeof(ParsedObj));
    }
    memset_char(parsed_obj, 0, sizeof(ParsedObj));
    
    uint32_t good = 0;
    parse_obj(
        /* ParsedObj * recipient: */
            parsed_obj,
        /* char * raw_buffer: */
            obj_text,
        /* uint32_t * success: */
            &good);
    log_assert(good);
    
    if (
        parsed_obj->vertices_count < 1 ||
        (parsed_obj->triangles_count + parsed_obj->quads_count) < 1)
    {
        good = 0;
    }
    log_assert(good);
    
    if (!good) {
        return -1;
    }
    
    int32_t new_mesh_head_id = (int32_t)all_mesh_vertices->size;
    all_mesh_summaries[all_mesh_summaries_size].vertices_head_i =
        new_mesh_head_id;
    
    log_assert(all_mesh_vertices->size < ALL_LOCKED_VERTICES_SIZE);
    
    all_mesh_summaries[all_mesh_summaries_size].materials_size =
        parsed_obj->materials_count < 1 ?
            1 : parsed_obj->materials_count;
    
    for (uint32_t i = 0; i < parsed_obj->materials_count; i++) {
        strcpy_capped(
            all_mesh_summaries[all_mesh_summaries_size].
                material_names[i],
            OBJ_STRING_SIZE,
            parsed_obj->materials[i].name);
    }
    
    if (parsed_obj->materials_count < 1) {
        parsed_obj->materials_count = 1;
    }
    
    for (
        uint32_t triangle_i = 0;
        triangle_i < parsed_obj->triangles_count;
        triangle_i++)
    {
        uint32_t cur_material_i = parsed_obj->triangles[triangle_i][4];
        log_assert(cur_material_i < parsed_obj->materials_count);
        
        uint32_t locked_vert_i = all_mesh_vertices->size;
        
        // We have to read all 3 positions first because we may need them to
        // infer the normals if the .obj file has no normals
        for (uint32_t _ = 0; _ < 3; _++) {
            uint32_t vert_i = parsed_obj->triangles[triangle_i][_];
            
            log_assert(vert_i >= 1);
            log_assert(vert_i <= parsed_obj->vertices_count);
            log_assert(
                locked_vert_i < ALL_LOCKED_VERTICES_SIZE);
            
            all_mesh_vertices->gpu_data[locked_vert_i + _].xyz[0] =
                parsed_obj->vertices[vert_i - 1][0];
            all_mesh_vertices->gpu_data[locked_vert_i + _].xyz[1] =
                parsed_obj->vertices[vert_i - 1][1];
            all_mesh_vertices->gpu_data[locked_vert_i + _].xyz[2] =
                parsed_obj->vertices[vert_i - 1][2];
        }
        
        for (uint32_t _ = 0; _ < 3; _++) {
            all_mesh_vertices->gpu_data[locked_vert_i + _].
                parent_material_i = cur_material_i;
            log_assert(cur_material_i >= 0);
            log_assert(cur_material_i < MAX_MATERIALS_PER_POLYGON);
            
            if (
                parsed_obj->normals_count > 0 &&
                parsed_obj->normals != NULL &&
                parsed_obj->triangle_normals != NULL)
            {
                uint32_t norm_i = parsed_obj->triangle_normals[triangle_i][_];
                
                log_assert(norm_i >= 1);
                log_assert(norm_i <= parsed_obj->normals_count);
                
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz[0] = parsed_obj->normals[norm_i - 1][0];
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz[1] = parsed_obj->normals[norm_i - 1][1];
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz[2] = parsed_obj->normals[norm_i - 1][2];
            } else if (_ == 0) {
                guess_gpu_triangle_normal(
                    /* GPULockedVertex * to_change: */
                        &all_mesh_vertices->gpu_data[locked_vert_i]);
                tok_memcpy(
                    all_mesh_vertices->gpu_data[locked_vert_i + 1].
                        normal_xyz,
                    all_mesh_vertices->gpu_data[locked_vert_i].
                        normal_xyz,
                    sizeof(float) * 3);
                tok_memcpy(
                    all_mesh_vertices->gpu_data[locked_vert_i + 2].
                        normal_xyz,
                    all_mesh_vertices->gpu_data[locked_vert_i].
                        normal_xyz,
                    sizeof(float) * 3);
            }
            
            #ifndef LOGGER_IGNORE_ASSERTS
            float check_x = all_mesh_vertices->gpu_data[
                locked_vert_i + _].normal_xyz[0];
            float check_y = all_mesh_vertices->gpu_data[
                locked_vert_i + _].normal_xyz[1];
            float check_z = all_mesh_vertices->gpu_data[
                locked_vert_i + _].normal_xyz[2];
            log_assert(
                fabs(check_x) + fabs(check_y) + fabs(check_z) > 0.3f);
            #endif
            
            normalize_zvertex_f3(
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz);
            
            if (parsed_obj->textures_count > 0) {
                uint32_t text_i = parsed_obj->triangle_textures[triangle_i][_];
                
                log_assert(text_i >= 1);
                log_assert(text_i <= parsed_obj->textures_count);
                
                all_mesh_vertices->gpu_data[locked_vert_i + _].uv[0] =
                    parsed_obj->textures[text_i - 1][0];
                
                all_mesh_vertices->gpu_data[locked_vert_i + _].uv[1] =
                    parsed_obj->textures[text_i - 1][1];
            } else {
                // No uv data in .obj file, gotta guess
                // TODO: Maybe should be part of the obj parser?
                all_mesh_vertices->gpu_data[locked_vert_i + _].uv[0] =
                    all_mesh_vertices->size % 2 == 0 ? 0.0f : 1.0f;
                all_mesh_vertices->gpu_data[locked_vert_i + _].uv[1] =
                    all_mesh_vertices->size % 4 == 0 ? 0.0f : 1.0f;
            }
            
        }
        all_mesh_vertices->size += 3;
    }
    
    for (
        uint32_t quad_i = 0;
        quad_i < parsed_obj->quads_count;
        quad_i++)
    {
        /*
        The division into two triangles would be one with the first 3 indices,
        and one with the first, third, and fourth. In this example:
        
        0 1 2
        0 2 3
        
        3-------2
        |      /|
        |    /  |
        |  /    |
        |/      |
        0-------1
        */
        
        uint32_t cur_material_i = parsed_obj->quads[quad_i][5];
        log_assert(cur_material_i < parsed_obj->materials_count);
        
        for (uint32_t offset = 0; offset < 2; offset++) {
            for (uint32_t _ = 0; _ < 3; _++) {
                uint32_t vert_i = parsed_obj->quads[quad_i]
                    [_ + ((_ > 0) * offset)];
                
                log_assert(vert_i >= 1);
                log_assert(vert_i <= parsed_obj->vertices_count);
                
                all_mesh_vertices->gpu_data[all_mesh_vertices->size].xyz[0] =
                    parsed_obj->vertices[vert_i - 1][0];
                all_mesh_vertices->gpu_data[all_mesh_vertices->size].xyz[1] =
                    parsed_obj->vertices[vert_i - 1][1];
                all_mesh_vertices->gpu_data[all_mesh_vertices->size].xyz[2] =
                    parsed_obj->vertices[vert_i - 1][2];
                
                all_mesh_vertices->gpu_data[all_mesh_vertices->size].
                    parent_material_i =
                        cur_material_i;
                
                if (parsed_obj->normals_count > 0) {
                    uint32_t norm_i = parsed_obj->quad_normals[quad_i]
                        [_ + offset];
                    
                    log_assert(norm_i >= 1);
                    log_assert(norm_i <= parsed_obj->normals_count);
                    
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].
                        normal_xyz[0] =
                            parsed_obj->normals[norm_i - 1][0];
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].
                        normal_xyz[1] =
                            parsed_obj->normals[norm_i - 1][1];
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].
                        normal_xyz[2] =
                            parsed_obj->normals[norm_i - 1][2];
                } else {
                    guess_gpu_triangle_normal(
                        /* GPULockedVertex * to_change: */
                            &all_mesh_vertices->gpu_data[
                                all_mesh_vertices->size]);
                }
                
                if (parsed_obj->textures_count > 0) {
                    uint32_t text_i = parsed_obj->quad_textures[quad_i]
                        [_ + offset];
                    
                    log_assert(text_i >= 1);
                    log_assert(text_i <= parsed_obj->textures_count);
                    
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].uv[0] =
                        parsed_obj->textures[text_i - 1][0];
                    
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].uv[1] =
                        parsed_obj->textures[text_i - 1][1];
                } else {
                    // No uv data in .obj file, gotta guess
                    // TODO: Maybe should be part of the obj parser?
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].uv[0] =
                        all_mesh_vertices->size % 2 == 0 ? 0.0f : 1.0f;
                    all_mesh_vertices->gpu_data[all_mesh_vertices->size].uv[1] =
                        all_mesh_vertices->size % 4 == 0 ? 0.0f : 1.0f;
                }
                
                all_mesh_vertices->size += 1;
            }
        }
    }
    
    if (expected_materials_names != NULL) {
        log_assert(parsed_obj->materials_count == expected_materials_count);
        for (uint32_t i = 0; i < parsed_obj->materials_count; i++) {
            log_assert(
                are_equal_strings(
                    parsed_obj->materials[i].name,
                    expected_materials_names[i]));
        }
    }
    free_obj(parsed_obj);
    
    all_mesh_summaries[all_mesh_summaries_size].mesh_id =
        (int32_t)all_mesh_summaries_size;
    all_mesh_summaries[all_mesh_summaries_size].vertices_size =
        (int32_t)all_mesh_vertices->size -
        all_mesh_summaries[all_mesh_summaries_size].vertices_head_i;
    log_assert(all_mesh_summaries[all_mesh_summaries_size].vertices_size > 0);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    uint32_t new_tail_i = (uint32_t)(
        all_mesh_summaries[all_mesh_summaries_size].vertices_head_i +
        all_mesh_summaries[all_mesh_summaries_size].vertices_size -
        1);
    log_assert(new_tail_i < all_mesh_vertices->size);
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
        tri_i < (int32_t)all_mesh_vertices->size;
        tri_i += 3)
    {
        for (int32_t m = 0; m < 3; m++) {
            if (min_x > all_mesh_vertices->gpu_data[tri_i + m].xyz[0]) {
                min_x = all_mesh_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (min_y > all_mesh_vertices->gpu_data[tri_i + m].xyz[1]) {
                min_y = all_mesh_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (min_z > all_mesh_vertices->gpu_data[tri_i + m].xyz[2]) {
                min_z = all_mesh_vertices->gpu_data[tri_i + m].xyz[2];
            }
            if (max_x < all_mesh_vertices->gpu_data[tri_i + m].xyz[0]) {
                max_x = all_mesh_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (max_y < all_mesh_vertices->gpu_data[tri_i + m].xyz[1]) {
                max_y = all_mesh_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (max_z < all_mesh_vertices->gpu_data[tri_i + m].xyz[2]) {
                max_z = all_mesh_vertices->gpu_data[tri_i + m].xyz[2];
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
    
    all_mesh_summaries_size += 1;
    log_assert(all_mesh_summaries_size <= ALL_MESHES_SIZE);
    
    #ifndef LOGGER_IGNORE_ASSERTS
    assert_objmodel_validity((int32_t)all_mesh_summaries_size - 1);
    #endif
    
    return (int32_t)all_mesh_summaries_size - 1;
}

int32_t new_mesh_id_from_resource_asserts(
    const char * filename,
    const uint32_t expected_materials_count,
    const char expected_materials_names[MAX_MATERIALS_PER_POLYGON][256])
{
    #ifdef LOGGER_IGNORE_ASSERTS
    (void)expected_materials_count;
    #endif
    
    log_assert(all_mesh_summaries_size < ALL_MESHES_SIZE);
    
    FileBuffer obj_file;
    obj_file.size_without_terminator = platform_get_resource_size(filename);
    log_assert(obj_file.size_without_terminator > 0);
    obj_file.contents = (char *)malloc_from_managed(obj_file.size_without_terminator + 1);
    obj_file.good = false;
    
    platform_read_resource_file(
        /* char * filename: */
            filename,
        /* FileBuffer *out_preallocatedbuffer: */
            &obj_file);
    
    log_assert(obj_file.good);
    
    int32_t new_mesh_id = new_mesh_id_from_obj_text(
        /* const char * obj_text: */
            obj_file.contents,
        /* const uint32_t expected_materials_count: */
            expected_materials_count,
        /* const char expected_materials_names[
            MAX_MATERIALS_PER_POLYGON][256]: */
            expected_materials_names);
    
    strcpy_capped(
        all_mesh_summaries[new_mesh_id].resource_name,
        OBJ_STRING_SIZE,
        filename);
    
    free_from_managed(obj_file.contents);
    obj_file.contents = NULL;
    
    return new_mesh_id;
}

int32_t new_mesh_id_from_resource(
    const char * filename)
{
    return new_mesh_id_from_resource_asserts(filename, 0, NULL);
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
            if (smallest_x > all_mesh_vertices->gpu_data[tri_i + m].xyz[0]) {
                smallest_x = all_mesh_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (largest_x < all_mesh_vertices->gpu_data[tri_i + m].xyz[0]) {
                largest_x = all_mesh_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (smallest_y > all_mesh_vertices->gpu_data[tri_i + m].xyz[1]) {
                smallest_y = all_mesh_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (largest_y < all_mesh_vertices->gpu_data[tri_i + m].xyz[1]) {
                largest_y = all_mesh_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (smallest_z > all_mesh_vertices->gpu_data[tri_i + m].xyz[2]) {
                smallest_z = all_mesh_vertices->gpu_data[tri_i + m].xyz[2];
            }
            if (largest_z < all_mesh_vertices->gpu_data[tri_i + m].xyz[2]) {
                largest_z = all_mesh_vertices->gpu_data[tri_i + m].xyz[2];
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
        all_mesh_vertices->gpu_data[vert_i].xyz[0] -= x_delta;
        all_mesh_vertices->gpu_data[vert_i].xyz[1] -= y_delta;
        all_mesh_vertices->gpu_data[vert_i].xyz[2] -= z_delta;
    }
}

static float get_squared_distance_from_locked_vertices(
    const GPULockedVertex a,
    const GPULockedVertex b)
{
    return
        ((a.xyz[0] - b.xyz[0]) * (a.xyz[0] - b.xyz[0])) +
        ((a.xyz[1] - b.xyz[1]) * (a.xyz[1] - b.xyz[1])) +
        ((a.xyz[2] - b.xyz[2]) * (a.xyz[2] - b.xyz[2]));
}

/* the largest length amongst any dimension be it x, y, or z */
static float get_squared_triangle_length_from_locked_vertices(
    const GPULockedVertex * vertices)
{
    log_assert(vertices[0].parent_material_i < MAX_MATERIALS_PER_POLYGON);
    log_assert(vertices[1].parent_material_i < MAX_MATERIALS_PER_POLYGON);
    log_assert(vertices[2].parent_material_i < MAX_MATERIALS_PER_POLYGON);
    
    float largest_squared_dist = FLOAT32_MIN;
    #ifndef LOGGER_IGNORE_ASSERTS
    int32_t largest_start_vertex_i = -1;
    int32_t largest_end_vertex_i = -1;
    #endif
    
    for (int32_t start_vertex_i = 0; start_vertex_i < 3; start_vertex_i++) {
        
        int32_t end_vertex_i = (start_vertex_i + 1) % 3;
        log_assert(start_vertex_i != end_vertex_i);
        log_assert(start_vertex_i <= 3);
        log_assert(end_vertex_i <= 3);
        
        float squared_x =
            ((vertices[start_vertex_i].xyz[0] -
                vertices[end_vertex_i].xyz[0]) *
            (vertices[start_vertex_i].xyz[0] -
                vertices[end_vertex_i].xyz[0]));
        float squared_y =
            ((vertices[start_vertex_i].xyz[1] -
                vertices[end_vertex_i].xyz[1]) *
            (vertices[start_vertex_i].xyz[1] -
                vertices[end_vertex_i].xyz[1]));
        float squared_z =
            ((vertices[start_vertex_i].xyz[2] -
                vertices[end_vertex_i].xyz[2]) *
            (vertices[start_vertex_i].xyz[2] -
                vertices[end_vertex_i].xyz[2]));
        
        float new_squared_dist =
            squared_x +
            squared_y +
            squared_z;
        
        log_assert(new_squared_dist > 0.0f);
        
        if (new_squared_dist > largest_squared_dist) {
            largest_squared_dist = new_squared_dist;
            #ifndef LOGGER_IGNORE_ASSERTS
            largest_start_vertex_i = start_vertex_i;
            largest_end_vertex_i = end_vertex_i;
            #endif
            log_assert(largest_start_vertex_i != largest_end_vertex_i);
        }
    }
    
    log_assert(largest_start_vertex_i != largest_end_vertex_i);
    
    return largest_squared_dist;
}

static int32_t find_biggest_area_triangle_head_in(
    int32_t head_vertex_i,
    int32_t tail_vertex_i)
{
    log_assert(tail_vertex_i > head_vertex_i);
    log_assert((tail_vertex_i - head_vertex_i) % 3 == 2);
    log_assert(all_mesh_vertices->gpu_data[tail_vertex_i].parent_material_i <
        MAX_MATERIALS_PER_POLYGON);
    log_assert(all_mesh_vertices->gpu_data[tail_vertex_i].parent_material_i <
        MAX_MATERIALS_PER_POLYGON);
    log_assert(all_mesh_vertices->gpu_data[tail_vertex_i].parent_material_i <
        MAX_MATERIALS_PER_POLYGON);
    
    float biggest_area = FLOAT32_MIN;
    int32_t biggest_area_i = -1;
    
    for (int32_t i = head_vertex_i; i < (tail_vertex_i - 1); i += 3) {
        float area = get_squared_triangle_length_from_locked_vertices(
            all_mesh_vertices->gpu_data + i);
        log_assert(area > 0.0f);
        if (area > biggest_area) {
            biggest_area = area;
            biggest_area_i = i;
        }
    }
    
    log_assert(biggest_area_i >= 0);
    return biggest_area_i;
}

void create_shattered_version_of_mesh(
    const int32_t mesh_id,
    const uint32_t triangles_multiplier)
{
    log_assert(triangles_multiplier >= 1);
    if (triangles_multiplier == 1) {
        all_mesh_summaries[mesh_id].shattered_vertices_size =
            all_mesh_summaries[mesh_id].vertices_size;
        all_mesh_summaries[mesh_id].shattered_vertices_head_i =
            all_mesh_summaries[mesh_id].vertices_head_i;
        return;
    }
    
    int32_t orig_head_i = all_mesh_summaries[mesh_id].vertices_head_i;
    #ifndef LOGGER_IGNORE_ASSERTS
    int32_t orig_tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
        all_mesh_summaries[mesh_id].vertices_size;
    #endif
    int32_t orig_vertices_size = all_mesh_summaries[mesh_id].vertices_size;
    
    int32_t new_head_i = (int32_t)all_mesh_vertices->size;
    
    all_mesh_summaries[mesh_id].shattered_vertices_head_i = new_head_i;
    all_mesh_summaries[mesh_id].shattered_vertices_size =
        all_mesh_summaries[mesh_id].vertices_size *
            (int32_t)triangles_multiplier;
    
    int32_t goal_new_tail_i =
        (int32_t)all_mesh_summaries[mesh_id].shattered_vertices_head_i +
        (int32_t)all_mesh_summaries[mesh_id].shattered_vertices_size -
        1;
    
    /*
    We will iterate through all triangles, finding the biggest one each time.
    Next, split the biggest one into 2, overwriting its original and appending
    the new one.
    This can be done once on app startup, since we preload all objs anyway.
    */
    
    // first, copy all of the original triangle vertices as they are
    int32_t temp_new_tail_i = new_head_i - 1;
    for (int32_t i = 0; i < orig_vertices_size; i += 3) {
        
        log_assert(orig_head_i + i <= orig_tail_i);
        all_mesh_vertices->gpu_data[new_head_i + i + 0] =
            all_mesh_vertices->gpu_data[orig_head_i + i + 0];
        all_mesh_vertices->gpu_data[new_head_i + i + 1] =
            all_mesh_vertices->gpu_data[orig_head_i + i + 1];
        all_mesh_vertices->gpu_data[new_head_i + i + 2] =
            all_mesh_vertices->gpu_data[orig_head_i + i + 2];
        
        temp_new_tail_i += 3;
        #ifndef LOGGER_IGNORE_ASSERTS
        log_assert(new_head_i + i <= temp_new_tail_i);
        
        float tri_length = get_squared_triangle_length_from_locked_vertices(
            &all_mesh_vertices->gpu_data[new_head_i + i]);
        log_assert(tri_length > 0);
        #endif
    }
    log_assert(temp_new_tail_i > new_head_i);
    log_assert((temp_new_tail_i - new_head_i) % 3 == 2);
    
    while (temp_new_tail_i < goal_new_tail_i) {
        
        // find the biggest triangle to split in 2
        int32_t biggest_area_head_i = find_biggest_area_triangle_head_in(
            new_head_i,
            temp_new_tail_i);
        log_assert(biggest_area_head_i >= new_head_i);
        log_assert((biggest_area_head_i - new_head_i) % 3 == 0);
        
        // find a 'middle line' that splits this triangle in 2
        int32_t midline_start_vert_i = 0;
        int32_t midline_end_vert_i = 1;
        
        #define USE_MIDLINE -1
        int32_t first_new_triangle_vertices[3];
        int32_t second_new_triangle_vertices[3];
        
        float distance_0_to_1 =
            get_squared_distance_from_locked_vertices(
                all_mesh_vertices->gpu_data[biggest_area_head_i + 0],
                all_mesh_vertices->gpu_data[biggest_area_head_i + 1]);
        float distance_1_to_2 =
            get_squared_distance_from_locked_vertices(
                all_mesh_vertices->gpu_data[biggest_area_head_i + 1],
                all_mesh_vertices->gpu_data[biggest_area_head_i + 2]);
        float distance_2_to_0 =
            get_squared_distance_from_locked_vertices(
                all_mesh_vertices->gpu_data[biggest_area_head_i + 2],
                all_mesh_vertices->gpu_data[biggest_area_head_i + 0]);
        
        log_assert(distance_0_to_1 > 0.0f);
        log_assert(distance_1_to_2 > 0.0f);
        log_assert(distance_2_to_0 > 0.0f);
        
        if (
            distance_0_to_1 >= distance_1_to_2 &&
            distance_0_to_1 >= distance_2_to_0)
        {
            /*
            Our triangle with vertices 0, 1, and 2, with 0-1 being the
            biggest line and 'M' splitting that line in the middle
            0    M     1
            ...........
            .      .
            .   .
            . .
            .
            2
            */
            midline_start_vert_i = 0;
            midline_end_vert_i = 1;
            
            // first new triangle will be 0-M-2
            first_new_triangle_vertices[0] = 0;
            first_new_triangle_vertices[1] = USE_MIDLINE;
            first_new_triangle_vertices[2] = 2;
            
            // and the second triangle will be M-1-2
            second_new_triangle_vertices[0] = USE_MIDLINE;
            second_new_triangle_vertices[1] = 1;
            second_new_triangle_vertices[2] = 2;
        } else if (
            distance_1_to_2 >= distance_2_to_0 &&
            distance_1_to_2 >= distance_0_to_1)
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
            
            midline_start_vert_i = 1;
            midline_end_vert_i = 2;
            
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
            
            midline_start_vert_i = 0;
            midline_end_vert_i = 2;
            
            first_new_triangle_vertices[0] = 0;
            first_new_triangle_vertices[1] = 1;
            first_new_triangle_vertices[2] = USE_MIDLINE;
            
            second_new_triangle_vertices[0] = USE_MIDLINE;
            second_new_triangle_vertices[1] = 1;
            second_new_triangle_vertices[2] = 2;
        }
        
        GPULockedVertex mid_of_line;
        mid_of_line.xyz[0] =
            (all_mesh_vertices->gpu_data[
                    biggest_area_head_i + midline_start_vert_i].xyz[0] +
                all_mesh_vertices->gpu_data[
                    biggest_area_head_i + midline_end_vert_i].xyz[0]) / 2;
        mid_of_line.xyz[1] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].xyz[1] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].xyz[1]) / 2;
        mid_of_line.xyz[2] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].xyz[2] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].xyz[2]) / 2;
        mid_of_line.normal_xyz[0] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].normal_xyz[0] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].normal_xyz[0]) / 2;
        mid_of_line.normal_xyz[1] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].normal_xyz[1] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].normal_xyz[1]) / 2;
        mid_of_line.normal_xyz[2] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].normal_xyz[2] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].normal_xyz[2]) / 2;
        mid_of_line.uv[0] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].uv[0] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].uv[0]) / 2;
        mid_of_line.uv[1] =
            (all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].uv[1] +
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].uv[1]) / 2;
        mid_of_line.parent_material_i =
            all_mesh_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].parent_material_i;
        
        // split the triangle at biggest_area_i into 2
        GPULockedVertex first_tri[3];
        GPULockedVertex second_tri[3];
        
        first_tri[0] = all_mesh_vertices->gpu_data[biggest_area_head_i + 0];
        first_tri[1] = all_mesh_vertices->gpu_data[biggest_area_head_i + 1];
        first_tri[2] = all_mesh_vertices->gpu_data[biggest_area_head_i + 2];
        
        second_tri[0] = all_mesh_vertices->gpu_data[biggest_area_head_i + 0];
        second_tri[1] = all_mesh_vertices->gpu_data[biggest_area_head_i + 1];
        second_tri[2] = all_mesh_vertices->gpu_data[biggest_area_head_i + 2];
        
        for (uint32_t m = 0; m < 3; m++) {
            
            if (first_new_triangle_vertices[m] == USE_MIDLINE) {
                first_tri[m] = mid_of_line;
            } else {
                log_assert(first_new_triangle_vertices[m] >= 0);
                log_assert(first_new_triangle_vertices[m] < 3);
                first_tri[m] =
                    all_mesh_vertices->gpu_data[
                        biggest_area_head_i + first_new_triangle_vertices[m]];
                log_assert(first_tri[m].parent_material_i <
                    MAX_MATERIALS_PER_POLYGON);
            }
            
            if (second_new_triangle_vertices[m] == USE_MIDLINE) {
                second_tri[m] = mid_of_line;
            } else {
                log_assert(second_new_triangle_vertices[m] >= 0);
                log_assert(second_new_triangle_vertices[m]  < 3);
                second_tri[m] =
                    all_mesh_vertices->gpu_data[
                        biggest_area_head_i + second_new_triangle_vertices[m]];
                log_assert(second_tri[m].parent_material_i <
                    MAX_MATERIALS_PER_POLYGON);
            }
        }
        
        #ifndef LOGGER_IGNORE_ASSERTS
        float orig_area =
            get_squared_triangle_length_from_locked_vertices(
                &all_mesh_vertices->gpu_data[biggest_area_head_i]);
        float first_tri_area =
            get_squared_triangle_length_from_locked_vertices(first_tri);
        float second_tri_area =
            get_squared_triangle_length_from_locked_vertices(second_tri);
        log_assert(orig_area > 0.0f);
        log_assert(first_tri_area > 0.0f);
        log_assert(second_tri_area > 0.0f);
        #endif
        
        all_mesh_vertices->gpu_data[biggest_area_head_i + 0] = first_tri[0];
        log_assert(
            all_mesh_vertices->gpu_data[biggest_area_head_i + 0].
                parent_material_i < MAX_MATERIALS_PER_POLYGON);
        all_mesh_vertices->gpu_data[biggest_area_head_i + 1] = first_tri[1];
        log_assert(
            all_mesh_vertices->gpu_data[biggest_area_head_i + 1].
                parent_material_i < MAX_MATERIALS_PER_POLYGON);
        all_mesh_vertices->gpu_data[biggest_area_head_i + 2] = first_tri[2];
        log_assert(
            all_mesh_vertices->gpu_data[biggest_area_head_i + 2].
                parent_material_i < MAX_MATERIALS_PER_POLYGON);
        #ifndef LOGGER_IGNORE_ASSERTS
        float overwritten_area =
            get_squared_triangle_length_from_locked_vertices(
                /* const GPULockedVertex * vertices: */
                    all_mesh_vertices->gpu_data + biggest_area_head_i);
        log_assert(overwritten_area > 0.0f);
        #endif
        
        all_mesh_vertices->gpu_data[temp_new_tail_i + 1] = second_tri[0];
        log_assert(
            all_mesh_vertices->gpu_data[temp_new_tail_i + 1].
                parent_material_i < MAX_MATERIALS_PER_POLYGON);
        all_mesh_vertices->gpu_data[temp_new_tail_i + 2] = second_tri[1];
        log_assert(
            all_mesh_vertices->gpu_data[temp_new_tail_i + 2].
                parent_material_i < MAX_MATERIALS_PER_POLYGON);
        all_mesh_vertices->gpu_data[temp_new_tail_i + 3] = second_tri[2];
        log_assert(
            all_mesh_vertices->gpu_data[temp_new_tail_i + 3].
                parent_material_i < MAX_MATERIALS_PER_POLYGON);
        #ifndef LOGGER_IGNORE_ASSERTS
        float new_area =
            get_squared_triangle_length_from_locked_vertices(
                /* const GPULockedVertex * vertices: */
                    all_mesh_vertices->gpu_data + temp_new_tail_i + 1);
        log_assert(new_area > 0.0f);
        #endif
        
        temp_new_tail_i += 3;
        log_assert(temp_new_tail_i <= goal_new_tail_i);
    }
    
    log_assert(all_mesh_vertices->size < (uint32_t)goal_new_tail_i);
    
    all_mesh_vertices->size = (uint32_t)goal_new_tail_i + 1;
}
