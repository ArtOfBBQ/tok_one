#include "T1_mesh_summary.h"

T1MeshSummary * T1_mesh_summary_list = NULL;
uint32_t T1_mesh_summary_list_size = 0;

T1LockedVertexWithMaterialCollection *
    T1_mesh_summary_all_vertices =
        NULL;
