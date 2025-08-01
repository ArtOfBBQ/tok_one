#include "T1_objmodel.h"

MeshSummary * all_mesh_summaries;
uint32_t all_mesh_summaries_size;

static void construct_mesh_summary(
    MeshSummary * to_construct,
    const int32_t id)
{
    to_construct->resource_name[0]            = '\0';
    to_construct->mesh_id                     =   id;
    to_construct->vertices_head_i             =   -1; // index @ all_mesh_vertices
    to_construct->vertices_size               =    0;
    to_construct->base_width                  = 0.0f;
    to_construct->base_height                 = 0.0f;
    to_construct->base_depth                  = 0.0f;
    to_construct->shattered_vertices_head_i   =   -1;
    to_construct->shattered_vertices_size     =    0;
    to_construct->locked_material_base_offset = UINT32_MAX;
    to_construct->materials_size              =    0;
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
    
    float return_value = sqrtf(sum_squares);
    
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

static ParsedObj * parsed_obj = NULL;

void T1_objmodel_init(void) {
    parsed_obj = malloc_from_unmanaged(sizeof(ParsedObj));
    common_memset_char(parsed_obj, 0, sizeof(ParsedObj));
    
    all_mesh_summaries = (MeshSummary *)malloc_from_unmanaged(
        sizeof(MeshSummary) * ALL_MESHES_SIZE);
    
    for (uint32_t i = 0; i < ALL_MESHES_SIZE; i++) {
        construct_mesh_summary(&all_mesh_summaries[i], (int32_t)i);
    }
    
    assert(ALL_LOCKED_VERTICES_SIZE > 0);
    all_mesh_vertices = (LockedVertexWithMaterialCollection *)
        malloc_from_unmanaged(sizeof(LockedVertexWithMaterialCollection));
    common_memset_char(
        all_mesh_vertices,
        0,
        sizeof(LockedVertexWithMaterialCollection));
    
    // Let's hardcode a basic quad since that's a mesh that will be used by
    // even the features inherent to the engine itself (the terminal, any
    // text labels, the FPS label, etc)
    common_strcpy_capped(
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
    all_mesh_vertices->gpu_data[0].parent_material_i      = PARENT_MATERIAL_BASE;
    // top right vertex
    all_mesh_vertices->gpu_data[1].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[1].xyz[1]                 = top_vertex;
    all_mesh_vertices->gpu_data[1].xyz[2]                 = 0.0f;
    all_mesh_vertices->gpu_data[1].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[1].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[1].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[1].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[1].uv[1]                  = top_uv_coord;
    all_mesh_vertices->gpu_data[1].parent_material_i      = UINT32_MAX;
    // bottom left vertex
    all_mesh_vertices->gpu_data[2].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[2].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[2].xyz[2]            = 0.0f;
    all_mesh_vertices->gpu_data[2].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[2].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[2].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[2].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[2].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[2].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic quad, triangle 2 
    // top right vertex
    all_mesh_vertices->gpu_data[3].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[3].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[3].xyz[2]            = 0.0f;
    all_mesh_vertices->gpu_data[3].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[3].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[3].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[3].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[3].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[3].parent_material_i = PARENT_MATERIAL_BASE;
    // bottom right vertex
    all_mesh_vertices->gpu_data[4].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[4].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[4].xyz[2]            = 0.0f;
    all_mesh_vertices->gpu_data[4].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[4].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[4].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[4].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[4].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[4].parent_material_i = PARENT_MATERIAL_BASE;
    // bottom left vertex
    all_mesh_vertices->gpu_data[5].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[5].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[5].xyz[2]            = 0.0f;
    all_mesh_vertices->gpu_data[5].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[5].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[5].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[5].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[5].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[5].parent_material_i = PARENT_MATERIAL_BASE;
    
    // Let's hardcode a basic cube since that will be used by the particle
    // effects system
    common_strcpy_capped(
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
    all_mesh_vertices->gpu_data[6].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[6].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[6].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[6].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[6].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[6].normal_xyz[0]     =  0.0f;
    all_mesh_vertices->gpu_data[6].normal_xyz[1]     =  0.0f;
    all_mesh_vertices->gpu_data[6].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[6].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[7].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[7].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[7].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[7].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[7].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[7].normal_xyz[0]     =  0.0f;
    all_mesh_vertices->gpu_data[7].normal_xyz[1]     =  0.0f;
    all_mesh_vertices->gpu_data[7].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[7].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[8].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[8].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[8].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[8].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[8].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[8].normal_xyz[0]     =  0.0f;
    all_mesh_vertices->gpu_data[8].normal_xyz[1]     =  0.0f;
    all_mesh_vertices->gpu_data[8].normal_xyz[2]     = -1.0f;
    all_mesh_vertices->gpu_data[8].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, front face triangle  2
    all_mesh_vertices->gpu_data[9].xyz[0]                  = right_vertex;
    all_mesh_vertices->gpu_data[9].xyz[1]                  = top_vertex;
    all_mesh_vertices->gpu_data[9].xyz[2]                  = front_vertex;
    all_mesh_vertices->gpu_data[9].uv[0]                   = right_uv_coord;
    all_mesh_vertices->gpu_data[9].uv[1]                   = top_uv_coord;
    all_mesh_vertices->gpu_data[9].normal_xyz[0]           = 0.0f;
    all_mesh_vertices->gpu_data[9].normal_xyz[1]           = 0.0f;
    all_mesh_vertices->gpu_data[9].normal_xyz[2]           = -1.0f;
    all_mesh_vertices->gpu_data[9].parent_material_i       = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[10].xyz[0]                 = right_vertex;
    all_mesh_vertices->gpu_data[10].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[10].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[10].uv[0]                  = right_uv_coord;
    all_mesh_vertices->gpu_data[10].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[10].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[10].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[10].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[10].parent_material_i      = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[11].xyz[0]                 = left_vertex;
    all_mesh_vertices->gpu_data[11].xyz[1]                 = bottom_vertex;
    all_mesh_vertices->gpu_data[11].xyz[2]                 = front_vertex;
    all_mesh_vertices->gpu_data[11].uv[0]                  = left_uv_coord;
    all_mesh_vertices->gpu_data[11].uv[1]                  = bottom_uv_coord;
    all_mesh_vertices->gpu_data[11].normal_xyz[0]          = 0.0f;
    all_mesh_vertices->gpu_data[11].normal_xyz[1]          = 0.0f;
    all_mesh_vertices->gpu_data[11].normal_xyz[2]          = -1.0f;
    all_mesh_vertices->gpu_data[11].parent_material_i      = PARENT_MATERIAL_BASE;
    
    // basic cube, back face triangle 1
    all_mesh_vertices->gpu_data[12].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[12].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[12].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[12].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[12].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[12].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[12].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[12].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[12].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[13].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[13].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[13].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[13].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[13].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[13].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[13].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[13].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[13].parent_material_i = PARENT_MATERIAL_BASE;

    all_mesh_vertices->gpu_data[14].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[14].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[14].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[14].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[14].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[14].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[14].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[14].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[14].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, back face triangle 2
    all_mesh_vertices->gpu_data[15].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[15].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[15].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[15].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[15].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[15].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[15].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[15].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[15].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[16].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[16].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[16].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[16].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[16].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[16].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[16].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[16].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[16].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[17].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[17].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[17].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[17].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[17].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[17].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[17].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[17].normal_xyz[2]     = 1.0f;
    all_mesh_vertices->gpu_data[17].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, left face triangle 1
    all_mesh_vertices->gpu_data[18].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[18].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[18].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[18].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[18].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[18].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[18].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[18].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[18].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[19].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[19].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[19].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[19].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[19].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[19].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[19].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[19].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[19].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[20].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[20].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[20].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[20].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[20].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[20].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[20].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[20].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[20].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, left face triangle 2
    all_mesh_vertices->gpu_data[21].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[21].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[21].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[21].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[21].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[21].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[21].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[21].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[21].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[22].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[22].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[22].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[22].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[22].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[22].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[22].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[22].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[22].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[23].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[23].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[23].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[23].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[23].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[23].normal_xyz[0]     = -1.0f;
    all_mesh_vertices->gpu_data[23].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[23].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[23].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, right face triangle 1
    all_mesh_vertices->gpu_data[24].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[24].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[24].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[24].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[24].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[24].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[24].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[24].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[24].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[25].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[25].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[25].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[25].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[25].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[25].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[25].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[25].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[25].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[26].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[26].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[26].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[26].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[26].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[26].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[26].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[26].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[26].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, right face triangle 2
    all_mesh_vertices->gpu_data[27].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[27].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[27].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[27].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[27].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[27].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[27].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[27].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[27].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[28].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[28].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[28].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[28].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[28].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[28].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[28].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[28].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[28].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[29].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[29].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[29].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[29].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[29].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[29].normal_xyz[0]     = 1.0f;
    all_mesh_vertices->gpu_data[29].normal_xyz[1]     = 0.0f;
    all_mesh_vertices->gpu_data[29].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[29].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, top face triangle 1
    all_mesh_vertices->gpu_data[30].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[30].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[30].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[30].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[30].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[30].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[30].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[30].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[30].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[31].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[31].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[31].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[31].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[31].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[31].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[31].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[31].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[31].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[32].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[32].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[32].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[32].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[32].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[32].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[32].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[32].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[32].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, top face triangle 2
    all_mesh_vertices->gpu_data[33].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[33].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[33].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[33].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[33].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[33].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[33].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[33].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[33].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[34].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[34].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[34].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[34].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[34].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[34].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[34].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[34].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[34].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[35].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[35].xyz[1]            = top_vertex;
    all_mesh_vertices->gpu_data[35].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[35].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[35].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[35].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[35].normal_xyz[1]     = 1.0f;
    all_mesh_vertices->gpu_data[35].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[35].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, bottom face triangle 1
    all_mesh_vertices->gpu_data[36].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[36].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[36].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[36].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[36].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[36].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[36].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[36].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[36].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[37].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[37].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[37].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[37].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[37].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[37].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[37].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[37].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[37].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[38].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[38].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[38].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[38].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[38].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[38].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[38].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[38].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[38].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, bottom face triangle 2
    all_mesh_vertices->gpu_data[39].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[39].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[39].xyz[2]            = back_vertex;
    all_mesh_vertices->gpu_data[39].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[39].uv[1]             = top_uv_coord;
    all_mesh_vertices->gpu_data[39].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[39].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[39].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[39].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[40].xyz[0]            = right_vertex;
    all_mesh_vertices->gpu_data[40].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[40].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[40].uv[0]             = right_uv_coord;
    all_mesh_vertices->gpu_data[40].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[40].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[40].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[40].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[40].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_vertices->gpu_data[41].xyz[0]            = left_vertex;
    all_mesh_vertices->gpu_data[41].xyz[1]            = bottom_vertex;
    all_mesh_vertices->gpu_data[41].xyz[2]            = front_vertex;
    all_mesh_vertices->gpu_data[41].uv[0]             = left_uv_coord;
    all_mesh_vertices->gpu_data[41].uv[1]             = bottom_uv_coord;
    all_mesh_vertices->gpu_data[41].normal_xyz[0]     = 0.0f;
    all_mesh_vertices->gpu_data[41].normal_xyz[1]     = -1.0f;
    all_mesh_vertices->gpu_data[41].normal_xyz[2]     = 0.0f;
    all_mesh_vertices->gpu_data[41].parent_material_i = PARENT_MATERIAL_BASE;
    
    all_mesh_summaries_size = 2;
    all_mesh_vertices->size = 42;
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

static int32_t new_mesh_id_from_parsed_obj_and_parsed_materials(
     const char * original_obj_filename,
     ParsedObj * arg_parsed_obj,
     ParsedMaterial * parsed_materials,
     const uint32_t parsed_materials_size)
{
    float invert_z_axis_modifier = -1.0f;
    
    int32_t new_mesh_head_id =
        (int32_t)all_mesh_vertices->size;
    all_mesh_summaries[all_mesh_summaries_size].vertices_head_i =
        new_mesh_head_id;
    
    log_assert(all_mesh_vertices->size < ALL_LOCKED_VERTICES_SIZE);
    
    all_mesh_summaries[all_mesh_summaries_size].materials_size =
        arg_parsed_obj->materials_count;
    
    uint32_t first_material_head_i =
        T1_material_preappend_locked_material_i(
            original_obj_filename,
            parsed_obj->material_names == NULL ?
                "default" :
                parsed_obj->material_names[0].name);
    
    if (parsed_obj->materials_count > 0) {
        // Preregister all materials, starting with the index of the head
        all_mesh_summaries[all_mesh_summaries_size].
            locked_material_head_i = first_material_head_i;
        
        for (uint32_t i = 1; i < arg_parsed_obj->materials_count; i++) {
            uint32_t _ = T1_material_preappend_locked_material_i(
                original_obj_filename,
                parsed_obj->material_names[i].name);
            
            log_assert(first_material_head_i + i == _);
            (void)_;
        }
        
        // Fill in data for each material
        for (uint32_t i = 0; i < arg_parsed_obj->materials_count; i++) {
            int32_t matching_parsed_materials_i = -1;
            for (int32_t j = 0; j < (int32_t)parsed_materials_size; j++) {
                if (
                    common_are_equal_strings(
                        arg_parsed_obj->material_names[i].name,
                        parsed_materials[j].name))
                {
                    matching_parsed_materials_i = j;
                }
            }
            
            log_assert(
                matching_parsed_materials_i  < (int32_t)parsed_materials_size);
            
            GPULockedMaterial * locked_mat = T1_material_fetch_ptr(
                /* const uint32_t locked_material_i: */
                    all_mesh_summaries[all_mesh_summaries_size].
                        locked_material_head_i + i);
            
            T1_material_construct(locked_mat);
            
            if (matching_parsed_materials_i >= 0) {
                locked_mat->ambient_rgb[0] =
                    parsed_materials[matching_parsed_materials_i].ambient_rgb[0];
                locked_mat->ambient_rgb[1] =
                    parsed_materials[matching_parsed_materials_i].ambient_rgb[1];
                locked_mat->ambient_rgb[2] =
                    parsed_materials[matching_parsed_materials_i].ambient_rgb[2];
                
                locked_mat->alpha =
                    parsed_materials[matching_parsed_materials_i].alpha;
                
                locked_mat->diffuse_rgb[0] =
                    parsed_materials[matching_parsed_materials_i].diffuse_rgb[0];
                locked_mat->diffuse_rgb[1] =
                    parsed_materials[matching_parsed_materials_i].diffuse_rgb[1];
                locked_mat->diffuse_rgb[2] =
                    parsed_materials[matching_parsed_materials_i].diffuse_rgb[2];
                
                locked_mat->specular_rgb[0] =
                    parsed_materials[matching_parsed_materials_i].specular_rgb[0];
                locked_mat->specular_rgb[1] =
                    parsed_materials[matching_parsed_materials_i].specular_rgb[1];
                locked_mat->specular_rgb[2] =
                    parsed_materials[matching_parsed_materials_i].specular_rgb[2];
                
                locked_mat->specular_exponent =
                    parsed_materials[matching_parsed_materials_i].
                        specular_exponent;
                
                locked_mat->illum = 1.0f;
                
                locked_mat->uv_scroll[0] = parsed_materials[matching_parsed_materials_i].
                        T1_uv_scroll[0];
                locked_mat->uv_scroll[1] = parsed_materials[matching_parsed_materials_i].
                        T1_uv_scroll[1];
                
                T1_texture_array_get_filename_location(
                    /* const char * for_filename: */
                        parsed_materials[matching_parsed_materials_i].diffuse_map,
                    /* int32_t * texture_array_i_recipient: */
                        &locked_mat->texturearray_i,
                    /* int32_t * texture_i_recipient: */
                        &locked_mat->texture_i);
                if (
                    parsed_materials[matching_parsed_materials_i].
                        diffuse_map[0] != '\0')
                {
                    log_append("WARNING: missing material texture: ");
                    log_append(parsed_materials[matching_parsed_materials_i].diffuse_map);
                    log_append(" in object: ");
                    log_append(original_obj_filename);
                    log_append("\n");
                    
                    if (
                        locked_mat->texturearray_i < 0 ||
                        locked_mat->texture_i < 0)
                    {
                        char errmsg[128];
                        common_strcpy_capped(errmsg, 128, "Missing material texture: ");
                        common_strcat_capped(
                            errmsg,
                            128,
                            parsed_materials[matching_parsed_materials_i].
                                diffuse_map);
                        log_dump_and_crash(errmsg);
                        return -1;
                    }
                } else {
                    log_assert(locked_mat->texturearray_i < 0);
                    log_assert(locked_mat->texture_i < 0);
                }
                
                #if NORMAL_MAPPING_ACTIVE
                T1_texture_array_get_filename_location(
                    /* const char * for_filename: */
                        parsed_materials[matching_parsed_materials_i].bump_or_normal_map,
                    /* int32_t * texture_array_i_recipient: */
                        &locked_mat->normalmap_texturearray_i,
                    /* int32_t * texture_i_recipient: */
                        &locked_mat->normalmap_texture_i);
                #endif
            }
        }
    }
    
    for (
        uint32_t triangle_i = 0;
        triangle_i < arg_parsed_obj->triangles_count;
        triangle_i++)
    {
        uint32_t cur_material_i = arg_parsed_obj->triangles[triangle_i][4];
        
        if (parsed_materials != NULL) {
            // check if the cur_material_i is the "use base material"
            // material
            int32_t parsed_mtls_matching_i = -1;
            for (int32_t i = 0; i < (int32_t)parsed_materials_size; i++) {
                if (
                    common_are_equal_strings(
                        arg_parsed_obj->material_names[cur_material_i].name,
                        parsed_materials[i].name))
                {
                    parsed_mtls_matching_i = i;
                }
            }
        
            if (parsed_materials[parsed_mtls_matching_i].use_base_mtl_flag)
            {
                cur_material_i = PARENT_MATERIAL_BASE;
                #ifndef LOGGER_IGNORE_ASSERTS
                
                #endif
                all_mesh_summaries[all_mesh_summaries_size].
                    locked_material_base_offset =
                        (uint32_t)cur_material_i;
            }
        }
        
        uint32_t locked_vert_i = all_mesh_vertices->size;
        
        // We have to read all 3 positions first because we may need them to
        // infer the normals if the .obj file has no normals
        for (uint32_t _ = 0; _ < 3; _++) {
            uint32_t vert_i = arg_parsed_obj->triangles[triangle_i][_];
            
            log_assert(vert_i >= 1);
            log_assert(vert_i <= arg_parsed_obj->vertices_count);
            log_assert(
                locked_vert_i < ALL_LOCKED_VERTICES_SIZE);
            
            all_mesh_vertices->gpu_data[locked_vert_i + _].xyz[0] =
                arg_parsed_obj->vertices[vert_i - 1][0];
            all_mesh_vertices->gpu_data[locked_vert_i + _].xyz[1] =
                arg_parsed_obj->vertices[vert_i - 1][1];
            all_mesh_vertices->gpu_data[locked_vert_i + _].xyz[2] =
                arg_parsed_obj->vertices[vert_i - 1][2] *
                invert_z_axis_modifier;
        }
        
        for (uint32_t _ = 0; _ < 3; _++) {
            all_mesh_vertices->gpu_data[locked_vert_i + _].
                parent_material_i = cur_material_i;
            all_mesh_vertices->gpu_data[locked_vert_i + _].
                locked_materials_head_i =
                    first_material_head_i;
            
            if (
                arg_parsed_obj->normals_count > 0 &&
                arg_parsed_obj->normals_vn != NULL &&
                arg_parsed_obj->triangle_normals != NULL)
            {
                uint32_t norm_i = arg_parsed_obj->triangle_normals[triangle_i][_];
                
                log_assert(norm_i >= 1);
                log_assert(norm_i <= arg_parsed_obj->normals_count);
                
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz[0] = arg_parsed_obj->normals_vn[norm_i - 1][0];
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz[1] = arg_parsed_obj->normals_vn[norm_i - 1][1];
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz[2] =
                        arg_parsed_obj->normals_vn[norm_i - 1][2] *
                        invert_z_axis_modifier;
            } else if (_ == 0) {
                log_assert(0); // check if deduced normals need to invert z
                guess_gpu_triangle_normal(
                    /* GPULockedVertex * to_change: */
                        &all_mesh_vertices->gpu_data[locked_vert_i]);
                common_memcpy(
                    all_mesh_vertices->gpu_data[locked_vert_i + 1].
                        normal_xyz,
                    all_mesh_vertices->gpu_data[locked_vert_i].
                        normal_xyz,
                    sizeof(float) * 3);
                common_memcpy(
                    all_mesh_vertices->gpu_data[locked_vert_i + 2].
                        normal_xyz,
                    all_mesh_vertices->gpu_data[locked_vert_i].
                        normal_xyz,
                    sizeof(float) * 3);
            }
            
            normalize_zvertex_f3(
                all_mesh_vertices->gpu_data[locked_vert_i + _].
                    normal_xyz);
            
            if (arg_parsed_obj->textures_count > 0) {
                uint32_t text_i = arg_parsed_obj->triangle_textures[triangle_i][_];
                
                log_assert(text_i >= 1);
                log_assert(text_i <= arg_parsed_obj->textures_count);
                
                all_mesh_vertices->gpu_data[locked_vert_i + _].uv[0] =
                    arg_parsed_obj->textures_vt_uv[text_i - 1][0];
                
                all_mesh_vertices->gpu_data[locked_vert_i + _].uv[1] =
                    arg_parsed_obj->textures_vt_uv[text_i - 1][1];
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
        quad_i < arg_parsed_obj->quads_count;
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
        
        uint32_t cur_material_i = arg_parsed_obj->quads[quad_i][5];
        log_assert(cur_material_i < arg_parsed_obj->materials_count);
        
        for (uint32_t quad_tri_i = 0; quad_tri_i < 2; quad_tri_i++) {
            
            GPULockedVertex * V1 = &all_mesh_vertices->gpu_data[all_mesh_vertices->size];
            GPULockedVertex * V2 = &all_mesh_vertices->gpu_data[all_mesh_vertices->size+1];
            GPULockedVertex * V3 = &all_mesh_vertices->gpu_data[all_mesh_vertices->size+2];
            
            uint32_t vert_1_i;
            uint32_t vert_2_i;
            uint32_t vert_3_i;
            uint32_t norm_1_i;
            uint32_t norm_2_i;
            uint32_t norm_3_i;
            uint32_t textr_1_i = 1;
            uint32_t textr_2_i = 1;
            uint32_t textr_3_i = 1;
            
            if (quad_tri_i == 0) {
                // First subtriangle of our quad
                vert_1_i = arg_parsed_obj->quads[quad_i][0];
                vert_2_i = arg_parsed_obj->quads[quad_i][1];
                vert_3_i = arg_parsed_obj->quads[quad_i][2];
                norm_1_i = arg_parsed_obj->quad_normals[quad_i][0];
                norm_2_i = arg_parsed_obj->quad_normals[quad_i][1];
                norm_3_i = arg_parsed_obj->quad_normals[quad_i][2];
                if (arg_parsed_obj->quad_textures) {
                    textr_1_i = arg_parsed_obj->quad_textures[quad_i][0];
                    textr_2_i = arg_parsed_obj->quad_textures[quad_i][1];
                    textr_3_i = arg_parsed_obj->quad_textures[quad_i][2];    
                }
            } else {
                // Second subtriangle of our quad
                log_assert(quad_tri_i == 1);
                vert_1_i = arg_parsed_obj->quads[quad_i][0];
                vert_2_i = arg_parsed_obj->quads[quad_i][2];
                vert_3_i = arg_parsed_obj->quads[quad_i][3];
                norm_1_i = arg_parsed_obj->quad_normals[quad_i][0];
                norm_2_i = arg_parsed_obj->quad_normals[quad_i][2];
                norm_3_i = arg_parsed_obj->quad_normals[quad_i][3];
                if (arg_parsed_obj->quad_textures) {
                    textr_1_i = arg_parsed_obj->quad_textures[quad_i][0];
                    textr_2_i = arg_parsed_obj->quad_textures[quad_i][2];
                    textr_3_i = arg_parsed_obj->quad_textures[quad_i][3];
                }
            }
            
            log_assert(vert_1_i >= 1);
            log_assert(vert_1_i <= arg_parsed_obj->vertices_count);
            log_assert(vert_2_i >= 1);
            log_assert(vert_2_i <= arg_parsed_obj->vertices_count);
            log_assert(vert_3_i >= 1);
            log_assert(vert_3_i <= arg_parsed_obj->vertices_count);
            
            vert_1_i  -= 1;
            vert_2_i  -= 1;
            vert_3_i  -= 1;
            norm_1_i  -= 1;
            norm_2_i  -= 1;
            norm_3_i  -= 1;
            textr_1_i -= 1;
            textr_2_i -= 1;
            textr_3_i -= 1;
            
            log_assert(all_mesh_vertices->size < ALL_LOCKED_VERTICES_SIZE);
            
            if (all_mesh_vertices->size >= ALL_LOCKED_VERTICES_SIZE) {
                return -1;
            }
            
            V1->xyz[0] = arg_parsed_obj->vertices[vert_1_i][0];
            V1->xyz[1] = arg_parsed_obj->vertices[vert_1_i][1];
            V1->xyz[2] =
                arg_parsed_obj->vertices[vert_1_i][2] *
                invert_z_axis_modifier;
            V2->xyz[0] = arg_parsed_obj->vertices[vert_2_i][0];
            V2->xyz[1] = arg_parsed_obj->vertices[vert_2_i][1];
            V2->xyz[2] =
                arg_parsed_obj->vertices[vert_2_i][2] *
                invert_z_axis_modifier;
            V3->xyz[0] = arg_parsed_obj->vertices[vert_3_i][0];
            V3->xyz[1] = arg_parsed_obj->vertices[vert_3_i][1];
            V3->xyz[2] =
                arg_parsed_obj->vertices[vert_3_i][2] *
                invert_z_axis_modifier;
            
            V1->parent_material_i       = cur_material_i;
            V1->locked_materials_head_i = first_material_head_i;
            V2->parent_material_i       = cur_material_i;
            V2->locked_materials_head_i = first_material_head_i;
            V3->parent_material_i       = cur_material_i;
            V3->locked_materials_head_i = first_material_head_i;
            
            if (arg_parsed_obj->normals_count > 0) {
                #ifndef LOGGER_IGNORE_ASSERTS
                uint32_t norm_i = arg_parsed_obj->quad_normals[quad_i]
                    [quad_tri_i];
                
                log_assert(norm_i >= 1);
                log_assert(norm_i <= arg_parsed_obj->normals_count);
                #endif
                
                V1->normal_xyz[0] = arg_parsed_obj->normals_vn[norm_1_i][0];
                V1->normal_xyz[1] = arg_parsed_obj->normals_vn[norm_1_i][1];
                V1->normal_xyz[2] = arg_parsed_obj->normals_vn[norm_1_i][2] *
                    invert_z_axis_modifier;
                V2->normal_xyz[0] = arg_parsed_obj->normals_vn[norm_2_i][0];
                V2->normal_xyz[1] = arg_parsed_obj->normals_vn[norm_2_i][1];
                V2->normal_xyz[2] = arg_parsed_obj->normals_vn[norm_2_i][2] *
                    invert_z_axis_modifier;
                V3->normal_xyz[0] = arg_parsed_obj->normals_vn[norm_3_i][0];
                V3->normal_xyz[1] = arg_parsed_obj->normals_vn[norm_3_i][1];
                V3->normal_xyz[2] = arg_parsed_obj->normals_vn[norm_3_i][2] *
                    invert_z_axis_modifier;
            } else {
                guess_gpu_triangle_normal(V1);
                guess_gpu_triangle_normal(V2);
                guess_gpu_triangle_normal(V3);
            }
            
            if (arg_parsed_obj->textures_count > 0) {
                log_assert(arg_parsed_obj->textures_vt_uv != NULL);
                V1->uv[0] = arg_parsed_obj->textures_vt_uv[textr_1_i][0];
                V1->uv[1] = arg_parsed_obj->textures_vt_uv[textr_1_i][1];
                V2->uv[0] = arg_parsed_obj->textures_vt_uv[textr_2_i][0];
                V2->uv[1] = arg_parsed_obj->textures_vt_uv[textr_2_i][1];
                V3->uv[0] = arg_parsed_obj->textures_vt_uv[textr_3_i][0];
                V3->uv[1] = arg_parsed_obj->textures_vt_uv[textr_3_i][1];
            } else {
                // No uv data in .obj file, gotta guess
                // TODO: Maybe should be part of the obj parser?
                V1->uv[0] = 0.0f;
                V1->uv[1] = 0.0f;
                V2->uv[0] = 0.0f;
                V2->uv[1] = 0.0f;
                V3->uv[0] = 0.0f;
                V3->uv[1] = 0.0f;
            }
            
            all_mesh_vertices->size += 3;
        }
    }
    
    T1_objparser_deinit(arg_parsed_obj);
    
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

int32_t T1_objmodel_new_mesh_id_from_obj_mtl_text(
    const char * original_obj_filename,
    const char * obj_text,
    const char * mtl_text)
{
    log_assert(parsed_obj != NULL);
    
    uint32_t good = 0;
    T1_objparser_parse(
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
    
    if (mtl_text == NULL || mtl_text[0] == '\0') {
        return new_mesh_id_from_parsed_obj_and_parsed_materials(
            original_obj_filename,
            parsed_obj,
            NULL,
            0);
    }
    
    good = 0;
    
    uint32_t parsed_materials_cap = 20;
    ParsedMaterial * parsed_materials = malloc_from_managed(
        sizeof(ParsedMaterial) * parsed_materials_cap);
    common_memset_char(
        parsed_materials,
        0,
        sizeof(ParsedMaterial) * parsed_materials_cap);
    uint32_t parsed_materials_size = 0;
    
    mtlparser_parse(
        /* ParsedMaterial * recipient: */
            parsed_materials,
        /* uint32_t * recipient_size: */
            &parsed_materials_size,
        /* const uint32_t recipient_cap: */
            parsed_materials_cap,
        /* const char * input: */
            mtl_text,
        /* uint32_t * good: */
            &good);
    
    if (!good) {
        #ifndef LOGGER_IGNORE_ASSERTS
        log_dump_and_crash(mtlparser_get_last_error_msg());
        #endif
        return -1;
    }
    
    return new_mesh_id_from_parsed_obj_and_parsed_materials(
        original_obj_filename,
        parsed_obj,
        parsed_materials,
        parsed_materials_size);
}

static void T1_objmodel_deduce_tangents_and_bitangents(
    const int32_t mesh_id)
{
    int32_t tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
            all_mesh_summaries[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i += 3)
    {
        for (int32_t offset = 0; offset < 3; offset++) {
        
        GPULockedVertex * v1 = &all_mesh_vertices->
            gpu_data[vert_i+((offset+0) % 3)];
        GPULockedVertex * v2 = &all_mesh_vertices->
            gpu_data[vert_i+((offset+1) % 3)];
        GPULockedVertex * v3 = &all_mesh_vertices->
            gpu_data[vert_i+((offset+2) % 3)];
        
        /*
        Compute edge vectors in world space and UV space:
        ΔP12 = P2 - P1, ΔP13 = P3 - P1.
        ΔUV12 = uv2 - uv1, ΔUV13 = uv3 - uv1.
        */
        float vec_1_to_2[3];
        vec_1_to_2[0] = v2->xyz[0] - v1->xyz[0];
        vec_1_to_2[1] = v2->xyz[1] - v1->xyz[1];
        vec_1_to_2[2] = v2->xyz[2] - v1->xyz[2];
        
        float uv_1_to_2[2];
        uv_1_to_2[0] = v2->uv[0] - v1->uv[0];
        uv_1_to_2[1] = v2->uv[1] - v1->uv[1];
        
        float vec_1_to_3[3];
        vec_1_to_3[0] = v3->xyz[0] - v1->xyz[0];
        vec_1_to_3[1] = v3->xyz[1] - v1->xyz[1];
        vec_1_to_3[2] = v3->xyz[2] - v1->xyz[2];
        
        float uv_1_to_3[2];
        uv_1_to_3[0] = v3->uv[0] - v1->uv[0];
        uv_1_to_3[1] = v3->uv[1] - v1->uv[1];
        
        
        float denom =
            ((uv_1_to_2[0] * uv_1_to_3[1]) - uv_1_to_3[0] * uv_1_to_2[1]);
        // Avoid division by zero
        if (common_fabs(denom) < 0.000001f) { denom = 0.000001f; };
        
        float T[3] = {
            (vec_1_to_2[0] * uv_1_to_3[1] - vec_1_to_3[0] * uv_1_to_2[1]) / denom,
            (vec_1_to_2[1] * uv_1_to_3[1] - vec_1_to_3[1] * uv_1_to_2[1]) / denom,
            (vec_1_to_2[2] * uv_1_to_3[1] - vec_1_to_3[2] * uv_1_to_2[1]) / denom
        };
        
        // Orthogonalize
        float N[3] = {
            v1->normal_xyz[0],
            v1->normal_xyz[1],
            v1->normal_xyz[2]
        };
        float dot_TN = T[0] * N[0] + T[1] * N[1] + T[2] * N[2];
        T[0] -= dot_TN * N[0];
        T[1] -= dot_TN * N[1];
        T[2] -= dot_TN * N[2];
        
        normalize_zvertex_f3(T);
        v1->tangent_xyz[0] = T[0];
        v1->tangent_xyz[1] = T[1];
        v1->tangent_xyz[2] = T[2];
        
        float B[3] = {
            N[1] * T[2] - N[2] * T[1],
            N[2] * T[0] - N[0] * T[2],
            N[0] * T[1] - N[1] * T[0]
        };
        normalize_zvertex_f3(B);
        v1->bitangent_xyz[0] = B[0];
        v1->bitangent_xyz[1] = B[1];
        v1->bitangent_xyz[2] = B[2];
        
        // Zero check
        float T_len = sqrtf(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
        float B_len = sqrtf(B[0] * B[0] + B[1] * B[1] + B[2] * B[2]);
        if (T_len <= 1e-6) {
            v1->tangent_xyz[0] = 0.0f;
            v1->tangent_xyz[1] = 0.0f;
            v1->tangent_xyz[2] = 0.0f;
        }
        if (B_len <= 1e-6) {
            v1->bitangent_xyz[0] = 0.0f;
            v1->bitangent_xyz[1] = 0.0f;
            v1->bitangent_xyz[2] = 0.0f;
        }
        }
    }
}

int32_t T1_objmodel_new_mesh_id_from_resources(
    const char * obj_filename,
    const char * mtl_filename,
    const bool32_t flip_uv_u,
    const bool32_t flip_uv_v)
{
    log_assert(all_mesh_summaries_size < ALL_MESHES_SIZE);
    if (!application_running) {
        log_append("Early exit from objmodel_new_mesh_id_from_resources(), application not running...\n");
        return -1;
    }
    
    FileBuffer obj_file_buf;
    obj_file_buf.size_without_terminator =
        platform_get_resource_size(obj_filename);
    if (obj_file_buf.size_without_terminator < 1) {
        log_append("Early exit from objmodel_new_mesh_id_from_resources(), obj resource: ");
        log_append(obj_filename);
        log_append(" doesn't exist...\n");
        return -1;
    }
    obj_file_buf.contents = (char *)malloc_from_managed(
        obj_file_buf.size_without_terminator + 1);
    obj_file_buf.good = false;
    
    platform_read_resource_file(
        /* char * filename: */
            obj_filename,
        /* FileBuffer *out_preallocatedbuffer: */
            &obj_file_buf);
    
    if (!obj_file_buf.good) {
        free_from_managed(obj_file_buf.contents);
        log_append("Early exit from objmodel_new_mesh_id_from_resources(), resource: ");
        log_append(obj_filename);
        log_append(" exists but failed to read...\n");
        return -1;
    }
    
    FileBuffer mtl_file_buf;
    if (mtl_filename == NULL || mtl_filename[0] == '\0') {
        mtl_file_buf.contents = NULL;
        mtl_file_buf.size_without_terminator = 0;
    } else {
        mtl_file_buf.size_without_terminator =
            platform_get_resource_size(mtl_filename);
        
        if (mtl_file_buf.size_without_terminator < 1) {
            log_append("Early exit from objmodel_new_mesh_id_from_resources(), mtl resource: ");
            log_append(mtl_filename);
            log_append(" doesn't exist...\n");
            return -1;
        }
        
        mtl_file_buf.contents = (char *)malloc_from_managed(
            mtl_file_buf.size_without_terminator + 1);
        mtl_file_buf.good = false;
        
        platform_read_resource_file(
            /* char * filename: */
                mtl_filename,
            /* FileBuffer *out_preallocatedbuffer: */
                &mtl_file_buf);
        
        if (!mtl_file_buf.good) {
            free_from_managed(obj_file_buf.contents);
            free_from_managed(mtl_file_buf.contents);
            
            log_append("Early exit from objmodel_new_mesh_id_from_resources(), resource: ");
            log_append(mtl_filename);
            log_append(" exists but failed to read...\n");
            
            return -1;
        }
    }
    
    int32_t return_value = T1_objmodel_new_mesh_id_from_obj_mtl_text(
            obj_filename,
        /* const char * obj_text: */
            obj_file_buf.contents,
        /* const char * mtl_text: */
            mtl_file_buf.contents);
    
    if (return_value < 0) {
        return -1;
    }
    
    common_strcpy_capped(
        all_mesh_summaries[return_value].resource_name,
        OBJ_STRING_SIZE,
        obj_filename);
    
    free_from_managed(obj_file_buf.contents);
    if (mtl_file_buf.contents != NULL) {
       free_from_managed(mtl_file_buf.contents);
    }
    
    if (flip_uv_v) {
        T1_objmodel_flip_mesh_uvs_v(return_value);
    }
    
    if (flip_uv_u) {
        T1_objmodel_flip_mesh_uvs_u(return_value);
    }
    
    T1_objmodel_deduce_tangents_and_bitangents(return_value);
    
    return return_value;
}

int32_t T1_objmodel_obj_resource_name_to_mesh_id(
    const char * obj_filename)
{
    for (int32_t i = 0; i < (int32_t)all_mesh_summaries_size; i++) {
        if (
            common_are_equal_strings(
                 all_mesh_summaries[i].resource_name,
                 obj_filename))
        {
            return i;
        }
    }
    
    return -1;
}

float T1_objmodel_get_x_multiplier_for_width(
    const int32_t mesh_id,
    const float screenspace_width,
    const float given_z)
{
    return engineglobals_screenspace_width_to_width(
        /* const float screenspace_width: */
            screenspace_width /
                all_mesh_summaries[mesh_id].base_width,
        /* const float given_z: */
            given_z);
}

float T1_objmodel_get_y_multiplier_for_height(
    const int32_t mesh_id,
    const float screenspace_height,
    const float given_z)
{
    return engineglobals_screenspace_height_to_height(
        /* const float screenspace_width: */
            screenspace_height /
                all_mesh_summaries[mesh_id].base_height,
        /* const float given_z: */
            given_z);
}

void T1_objmodel_center_mesh_offsets(
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

void T1_objmodel_flip_mesh_uvs(const int32_t mesh_id)
{
    int32_t tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
            all_mesh_summaries[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        log_assert(all_mesh_vertices->gpu_data[vert_i].uv[0] >= -0.01f);
        log_assert(all_mesh_vertices->gpu_data[vert_i].uv[0] <= 1.01f);
        log_assert(all_mesh_vertices->gpu_data[vert_i].uv[1] >= -0.01f);
        log_assert(all_mesh_vertices->gpu_data[vert_i].uv[1] <= 1.01f);
        all_mesh_vertices->gpu_data[vert_i].uv[0] = 1.0f -
            all_mesh_vertices->gpu_data[vert_i].uv[0];
        all_mesh_vertices->gpu_data[vert_i].uv[1] = 1.0f -
            all_mesh_vertices->gpu_data[vert_i].uv[1];
    }
}

void T1_objmodel_flip_mesh_uvs_u(const int32_t mesh_id)
{
    int32_t tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
            all_mesh_summaries[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        all_mesh_vertices->gpu_data[vert_i].uv[0] = 1.0f -
            all_mesh_vertices->gpu_data[vert_i].uv[0];
    }
}

void T1_objmodel_flip_mesh_uvs_v(const int32_t mesh_id)
{
    int32_t tail_i =
        all_mesh_summaries[mesh_id].vertices_head_i +
            all_mesh_summaries[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = all_mesh_summaries[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        all_mesh_vertices->gpu_data[vert_i].uv[1] = 1.0f -
            all_mesh_vertices->gpu_data[vert_i].uv[1];
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

void T1_objmodel_create_shattered_version_of_mesh(
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
            }
            
            if (second_new_triangle_vertices[m] == USE_MIDLINE) {
                second_tri[m] = mid_of_line;
            } else {
                log_assert(second_new_triangle_vertices[m] >= 0);
                log_assert(second_new_triangle_vertices[m]  < 3);
                second_tri[m] =
                    all_mesh_vertices->gpu_data[
                        biggest_area_head_i + second_new_triangle_vertices[m]];
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
        all_mesh_vertices->gpu_data[biggest_area_head_i + 1] = first_tri[1];
        all_mesh_vertices->gpu_data[biggest_area_head_i + 2] = first_tri[2];
        #ifndef LOGGER_IGNORE_ASSERTS
        float overwritten_area =
            get_squared_triangle_length_from_locked_vertices(
                /* const GPULockedVertex * vertices: */
                    all_mesh_vertices->gpu_data + biggest_area_head_i);
        log_assert(overwritten_area > 0.0f);
        #endif
        
        all_mesh_vertices->gpu_data[temp_new_tail_i + 1] = second_tri[0];
        all_mesh_vertices->gpu_data[temp_new_tail_i + 2] = second_tri[1];
        all_mesh_vertices->gpu_data[temp_new_tail_i + 3] = second_tri[2];
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
