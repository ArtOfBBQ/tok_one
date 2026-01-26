#include "T1_objmodel.h"

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

static void guess_gpu_triangle_normal(T1GPULockedVertex * to_change) {
    float vec1_x = to_change[1].xyz[0] - to_change[0].xyz[0];
    float vec1_y = to_change[1].xyz[1] - to_change[0].xyz[1];
    float vec1_z = to_change[1].xyz[2] - to_change[0].xyz[2];
    
    float vec2_x = to_change[2].xyz[0] - to_change[0].xyz[0];
    float vec2_y = to_change[2].xyz[1] - to_change[0].xyz[1];
    float vec2_z = to_change[2].xyz[2] - to_change[0].xyz[2];
    
    to_change[0].norm_xyz[0] = (vec1_y * vec2_z) - (vec1_z * vec2_y);
    to_change[0].norm_xyz[1] = (vec1_z * vec2_x) - (vec1_x * vec2_z);
    to_change[0].norm_xyz[2] = (vec1_x * vec2_y) - (vec1_y * vec2_x);
    normalize_vector_inplace(to_change[0].norm_xyz);
    to_change[1].norm_xyz[0] = to_change[0].norm_xyz[0];
    to_change[1].norm_xyz[1] = to_change[0].norm_xyz[1];
    to_change[1].norm_xyz[2] = to_change[0].norm_xyz[2];
    to_change[2].norm_xyz[0] = to_change[0].norm_xyz[0];
    to_change[2].norm_xyz[1] = to_change[0].norm_xyz[1];
    to_change[2].norm_xyz[2] = to_change[0].norm_xyz[2];
}

static ParsedObj * parsed_obj = NULL;

void T1_objmodel_init(void) {
    parsed_obj = T1_mem_malloc_from_unmanaged(sizeof(ParsedObj));
    T1_std_memset(parsed_obj, 0, sizeof(ParsedObj));
    
    T1_mesh_summary_list = (MeshSummary *)T1_mem_malloc_from_unmanaged(
        sizeof(MeshSummary) * ALL_MESHES_SIZE);
    
    for (uint32_t i = 0; i < ALL_MESHES_SIZE; i++) {
        construct_mesh_summary(&T1_mesh_summary_list[i], (int32_t)i);
    }
    
    assert(ALL_LOCKED_VERTICES_SIZE > 0);
    T1_mesh_summary_all_vertices = (LockedVertexWithMaterialCollection *)
        T1_mem_malloc_from_unmanaged(sizeof(LockedVertexWithMaterialCollection));
    T1_std_memset(
        T1_mesh_summary_all_vertices,
        0,
        sizeof(LockedVertexWithMaterialCollection));
    
    // Let's hardcode a basic quad since that will be used by
    // even crticical engine features (terminal, text labels)
    T1_std_strcpy_cap(
        T1_mesh_summary_list[0].resource_name,
        OBJ_STRING_SIZE,
        "basic_quad");
    T1_mesh_summary_list[0].mesh_id = BASIC_QUAD_MESH_ID;
    T1_mesh_summary_list[0].vertices_head_i = 0;
    T1_mesh_summary_list[0].vertices_size = 6;
    T1_mesh_summary_list[0].base_width = 2.0f;
    T1_mesh_summary_list[0].base_height = 2.0f;
    T1_mesh_summary_list[0].base_depth = 2.0f;
    T1_mesh_summary_list[0].materials_size = 1;
    T1_mesh_summary_list[0].shattered_vertices_head_i = -1;
    T1_mesh_summary_list[0].shattered_vertices_size = 0;
    
    const float left_vertex     = -1.0f;
    const float right_vertex    =  1.0f;
    const float top_vertex      =  1.0f;
    const float bottom_vertex   = -1.0f;
    const float left_uv_coord   =  0.0f;
    const float right_uv_coord  =  1.0f;
    const float bottom_uv_coord =  1.0f;
    const float top_uv_coord    =  0.0f;
    
    // basic quad, triangle 1
    // bottom left vertex
    T1_mesh_summary_all_vertices->gpu_data[0].xyz[0] = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[0].xyz[1] = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[0].xyz[2] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[0].norm_xyz[0] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[0].norm_xyz[1] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[0].norm_xyz[2] = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[0].uv[0] = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[0].uv[1] = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[0].parent_material_i =
        PARENT_MATERIAL_BASE;
    // top right vertex
    T1_mesh_summary_all_vertices->gpu_data[1].xyz[0]                 = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[1].xyz[1]                 = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[1].xyz[2]                 = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[1].norm_xyz[0]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[1].norm_xyz[1]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[1].norm_xyz[2]          = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[1].uv[0]                  = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[1].uv[1]                  = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[1].parent_material_i      = UINT32_MAX;
    // top left vertex
    T1_mesh_summary_all_vertices->gpu_data[2].xyz[0]                 = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[2].xyz[1]                 = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[2].xyz[2]                 = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[2].norm_xyz[0]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[2].norm_xyz[1]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[2].norm_xyz[2]          = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[2].uv[0]                  = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[2].uv[1]                  = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[2].parent_material_i      = PARENT_MATERIAL_BASE;
    
    
    // basic quad, triangle 2 
    // bottom left vertex
    T1_mesh_summary_all_vertices->gpu_data[3].xyz[0] = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[3].xyz[1] = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[3].xyz[2] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[3].uv[0] = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[3].uv[1] = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[3].norm_xyz[0] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[3].norm_xyz[1] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[3].norm_xyz[2] = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[3].parent_material_i =
        PARENT_MATERIAL_BASE;
    // bottom right vertex
    T1_mesh_summary_all_vertices->gpu_data[4].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[4].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[4].xyz[2]            = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[4].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[4].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[4].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[4].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[4].norm_xyz[2]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[4].parent_material_i = PARENT_MATERIAL_BASE;
    // top right vertex
    T1_mesh_summary_all_vertices->gpu_data[5].xyz[0] = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[5].xyz[1] = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[5].xyz[2] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[5].uv[0] = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[5].uv[1] = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[5].norm_xyz[0] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[5].norm_xyz[1] = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[5].norm_xyz[2] = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[5].parent_material_i =
        PARENT_MATERIAL_BASE;
    
    // Let's hardcode a basic cube (currently not used anywhere)
    T1_std_strcpy_cap(
        T1_mesh_summary_list[1].resource_name,
        OBJ_STRING_SIZE,
        "basic_cube");
    T1_mesh_summary_list[1].vertices_head_i = 6;
    T1_mesh_summary_list[1].vertices_size = 36;
    T1_mesh_summary_list[1].mesh_id = BASIC_CUBE_MESH_ID;
    T1_mesh_summary_list[1].materials_size = 1;
    T1_mesh_summary_list[1].base_width = 1.0f;
    T1_mesh_summary_list[1].base_height = 1.0f;
    T1_mesh_summary_list[1].base_depth = 1.0f;
    
    const float front_vertex =  -1.0f;
    const float back_vertex  =   1.0f;
    
    // basic cube, front face triangle 1
    T1_mesh_summary_all_vertices->gpu_data[6].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[6].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[6].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[6].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[6].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[6].norm_xyz[0]     =  0.0f;
    T1_mesh_summary_all_vertices->gpu_data[6].norm_xyz[1]     =  0.0f;
    T1_mesh_summary_all_vertices->gpu_data[6].norm_xyz[2]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[6].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[7].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[7].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[7].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[7].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[7].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[7].norm_xyz[0]     =  0.0f;
    T1_mesh_summary_all_vertices->gpu_data[7].norm_xyz[1]     =  0.0f;
    T1_mesh_summary_all_vertices->gpu_data[7].norm_xyz[2]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[7].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[8].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[8].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[8].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[8].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[8].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[8].norm_xyz[0]     =  0.0f;
    T1_mesh_summary_all_vertices->gpu_data[8].norm_xyz[1]     =  0.0f;
    T1_mesh_summary_all_vertices->gpu_data[8].norm_xyz[2]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[8].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, front face triangle  2
    T1_mesh_summary_all_vertices->gpu_data[9].xyz[0]                  = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[9].xyz[1]                  = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[9].xyz[2]                  = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[9].uv[0]                   = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[9].uv[1]                   = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[9].norm_xyz[0]           = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[9].norm_xyz[1]           = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[9].norm_xyz[2]           = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[9].parent_material_i       = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[10].xyz[0]                 = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[10].xyz[1]                 = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[10].xyz[2]                 = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[10].uv[0]                  = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[10].uv[1]                  = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[10].norm_xyz[0]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[10].norm_xyz[1]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[10].norm_xyz[2]          = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[10].parent_material_i      = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[11].xyz[0]                 = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[11].xyz[1]                 = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[11].xyz[2]                 = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[11].uv[0]                  = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[11].uv[1]                  = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[11].norm_xyz[0]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[11].norm_xyz[1]          = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[11].norm_xyz[2]          = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[11].parent_material_i      = PARENT_MATERIAL_BASE;
    
    // basic cube, back face triangle 1
    T1_mesh_summary_all_vertices->gpu_data[12].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[12].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[12].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[12].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[12].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[12].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[12].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[12].norm_xyz[2]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[12].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[13].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[13].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[13].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[13].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[13].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[13].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[13].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[13].norm_xyz[2]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[13].parent_material_i = PARENT_MATERIAL_BASE;

    T1_mesh_summary_all_vertices->gpu_data[14].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[14].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[14].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[14].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[14].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[14].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[14].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[14].norm_xyz[2]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[14].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, back face triangle 2
    T1_mesh_summary_all_vertices->gpu_data[15].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[15].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[15].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[15].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[15].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[15].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[15].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[15].norm_xyz[2]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[15].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[16].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[16].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[16].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[16].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[16].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[16].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[16].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[16].norm_xyz[2]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[16].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[17].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[17].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[17].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[17].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[17].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[17].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[17].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[17].norm_xyz[2]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[17].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, left face triangle 1
    T1_mesh_summary_all_vertices->gpu_data[18].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[18].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[18].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[18].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[18].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[18].norm_xyz[0]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[18].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[18].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[18].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[19].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[19].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[19].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[19].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[19].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[19].norm_xyz[0]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[19].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[19].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[19].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[20].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[20].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[20].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[20].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[20].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[20].norm_xyz[0]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[20].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[20].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[20].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, left face triangle 2
    T1_mesh_summary_all_vertices->gpu_data[21].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[21].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[21].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[21].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[21].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[21].norm_xyz[0]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[21].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[21].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[21].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[22].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[22].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[22].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[22].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[22].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[22].norm_xyz[0]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[22].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[22].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[22].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[23].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[23].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[23].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[23].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[23].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[23].norm_xyz[0]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[23].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[23].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[23].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, right face triangle 1
    T1_mesh_summary_all_vertices->gpu_data[24].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[24].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[24].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[24].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[24].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[24].norm_xyz[0]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[24].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[24].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[24].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[25].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[25].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[25].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[25].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[25].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[25].norm_xyz[0]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[25].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[25].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[25].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[26].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[26].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[26].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[26].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[26].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[26].norm_xyz[0]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[26].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[26].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[26].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, right face triangle 2
    T1_mesh_summary_all_vertices->gpu_data[27].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[27].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[27].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[27].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[27].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[27].norm_xyz[0]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[27].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[27].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[27].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[28].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[28].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[28].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[28].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[28].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[28].norm_xyz[0]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[28].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[28].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[28].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[29].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[29].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[29].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[29].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[29].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[29].norm_xyz[0]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[29].norm_xyz[1]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[29].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[29].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, top face triangle 1
    T1_mesh_summary_all_vertices->gpu_data[30].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[30].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[30].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[30].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[30].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[30].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[30].norm_xyz[1]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[30].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[30].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[31].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[31].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[31].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[31].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[31].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[31].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[31].norm_xyz[1]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[31].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[31].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[32].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[32].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[32].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[32].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[32].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[32].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[32].norm_xyz[1]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[32].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[32].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, top face triangle 2
    T1_mesh_summary_all_vertices->gpu_data[33].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[33].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[33].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[33].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[33].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[33].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[33].norm_xyz[1]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[33].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[33].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[34].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[34].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[34].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[34].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[34].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[34].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[34].norm_xyz[1]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[34].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[34].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[35].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[35].xyz[1]            = top_vertex;
    T1_mesh_summary_all_vertices->gpu_data[35].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[35].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[35].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[35].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[35].norm_xyz[1]     = 1.0f;
    T1_mesh_summary_all_vertices->gpu_data[35].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[35].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, bottom face triangle 1
    T1_mesh_summary_all_vertices->gpu_data[36].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[36].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[36].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[36].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[36].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[36].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[36].norm_xyz[1]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[36].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[36].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[37].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[37].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[37].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[37].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[37].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[37].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[37].norm_xyz[1]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[37].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[37].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[38].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[38].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[38].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[38].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[38].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[38].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[38].norm_xyz[1]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[38].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[38].parent_material_i = PARENT_MATERIAL_BASE;
    
    // basic cube, bottom face triangle 2
    T1_mesh_summary_all_vertices->gpu_data[39].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[39].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[39].xyz[2]            = back_vertex;
    T1_mesh_summary_all_vertices->gpu_data[39].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[39].uv[1]             = top_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[39].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[39].norm_xyz[1]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[39].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[39].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[40].xyz[0]            = right_vertex;
    T1_mesh_summary_all_vertices->gpu_data[40].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[40].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[40].uv[0]             = right_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[40].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[40].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[40].norm_xyz[1]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[40].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[40].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_all_vertices->gpu_data[41].xyz[0]            = left_vertex;
    T1_mesh_summary_all_vertices->gpu_data[41].xyz[1]            = bottom_vertex;
    T1_mesh_summary_all_vertices->gpu_data[41].xyz[2]            = front_vertex;
    T1_mesh_summary_all_vertices->gpu_data[41].uv[0]             = left_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[41].uv[1]             = bottom_uv_coord;
    T1_mesh_summary_all_vertices->gpu_data[41].norm_xyz[0]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[41].norm_xyz[1]     = -1.0f;
    T1_mesh_summary_all_vertices->gpu_data[41].norm_xyz[2]     = 0.0f;
    T1_mesh_summary_all_vertices->gpu_data[41].parent_material_i = PARENT_MATERIAL_BASE;
    
    T1_mesh_summary_list_size = 2;
    T1_mesh_summary_all_vertices->size = 42;
}

#if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
static void assert_objmodel_validity(int32_t mesh_id) {
    log_assert(mesh_id >= 0);
    log_assert(mesh_id < (int32_t)T1_mesh_summary_list_size);
    log_assert(T1_mesh_summary_list[mesh_id].vertices_head_i >= 0);
    log_assert(
        T1_mesh_summary_list[mesh_id].vertices_size < ALL_LOCKED_VERTICES_SIZE);
    int32_t all_vertices_tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
        T1_mesh_summary_list[mesh_id].vertices_size;
    log_assert(all_vertices_tail_i <= (int32_t)T1_mesh_summary_all_vertices->size);
}
#elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
#else
#error
#endif

static int32_t new_mesh_id_from_parsed_obj_and_parsed_materials(
     const char * original_obj_filename,
     ParsedObj * arg_parsed_obj,
     ParsedMaterial * parsed_materials,
     const uint32_t parsed_materials_size)
{
    float invert_z_axis_modifier = -1.0f;
    
    int32_t new_mesh_head_id =
        (int32_t)T1_mesh_summary_all_vertices->size;
    T1_mesh_summary_list[T1_mesh_summary_list_size].vertices_head_i =
        new_mesh_head_id;
    
    log_assert(T1_mesh_summary_all_vertices->size < ALL_LOCKED_VERTICES_SIZE);
    
    T1_mesh_summary_list[T1_mesh_summary_list_size].materials_size =
        arg_parsed_obj->materials_count;
    
    uint32_t first_material_head_i =
        T1_material_preappend_locked_material_i(
            original_obj_filename,
            parsed_obj->material_names == NULL ?
                "default" :
                parsed_obj->material_names[0].name);
    
    if (parsed_obj->materials_count > 0) {
        // Preregister all materials, starting with the index of the head
        T1_mesh_summary_list[T1_mesh_summary_list_size].
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
                    T1_std_are_equal_strings(
                        arg_parsed_obj->material_names[i].name,
                        parsed_materials[j].name))
                {
                    matching_parsed_materials_i = j;
                }
            }
            
            log_assert(
                matching_parsed_materials_i  < (int32_t)parsed_materials_size);
            
            T1GPUConstMatf32 * locked_mat_f32 = NULL;
            T1GPUConstMati32 * locked_mat_i32 = NULL;
            T1_material_fetch_ptrs(
                &locked_mat_f32,
                &locked_mat_i32,
                T1_mesh_summary_list[T1_mesh_summary_list_size].locked_material_head_i + i);
            
            T1_material_construct(
                locked_mat_f32,
                locked_mat_i32);
            
            if (matching_parsed_materials_i >= 0) {
                locked_mat_f32->ambient_rgb[0] =
                    parsed_materials[matching_parsed_materials_i].ambient_rgb[0];
                locked_mat_f32->ambient_rgb[1] =
                    parsed_materials[matching_parsed_materials_i].ambient_rgb[1];
                locked_mat_f32->ambient_rgb[2] =
                    parsed_materials[matching_parsed_materials_i].ambient_rgb[2];
                
                locked_mat_f32->alpha =
                    parsed_materials[matching_parsed_materials_i].alpha;
                
                locked_mat_f32->diffuse_rgb[0] =
                    parsed_materials[matching_parsed_materials_i].diffuse_rgb[0];
                locked_mat_f32->diffuse_rgb[1] =
                    parsed_materials[matching_parsed_materials_i].diffuse_rgb[1];
                locked_mat_f32->diffuse_rgb[2] =
                    parsed_materials[matching_parsed_materials_i].diffuse_rgb[2];
                
                locked_mat_f32->specular_rgb[0] =
                    parsed_materials[matching_parsed_materials_i].specular_rgb[0];
                locked_mat_f32->specular_rgb[1] =
                    parsed_materials[matching_parsed_materials_i].specular_rgb[1];
                locked_mat_f32->specular_rgb[2] =
                    parsed_materials[matching_parsed_materials_i].specular_rgb[2];
                
                locked_mat_f32->specular_exponent =
                    parsed_materials[matching_parsed_materials_i].
                        specular_exponent;
                
                locked_mat_f32->illum = 1.0f;
                
                locked_mat_f32->uv_scroll[0] = parsed_materials[matching_parsed_materials_i].
                        T1_uv_scroll[0];
                locked_mat_f32->uv_scroll[1] = parsed_materials[matching_parsed_materials_i].
                        T1_uv_scroll[1];
                
                if (parsed_materials[matching_parsed_materials_i].
                    diffuse_map[0] != '\0')
                {
                    T1Tex lmat = T1_texture_array_get_filename_location(
                        /* const char * for_filename: */
                            parsed_materials[matching_parsed_materials_i].
                                diffuse_map);
                    locked_mat_i32->texturearray_i =
                        lmat.array_i;
                    locked_mat_i32->texture_i =
                        lmat.slice_i;
                }
                
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
                        locked_mat_i32->texturearray_i < 0 ||
                        locked_mat_i32->texture_i < 0)
                    {
                        char errmsg[128];
                        T1_std_strcpy_cap(errmsg, 128, "Missing material texture: ");
                        T1_std_strcat_cap(
                            errmsg,
                            128,
                            parsed_materials[matching_parsed_materials_i].
                                diffuse_map);
                        log_dump_and_crash(errmsg);
                        return -1;
                    }
                } else {
                    log_assert(locked_mat_i32->texturearray_i < 0);
                    log_assert(locked_mat_i32->texture_i < 0);
                }
                
                #if T1_NORMAL_MAPPING_ACTIVE == T1_ACTIVE
                T1Tex lmat = T1_texture_array_get_filename_location(
                    /* const char * for_filename: */
                        parsed_materials[matching_parsed_materials_i].
                            bump_or_normal_map);
                
                locked_mat->normalmap_texturearray_i = lmat.array_i;
                locked_mat->normalmap_texture_i = lmat.slice_i;
                #elif T1_NORMAL_MAPPING_ACTIVE == T1_INACTIVE
                #else
                #error
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
                    T1_std_are_equal_strings(
                        arg_parsed_obj->material_names[cur_material_i].name,
                        parsed_materials[i].name))
                {
                    parsed_mtls_matching_i = i;
                }
            }
        
            if (
                parsed_mtls_matching_i >= 0 &&
                parsed_materials
                    [parsed_mtls_matching_i].
                        use_base_mtl_flag)
            {
                cur_material_i = PARENT_MATERIAL_BASE;
                
                T1_mesh_summary_list[T1_mesh_summary_list_size].
                    locked_material_base_offset =
                        (uint32_t)cur_material_i;
            }
        }
        
        uint32_t locked_vert_i = T1_mesh_summary_all_vertices->size;
        
        // We have to read all 3 positions first because we may need them to
        // infer the normals if the .obj file has no normals
        for (uint32_t _ = 0; _ < 3; _++) {
            uint32_t vert_i = arg_parsed_obj->triangles[triangle_i][_];
            
            log_assert(vert_i >= 1);
            log_assert(vert_i <= arg_parsed_obj->vertices_count);
            log_assert(
                locked_vert_i < ALL_LOCKED_VERTICES_SIZE);
            
            T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].xyz[0] =
                arg_parsed_obj->vertices[vert_i - 1][0];
            T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].xyz[1] =
                arg_parsed_obj->vertices[vert_i - 1][1];
            T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].xyz[2] =
                arg_parsed_obj->vertices[vert_i - 1][2] *
                invert_z_axis_modifier;
        }
        
        for (uint32_t _ = 0; _ < 3; _++) {
            T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].
                parent_material_i = cur_material_i;
            T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].
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
                
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].
                    norm_xyz[0] = arg_parsed_obj->normals_vn[norm_i - 1][0];
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].
                    norm_xyz[1] = arg_parsed_obj->normals_vn[norm_i - 1][1];
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].
                    norm_xyz[2] =
                        arg_parsed_obj->normals_vn[norm_i - 1][2] *
                        invert_z_axis_modifier;
            } else if (_ == 0) {
                log_assert(0); // check if deduced normals need to invert z
                guess_gpu_triangle_normal(
                    /* GPULockedVertex * to_change: */
                        &T1_mesh_summary_all_vertices->gpu_data[locked_vert_i]);
                T1_std_memcpy(
                    T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + 1].
                        norm_xyz,
                    T1_mesh_summary_all_vertices->gpu_data[locked_vert_i].
                        norm_xyz,
                    sizeof(float) * 3);
                T1_std_memcpy(
                    T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + 2].
                        norm_xyz,
                    T1_mesh_summary_all_vertices->gpu_data[locked_vert_i].
                        norm_xyz,
                    sizeof(float) * 3);
            }
            
            normalize_zvertex_f3(
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].
                    norm_xyz);
            
            if (arg_parsed_obj->textures_count > 0) {
                uint32_t text_i = arg_parsed_obj->triangle_textures[triangle_i][_];
                
                log_assert(text_i >= 1);
                log_assert(text_i <= arg_parsed_obj->textures_count);
                
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].uv[0] =
                    arg_parsed_obj->textures_vt_uv[text_i - 1][0];
                
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].uv[1] =
                    arg_parsed_obj->textures_vt_uv[text_i - 1][1];
            } else {
                // No uv data in .obj file, gotta guess
                // TODO: Maybe should be part of the obj parser?
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].uv[0] =
                    T1_mesh_summary_all_vertices->size % 2 == 0 ? 0.0f : 1.0f;
                T1_mesh_summary_all_vertices->gpu_data[locked_vert_i + _].uv[1] =
                    T1_mesh_summary_all_vertices->size % 4 == 0 ? 0.0f : 1.0f;
            }
            
        }
        T1_mesh_summary_all_vertices->size += 3;
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
            
            T1GPULockedVertex * V1 = &T1_mesh_summary_all_vertices->gpu_data[T1_mesh_summary_all_vertices->size];
            T1GPULockedVertex * V2 = &T1_mesh_summary_all_vertices->gpu_data[T1_mesh_summary_all_vertices->size+1];
            T1GPULockedVertex * V3 = &T1_mesh_summary_all_vertices->gpu_data[T1_mesh_summary_all_vertices->size+2];
            
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
            
            log_assert(T1_mesh_summary_all_vertices->size < ALL_LOCKED_VERTICES_SIZE);
            
            if (T1_mesh_summary_all_vertices->size >= ALL_LOCKED_VERTICES_SIZE) {
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
                #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
                uint32_t norm_i = arg_parsed_obj->quad_normals[quad_i]
                    [quad_tri_i];
                
                log_assert(norm_i >= 1);
                log_assert(norm_i <= arg_parsed_obj->normals_count);
                #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
                #else
                #error
                #endif
                
                V1->norm_xyz[0] = arg_parsed_obj->normals_vn[norm_1_i][0];
                V1->norm_xyz[1] = arg_parsed_obj->normals_vn[norm_1_i][1];
                V1->norm_xyz[2] = arg_parsed_obj->normals_vn[norm_1_i][2] *
                    invert_z_axis_modifier;
                V2->norm_xyz[0] = arg_parsed_obj->normals_vn[norm_2_i][0];
                V2->norm_xyz[1] = arg_parsed_obj->normals_vn[norm_2_i][1];
                V2->norm_xyz[2] = arg_parsed_obj->normals_vn[norm_2_i][2] *
                    invert_z_axis_modifier;
                V3->norm_xyz[0] = arg_parsed_obj->normals_vn[norm_3_i][0];
                V3->norm_xyz[1] = arg_parsed_obj->normals_vn[norm_3_i][1];
                V3->norm_xyz[2] = arg_parsed_obj->normals_vn[norm_3_i][2] *
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
            
            T1_mesh_summary_all_vertices->size += 3;
        }
    }
    
    T1_objparser_deinit(arg_parsed_obj);
    
    T1_mesh_summary_list[T1_mesh_summary_list_size].mesh_id =
        (int32_t)T1_mesh_summary_list_size;
    T1_mesh_summary_list[T1_mesh_summary_list_size].vertices_size =
        (int32_t)T1_mesh_summary_all_vertices->size -
        T1_mesh_summary_list[T1_mesh_summary_list_size].vertices_head_i;
    log_assert(T1_mesh_summary_list[T1_mesh_summary_list_size].vertices_size > 0);
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    uint32_t new_tail_i = (uint32_t)(
        T1_mesh_summary_list[T1_mesh_summary_list_size].vertices_head_i +
        T1_mesh_summary_list[T1_mesh_summary_list_size].vertices_size -
        1);
    log_assert(new_tail_i < T1_mesh_summary_all_vertices->size);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
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
        tri_i < (int32_t)T1_mesh_summary_all_vertices->size;
        tri_i += 3)
    {
        for (int32_t m = 0; m < 3; m++) {
            if (min_x > T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0]) {
                min_x = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (min_y > T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1]) {
                min_y = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (min_z > T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2]) {
                min_z = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2];
            }
            if (max_x < T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0]) {
                max_x = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (max_y < T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1]) {
                max_y = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (max_z < T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2]) {
                max_z = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2];
            }
        }
    }
    
    log_assert(max_x >= min_x);
    log_assert(max_y >= min_z);
    log_assert(max_z >= min_z);
    T1_mesh_summary_list[T1_mesh_summary_list_size].base_width =
        max_x - min_x;
    T1_mesh_summary_list[T1_mesh_summary_list_size].base_height =
        max_y - min_y;
    T1_mesh_summary_list[T1_mesh_summary_list_size].base_depth =
        max_z - min_z;
    
    T1_mesh_summary_list_size += 1;
    log_assert(T1_mesh_summary_list_size <= ALL_MESHES_SIZE);
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    assert_objmodel_validity((int32_t)T1_mesh_summary_list_size - 1);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    return (int32_t)T1_mesh_summary_list_size - 1;
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
    ParsedMaterial * parsed_materials = T1_mem_malloc_from_managed(
        sizeof(ParsedMaterial) * parsed_materials_cap);
    T1_std_memset(
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
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_dump_and_crash(mtlparser_get_last_error_msg());
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        return -1;
    }
    
    return new_mesh_id_from_parsed_obj_and_parsed_materials(
        original_obj_filename,
        parsed_obj,
        parsed_materials,
        parsed_materials_size);
}

#if T1_OUTLINES_ACTIVE == T1_ACTIVE
static void
    T1_objmodel_deduce_face_normals(
        const int32_t mesh_id)
{
    int32_t tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
            T1_mesh_summary_list[mesh_id].
                vertices_size;
    
    float edge1[3];
    float edge2[3];
    float face_normal_xyz[3];
    
    for (
        int32_t vert_i = T1_mesh_summary_list[mesh_id].
            vertices_head_i;
        vert_i < tail_i;
        vert_i += 3)
    {
        T1GPULockedVertex * v1 = &T1_mesh_summary_all_vertices->
                gpu_data[vert_i+0];
        T1GPULockedVertex * v2 = &T1_mesh_summary_all_vertices->
            gpu_data[vert_i+1];
        T1GPULockedVertex * v3 = &T1_mesh_summary_all_vertices->
            gpu_data[vert_i+2];
        
        /*
        The face normal is calculated as follows:
        edge1 = v2 - v1
        edge2 = v3 - v1
        normal = normalize(cross(edge1, edge2))
        */
        
        edge1[0] = v2->xyz[0] - v1->xyz[0];
        edge1[1] = v2->xyz[1] - v1->xyz[1];
        edge1[2] = v2->xyz[2] - v1->xyz[2];
        
        edge2[0] = v3->xyz[0] - v1->xyz[0];
        edge2[1] = v3->xyz[1] - v1->xyz[1];
        edge2[2] = v3->xyz[2] - v1->xyz[2];
        
        cross_vertices(
            /* float * a: */
                edge1,
            /* float * b: */
                edge2,
            /* float * recip: */
                face_normal_xyz);
        
        normalize_vertex(face_normal_xyz);
        
        T1_std_memcpy(
            v1->face_normal_xyz,
            face_normal_xyz,
            sizeof(float)*3);
        T1_std_memcpy(
            v2->face_normal_xyz,
            face_normal_xyz,
            sizeof(float)*3);
        T1_std_memcpy(
            v3->face_normal_xyz,
            face_normal_xyz,
            sizeof(float)*3);
    }
}
#elif T1_OUTLINES_ACTIVE == T1_INACTIVE
#else
#error
#endif

static void T1_objmodel_deduce_tangents_and_bitangents(
    const int32_t mesh_id)
{
    int32_t tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
            T1_mesh_summary_list[mesh_id].
                vertices_size;
    
    for (
        int32_t vert_i = T1_mesh_summary_list[mesh_id].
            vertices_head_i;
        vert_i < tail_i;
        vert_i += 3)
    {
        for (
            int32_t offset = 0;
            offset < 3;
            offset++)
        {
            T1GPULockedVertex * v1 = &T1_mesh_summary_all_vertices->
                gpu_data[vert_i+((offset+0) % 3)];
            T1GPULockedVertex * v2 = &T1_mesh_summary_all_vertices->
                gpu_data[vert_i+((offset+1) % 3)];
            T1GPULockedVertex * v3 = &T1_mesh_summary_all_vertices->
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
        if (T1_std_fabs(denom) < 0.000001f) { denom = 0.000001f; };
        
        float T[3] = {
            (vec_1_to_2[0] * uv_1_to_3[1] - vec_1_to_3[0] * uv_1_to_2[1]) / denom,
            (vec_1_to_2[1] * uv_1_to_3[1] - vec_1_to_3[1] * uv_1_to_2[1]) / denom,
            (vec_1_to_2[2] * uv_1_to_3[1] - vec_1_to_3[2] * uv_1_to_2[1]) / denom
        };
        
        // Orthogonalize
        float N[3] = {
            v1->norm_xyz[0],
            v1->norm_xyz[1],
            v1->norm_xyz[2]
        };
        float dot_TN = T[0] * N[0] + T[1] * N[1] + T[2] * N[2];
        T[0] -= dot_TN * N[0];
        T[1] -= dot_TN * N[1];
        T[2] -= dot_TN * N[2];
        
        normalize_zvertex_f3(T);
        v1->tan_xyz[0] = T[0];
        v1->tan_xyz[1] = T[1];
        v1->tan_xyz[2] = T[2];
        
        float B[3] = {
            N[1] * T[2] - N[2] * T[1],
            N[2] * T[0] - N[0] * T[2],
            N[0] * T[1] - N[1] * T[0]
        };
        normalize_zvertex_f3(B);
        v1->bitan_xyz[0] = B[0];
        v1->bitan_xyz[1] = B[1];
        v1->bitan_xyz[2] = B[2];
        
        // Zero check
        float T_len = sqrtf(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
        float B_len = sqrtf(B[0] * B[0] + B[1] * B[1] + B[2] * B[2]);
        if (T_len <= 1e-6) {
            v1->tan_xyz[0] = 0.0f;
            v1->tan_xyz[1] = 0.0f;
            v1->tan_xyz[2] = 0.0f;
        }
        if (B_len <= 1e-6) {
            v1->bitan_xyz[0] = 0.0f;
            v1->bitan_xyz[1] = 0.0f;
            v1->bitan_xyz[2] = 0.0f;
        }
        }
    }
}

int32_t T1_objmodel_new_mesh_id_from_resources(
    const char * obj_filename,
    const char * mtl_filename,
    const bool32_t flip_uv_u,
    const bool32_t flip_uv_v,
    bool32_t * success,
    char * error_message)
{
    *success = 0;
    
    log_assert(
        T1_mesh_summary_list_size <
            ALL_MESHES_SIZE);
    
    if (!T1_app_running) {
        T1_std_strcpy_cap(
            error_message,
            128,
            "Early exit from "
            "objmodel_new_mesh_id_from_res()"
            ", application not running...\n");
        return -1;
    }
    
    T1FileBuffer obj_file_buf;
    obj_file_buf.size_without_terminator =
        T1_platform_get_resource_size(
            obj_filename);
    
    if (
        obj_file_buf.size_without_terminator < 1)
    {
        T1_std_strcpy_cap(
            error_message,
            128,
            "Early exit from "
            "objmodel_new_mesh_id_from_res(), "
            "obj resource: ");
        T1_std_strcat_cap(
            error_message,
            128,
            obj_filename);
        T1_std_strcat_cap(
            error_message,
            128,
            " doesn't exist...\n");
        return -1;
    }
    
    obj_file_buf.contents = (char *)
        T1_mem_malloc_from_managed(
            obj_file_buf.
                size_without_terminator + 1);
    obj_file_buf.good = false;
    
    T1_platform_read_resource_file(
        /* char * filename: */
            obj_filename,
        /* FileBuffer *out_preallocatedbuffer: */
            &obj_file_buf);
    
    if (!obj_file_buf.good) {
        T1_mem_free_from_managed(obj_file_buf.contents);
        
        T1_std_strcpy_cap(
            error_message,
            128,
            "Early exit from "
            "objmodel_new_mesh_id_from_res(), "
            "resource: ");
        T1_std_strcat_cap(
            error_message,
            128,
            obj_filename);
        log_append(
            " exists but failed to read...\n");
        return -1;
    }
    
    T1FileBuffer mtl_file_buf;
    if (mtl_filename == NULL || mtl_filename[0] == '\0') {
        mtl_file_buf.contents = NULL;
        mtl_file_buf.size_without_terminator = 0;
    } else {
        mtl_file_buf.size_without_terminator =
            T1_platform_get_resource_size(mtl_filename);
        
        if (mtl_file_buf.
            size_without_terminator < 1)
        {
            T1_std_strcpy_cap(
                error_message,
                128,
                "Early exit from "
                "objmodel_new_mesh_id_from_res(), "
                " mtl resource: ");
            T1_std_strcat_cap(
                error_message,
                128,
                mtl_filename);
            T1_std_strcat_cap(
                error_message,
                128,
                " doesn't exist...\n");
            return -1;
        }
        
        mtl_file_buf.contents = (char *)
            T1_mem_malloc_from_managed(
                mtl_file_buf.size_without_terminator + 1);
        mtl_file_buf.good = false;
        
        T1_platform_read_resource_file(
            /* char * filename: */
                mtl_filename,
            /* FileBuffer *out_preallocatedbuffer: */
                &mtl_file_buf);
        
        if (!mtl_file_buf.good) {
            T1_mem_free_from_managed(obj_file_buf.contents);
            T1_mem_free_from_managed(mtl_file_buf.contents);
            
            T1_std_strcpy_cap(
                error_message,
                128,
                "Early exit from "
                "objmodel_new_mesh_id_from_res(),"
                "resource: ");
            T1_std_strcat_cap(
                error_message,
                128,
                mtl_filename);
            T1_std_strcat_cap(
                error_message,
                128,
                " exists but failed to read...\n");
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
        T1_std_strcpy_cap(
            error_message,
            128,
            "objmodel_new_mesh_id_from_res() "
            "failed (undocumented)");
        return -1;
    }
    
    T1_std_strcpy_cap(
        T1_mesh_summary_list[return_value].resource_name,
        OBJ_STRING_SIZE,
        obj_filename);
    
    T1_mem_free_from_managed(obj_file_buf.contents);
    if (mtl_file_buf.contents != NULL) {
       T1_mem_free_from_managed(mtl_file_buf.contents);
    }
    
    if (flip_uv_v) {
        T1_objmodel_flip_mesh_uvs_v(return_value);
    }
    
    if (flip_uv_u) {
        T1_objmodel_flip_mesh_uvs_u(return_value);
    }
    
    #if T1_OUTLINES_ACTIVE == T1_ACTIVE
    T1_objmodel_deduce_face_normals(
        return_value);
    #elif T1_OUTLINES_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_objmodel_deduce_tangents_and_bitangents(
        return_value);
    
    *success = 1;
    return return_value;
}

int32_t T1_objmodel_resource_name_to_mesh_id(
    const char * obj_filename)
{
    for (int32_t i = 0; i < (int32_t)T1_mesh_summary_list_size; i++) {
        if (
            T1_std_are_equal_strings(
                 T1_mesh_summary_list[i].resource_name,
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
    return T1_render_view_screen_width_to_width(
        /* const float screenspace_width: */
            screenspace_width /
                T1_mesh_summary_list[mesh_id].base_width,
        /* const float given_z: */
            given_z);
}

float T1_objmodel_get_y_multiplier_for_height(
    const int32_t mesh_id,
    const float screenspace_height,
    const float given_z)
{
    return T1_render_view_screen_height_to_height(
        /* const float screenspace_width: */
            screenspace_height /
                T1_mesh_summary_list[mesh_id].base_height,
        /* const float given_z: */
            given_z);
}

void T1_objmodel_center_mesh_offsets(
    const int32_t mesh_id)
{
    log_assert(mesh_id < (int32_t)T1_mesh_summary_list_size);
    
    float smallest_y = T1_F32_MAX;
    float largest_y = T1_F32_MIN;
    float smallest_x = T1_F32_MAX;
    float largest_x = T1_F32_MIN;
    float smallest_z = T1_F32_MAX;
    float largest_z = T1_F32_MIN;
    
    int32_t tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
            T1_mesh_summary_list[mesh_id].vertices_size;
    
    for (
        int32_t tri_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
        tri_i < tail_i;
        tri_i += 3)
    {
        for (int32_t m = 0; m < 3; m++) {
            if (smallest_x > T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0]) {
                smallest_x = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (largest_x < T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0]) {
                largest_x = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[0];
            }
            if (smallest_y > T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1]) {
                smallest_y = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (largest_y < T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1]) {
                largest_y = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[1];
            }
            if (smallest_z > T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2]) {
                smallest_z = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2];
            }
            if (largest_z < T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2]) {
                largest_z = T1_mesh_summary_all_vertices->gpu_data[tri_i + m].xyz[2];
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
    
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    float new_smallest_x = smallest_x - x_delta;
    float new_largest_x = largest_x - x_delta;
    log_assert(new_smallest_x + new_largest_x == 0.0f);
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    
    for (
        int32_t vert_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        T1_mesh_summary_all_vertices->gpu_data[vert_i].xyz[0] -= x_delta;
        T1_mesh_summary_all_vertices->gpu_data[vert_i].xyz[1] -= y_delta;
        T1_mesh_summary_all_vertices->gpu_data[vert_i].xyz[2] -= z_delta;
    }
}

void T1_objmodel_flip_mesh_uvs(const int32_t mesh_id)
{
    int32_t tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
            T1_mesh_summary_list[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        log_assert(T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[0] >= -0.01f);
        log_assert(T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[0] <= 1.01f);
        log_assert(T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[1] >= -0.01f);
        log_assert(T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[1] <= 1.01f);
        T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[0] = 1.0f -
            T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[0];
        T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[1] = 1.0f -
            T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[1];
    }
}

void T1_objmodel_flip_mesh_uvs_u(const int32_t mesh_id)
{
    int32_t tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
            T1_mesh_summary_list[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[0] = 1.0f -
            T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[0];
    }
}

void T1_objmodel_flip_mesh_uvs_v(const int32_t mesh_id)
{
    int32_t tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
            T1_mesh_summary_list[mesh_id].vertices_size;
    
    for (
        int32_t vert_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
        vert_i < tail_i;
        vert_i++)
    {
        T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[1] = 1.0f -
            T1_mesh_summary_all_vertices->gpu_data[vert_i].uv[1];
    }
}

static float get_squared_distance_from_locked_vertices(
    const T1GPULockedVertex a,
    const T1GPULockedVertex b)
{
    return
        ((a.xyz[0] - b.xyz[0]) * (a.xyz[0] - b.xyz[0])) +
        ((a.xyz[1] - b.xyz[1]) * (a.xyz[1] - b.xyz[1])) +
        ((a.xyz[2] - b.xyz[2]) * (a.xyz[2] - b.xyz[2]));
}

/* the largest length amongst any dimension be it x, y, or z */
static float get_squared_triangle_length_from_locked_vertices(
    const T1GPULockedVertex * vertices)
{
    float largest_squared_dist = T1_F32_MIN;
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    int32_t largest_start_vertex_i = -1;
    int32_t largest_end_vertex_i = -1;
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
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
        
        log_assert(new_squared_dist > 0.0001f);
        
        if (new_squared_dist > largest_squared_dist) {
            largest_squared_dist = new_squared_dist;
            #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
            largest_start_vertex_i = start_vertex_i;
            largest_end_vertex_i = end_vertex_i;
            #elif T1_LOGGER_ASSERTS_ACTIE == T1_INACTIVE
            #else
            #error
            #endif
            
            log_assert(largest_start_vertex_i !=
                largest_end_vertex_i);
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
    
    float biggest_area = T1_F32_MIN;
    int32_t biggest_area_i = -1;
    
    for (int32_t i = head_vertex_i; i < (tail_vertex_i - 1); i += 3) {
        float area = get_squared_triangle_length_from_locked_vertices(
            T1_mesh_summary_all_vertices->gpu_data + i);
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
        T1_mesh_summary_list[mesh_id].shattered_vertices_size =
            T1_mesh_summary_list[mesh_id].vertices_size;
        T1_mesh_summary_list[mesh_id].shattered_vertices_head_i =
            T1_mesh_summary_list[mesh_id].vertices_head_i;
        return;
    }
    
    int32_t orig_head_i = T1_mesh_summary_list[mesh_id].vertices_head_i;
    #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
    int32_t orig_tail_i =
        T1_mesh_summary_list[mesh_id].vertices_head_i +
        T1_mesh_summary_list[mesh_id].vertices_size;
    #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
    #else
    #error
    #endif
    int32_t orig_vertices_size = T1_mesh_summary_list[mesh_id].vertices_size;
    
    int32_t new_head_i = (int32_t)T1_mesh_summary_all_vertices->size;
    
    T1_mesh_summary_list[mesh_id].shattered_vertices_head_i = new_head_i;
    T1_mesh_summary_list[mesh_id].shattered_vertices_size =
        T1_mesh_summary_list[mesh_id].vertices_size *
            (int32_t)triangles_multiplier;
    
    int32_t goal_new_tail_i =
        (int32_t)T1_mesh_summary_list[mesh_id].shattered_vertices_head_i +
        (int32_t)T1_mesh_summary_list[mesh_id].shattered_vertices_size -
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
        T1_mesh_summary_all_vertices->gpu_data[new_head_i + i + 0] =
            T1_mesh_summary_all_vertices->gpu_data[orig_head_i + i + 0];
        T1_mesh_summary_all_vertices->gpu_data[new_head_i + i + 1] =
            T1_mesh_summary_all_vertices->gpu_data[orig_head_i + i + 1];
        T1_mesh_summary_all_vertices->gpu_data[new_head_i + i + 2] =
            T1_mesh_summary_all_vertices->gpu_data[orig_head_i + i + 2];
        
        temp_new_tail_i += 3;
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        log_assert(new_head_i + i <= temp_new_tail_i);
        
        float tri_length = get_squared_triangle_length_from_locked_vertices(
            &T1_mesh_summary_all_vertices->gpu_data[new_head_i + i]);
        log_assert(tri_length > 0);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
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
                T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 0],
                T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 1]);
        float distance_1_to_2 =
            get_squared_distance_from_locked_vertices(
                T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 1],
                T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 2]);
        float distance_2_to_0 =
            get_squared_distance_from_locked_vertices(
                T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 2],
                T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 0]);
        
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
        
        T1GPULockedVertex mid_of_line;
        mid_of_line.xyz[0] =
            (T1_mesh_summary_all_vertices->gpu_data[
                    biggest_area_head_i + midline_start_vert_i].xyz[0] +
                T1_mesh_summary_all_vertices->gpu_data[
                    biggest_area_head_i + midline_end_vert_i].xyz[0]) / 2;
        mid_of_line.xyz[1] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].xyz[1] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].xyz[1]) / 2;
        mid_of_line.xyz[2] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].xyz[2] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].xyz[2]) / 2;
        mid_of_line.norm_xyz[0] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].norm_xyz[0] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].norm_xyz[0]) / 2;
        mid_of_line.norm_xyz[1] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].norm_xyz[1] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].norm_xyz[1]) / 2;
        mid_of_line.norm_xyz[2] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].norm_xyz[2] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].norm_xyz[2]) / 2;
        mid_of_line.uv[0] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].uv[0] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].uv[0]) / 2;
        mid_of_line.uv[1] =
            (T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].uv[1] +
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_end_vert_i].uv[1]) / 2;
        mid_of_line.parent_material_i =
            T1_mesh_summary_all_vertices->gpu_data[
                biggest_area_head_i + midline_start_vert_i].parent_material_i;
        
        // split the triangle at biggest_area_i into 2
        T1GPULockedVertex first_tri[3];
        T1GPULockedVertex second_tri[3];
        
        first_tri[0] = T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 0];
        first_tri[1] = T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 1];
        first_tri[2] = T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 2];
        
        second_tri[0] = T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 0];
        second_tri[1] = T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 1];
        second_tri[2] = T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 2];
        
        for (uint32_t m = 0; m < 3; m++) {
            
            if (first_new_triangle_vertices[m] == USE_MIDLINE) {
                first_tri[m] = mid_of_line;
            } else {
                log_assert(first_new_triangle_vertices[m] >= 0);
                log_assert(first_new_triangle_vertices[m] < 3);
                first_tri[m] =
                    T1_mesh_summary_all_vertices->gpu_data[
                        biggest_area_head_i + first_new_triangle_vertices[m]];
            }
            
            if (second_new_triangle_vertices[m] == USE_MIDLINE) {
                second_tri[m] = mid_of_line;
            } else {
                log_assert(second_new_triangle_vertices[m] >= 0);
                log_assert(second_new_triangle_vertices[m]  < 3);
                second_tri[m] =
                    T1_mesh_summary_all_vertices->gpu_data[
                        biggest_area_head_i + second_new_triangle_vertices[m]];
            }
        }
        
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        float orig_area =
            get_squared_triangle_length_from_locked_vertices(
                &T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i]);
        float first_tri_area =
            get_squared_triangle_length_from_locked_vertices(first_tri);
        float second_tri_area =
            get_squared_triangle_length_from_locked_vertices(second_tri);
        log_assert(orig_area > 0.0f);
        log_assert(first_tri_area > 0.0f);
        log_assert(second_tri_area > 0.0f);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 0] = first_tri[0];
        T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 1] = first_tri[1];
        T1_mesh_summary_all_vertices->gpu_data[biggest_area_head_i + 2] = first_tri[2];
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        float overwritten_area =
            get_squared_triangle_length_from_locked_vertices(
                /* const GPULockedVertex * vertices: */
                    T1_mesh_summary_all_vertices->gpu_data + biggest_area_head_i);
        log_assert(overwritten_area > 0.0f);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_mesh_summary_all_vertices->gpu_data[temp_new_tail_i + 1] = second_tri[0];
        T1_mesh_summary_all_vertices->gpu_data[temp_new_tail_i + 2] = second_tri[1];
        T1_mesh_summary_all_vertices->gpu_data[temp_new_tail_i + 3] = second_tri[2];
        #if T1_LOGGER_ASSERTS_ACTIVE == T1_ACTIVE
        float new_area =
            get_squared_triangle_length_from_locked_vertices(
                /* const GPULockedVertex * vertices: */
                    T1_mesh_summary_all_vertices->gpu_data + temp_new_tail_i + 1);
        log_assert(new_area > 0.0f);
        #elif T1_LOGGER_ASSERTS_ACTIVE == T1_INACTIVE
        #else
        #error
        #endif
        
        temp_new_tail_i += 3;
        log_assert(temp_new_tail_i <= goal_new_tail_i);
    }
    
    log_assert(T1_mesh_summary_all_vertices->size < (uint32_t)goal_new_tail_i);
    
    T1_mesh_summary_all_vertices->size = (uint32_t)goal_new_tail_i + 1;
}
