#include "T1_mesh_summary.h"
#include "T1_std.h"

T1MeshSummary * T1_mesh_summary_list = NULL;
u32 T1_mesh_summary_list_size = 0;

T1LockedVertexWithMaterialCollection *
    T1_mesh_summary_all_vertices =
        NULL;
