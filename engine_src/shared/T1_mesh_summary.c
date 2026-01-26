#include "T1_mesh_summary.h"

MeshSummary * T1_mesh_summary_list = NULL;
uint32_t T1_mesh_summary_list_size = 0;

LockedVertexWithMaterialCollection *
    T1_mesh_summary_all_vertices =
        NULL;
