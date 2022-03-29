#include "zpolygon.h"

ProjectionConstants projection_constants = {};

zCamera camera = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

void init_projection_constants() {
    
    if (window_height < 50.0f || window_width < 50.0f) {
        printf("ERROR: unexpected window size [%f,%f]\n",
            window_height,
            window_width);
        assert(0);
    }
    
    ProjectionConstants * pjc = &projection_constants;
    
    pjc->near = 0.1f;
    pjc->far = 1000.0f;
    
    float field_of_view = 90.0f;
    pjc->field_of_view_rad =
        ((field_of_view * 0.5) / 180.0f) * 3.14159f;
    pjc->field_of_view_modifier =
        1.0f / tanf(pjc->field_of_view_rad);
    
    pjc->aspect_ratio =
        window_height / window_width; 
}

uint32_t chars_till_next_space_or_slash(
    char * buffer)
{
    uint32_t i = 0;
    
    while (
        buffer[i] != '\n'
        && buffer[i] != ' '
        && buffer[i] != '/')
    {
        i++;
    }

    return i;
}

uint32_t chars_till_next_nonspace(
    char * buffer)
{
    uint32_t i = 0;

    while (buffer[i] == ' ') {
        i++;
    }
    
    return i;
}

zPolygon * load_from_obj(
    char * rawdata,
    uint64_t rawdata_size)
{
    zPolygon * return_value = malloc(sizeof(zPolygon));
    return_value->x = 0.0f;
    return_value->y = 0.0f;
    return_value->z = 1.0f;
    return_value->z_angle = 0.0f;
    return_value->y_angle = 0.0f; // 3.14 to face camera
    return_value->x_angle = 0.0f;
    return_value->triangles_size = 0;
    
   
    // TODO: think about buffer size 
    // pass through buffer once to read all vertices 
    zVertex new_vertices[5000];
    float uv_u[2500];
    float uv_v[2500];
    uint32_t new_uv_i = 0;
    
    uint32_t i = 0;
    uint32_t new_vertex_i = 0;
    while (i < rawdata_size) {
        
        // read the 1st character, which denominates the type
        // of information
        if (rawdata[i] == 'v'
            && rawdata[i+1] == ' ') {
            // discard the 'v'
            i++;
            
            // read vertex data
            zVertex new_vertex;
            
            // skip the space(s) after the 'v'
            assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read vertex x
            new_vertex.x = atof(rawdata + i);
            
            // discard vertex x
            i += chars_till_next_space_or_slash(
                rawdata + i);
            assert(rawdata[i] == ' ');
            
            // discard the spaces after vertex x
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read vertex y
            new_vertex.y = atof(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read vertex z
            new_vertex.z = atof(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            assert(rawdata[i] == '\n');
            i++;
            
            new_vertices[new_vertex_i] = new_vertex;
            assert(
                new_vertices[new_vertex_i].x == new_vertex.x);
            assert(
                new_vertices[new_vertex_i].y
                    == new_vertex.y);
            assert(
                new_vertices[new_vertex_i].z
                    == new_vertex.z);
            new_vertex_i++;
        } else if (
            rawdata[i] == 'v'
            && rawdata[i+1] == 't')
        {
            // discard the 'vt'
            i += 2;
            
            // skip the space(s) after the 'vt'
            assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read the u coordinate
            uv_u[new_uv_i] = atof(rawdata + i);
            
            // discard the u coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            assert(rawdata[i] == ' ');
            
            // skip the space(s) after the u coord
            assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read the v coordinate
            uv_v[new_uv_i] = atof(rawdata + i);
            
            // discard the v coordinate
            i += chars_till_next_space_or_slash(
                rawdata + i);
            assert(rawdata[i] == '\n');
            
            new_uv_i += 1;
            
        } else {
            if (rawdata[i] == 'f') {
                return_value->triangles_size += 1;
            }
            // skip until the next line break character 
            while (rawdata[i] != '\n') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    // pass through rawdata again to read all triangles 
    return_value->triangles =
        malloc(
            sizeof(zTriangle) * return_value->triangles_size);
    
    i = 0;
    uint32_t new_triangle_i = 0;
    int32_t using_texturearray_i = -1;
    int32_t using_texture_i = -1;
    float using_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    while (i < rawdata_size) {
        if (rawdata[i] == 'u') {
            uint32_t j = i + 1;
            while (rawdata[j] != '\n') {
                j++;
            }
            uint32_t line_size = j - i;
            
            char * usemtl_hint =
                malloc(sizeof(line_size));
            
            for (j = 0; j < (line_size); j++) {
                usemtl_hint[j] = rawdata[i + j];
            }
            
            if (are_equal_strings(
                    "usemtl Face",
                    usemtl_hint,
                    line_size))
            {
                using_texturearray_i = 1;
                using_texture_i = 2;
            }
            
            if (are_equal_strings(
                    "usemtl Back",
                    usemtl_hint,
                    line_size))
            {
                using_texturearray_i = 1;
                using_texture_i = 5;
            }
            
            if (are_equal_strings(
                    "usemtl Side",
                    usemtl_hint,
                    line_size))
            {
                using_texturearray_i = -1;
                using_texture_i = -1;
                using_color[0] = 0.5;
                using_color[1] = 0.5;
                using_color[2] = 0.5;
                using_color[3] = 1.0;
            }
            
            printf(
                "got usemtl hint from blender: %s\nbailing out\n",
                usemtl_hint);
            
            free(usemtl_hint);
            
            // skip until the next line break character 
            while (rawdata[i] != '\n') {
                i++;
            }
            // skip the line break character
            i++;
            
        } else if (rawdata[i] == 'f') {

            // discard the 'f'
            i++;
            assert(rawdata[i] == ' ');
            
            // skip the space(s) after the 'f'
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read triangle data
            zTriangle new_triangle;
            new_triangle.draw_normals = 0;
            new_triangle.visible = 1;
            
            // read 1st vertex index
            uint32_t vertex_i_0 = atoi(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            
            uint32_t uv_coord_i_0;
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                uv_coord_i_0 =
                    atoi(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
            }
            
            assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read 2nd vertex index
            uint32_t vertex_i_1 = atoi(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            
            uint32_t uv_coord_i_1;
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                uv_coord_i_1 =
                    atoi(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
            }
            
            assert(rawdata[i] == ' ');
            i += chars_till_next_nonspace(rawdata + i);
            assert(rawdata[i] != ' ');
            
            // read 3rd vertex index
            uint32_t vertex_i_2 = atoi(rawdata + i);
            i += chars_till_next_space_or_slash(
                rawdata + i);
            uint32_t uv_coord_i_2;
            if (rawdata[i] == '/')
            {
                // skip the slash
                i++;
                uv_coord_i_2 =
                    atoi(rawdata + i);
                i += chars_till_next_space_or_slash(
                    rawdata + i);
            }
            assert(rawdata[i] == '\n');
            i++;
            
            assert(vertex_i_0 != vertex_i_1);
            assert(vertex_i_0 != vertex_i_2);
            assert(vertex_i_0 > 0);
            assert(vertex_i_1 > 0);
            assert(vertex_i_2 > 0);
            assert(uv_coord_i_0 > 0);
            assert(uv_coord_i_1 > 0);
            assert(uv_coord_i_2 > 0);
            
            new_triangle.vertices[0] =
                new_vertices[vertex_i_0 - 1];
            new_triangle.vertices[1] =
                new_vertices[vertex_i_1 - 1];
            new_triangle.vertices[2] =
                new_vertices[vertex_i_2 - 1];
            
            new_triangle.vertices[0].uv[0] =
                uv_u[uv_coord_i_0 - 1];
            new_triangle.vertices[0].uv[1] =
                uv_v[uv_coord_i_0 - 1];
            new_triangle.vertices[1].uv[0] =
                uv_u[uv_coord_i_1 - 1];
            new_triangle.vertices[1].uv[1] =
                uv_v[uv_coord_i_1 - 1];
            new_triangle.vertices[2].uv[0] =
                uv_u[uv_coord_i_2 - 1];
            new_triangle.vertices[2].uv[1] =
                uv_v[uv_coord_i_2 - 1];
            
            new_triangle.color[0] = using_color[0];
            new_triangle.color[1] = using_color[1];
            new_triangle.color[2] = using_color[2];
            new_triangle.color[3] = using_color[3];
            
            new_triangle.texturearray_i = using_texturearray_i;
            new_triangle.texture_i = using_texture_i;
            
            return_value->triangles[new_triangle_i] =
                new_triangle;
            new_triangle_i++;
        } else {
            // skip until the next line break character 
            while (rawdata[i] != '\n') {
                i++;
            }
            
            // skip the line break character
            i++;
        }
    }
    
    return return_value;
}

void scale_zpolygon(
    zPolygon * to_scale,
    const float new_height)
{
    assert(to_scale != NULL);
    
    float largest_height = 0.0f;
    for (uint32_t i = 0; i < to_scale->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            float height =
                fabs(to_scale->triangles[i].vertices[j].y);
            if (height > largest_height)
            {
                largest_height = height;
            }
        }
    }
    assert(largest_height > 0.0f);
    
    float scale_factor = new_height / largest_height;
    
    for (uint32_t i = 0; i < to_scale->triangles_size; i++) {
        for (uint32_t j = 0; j < 3; j++)
        {
            to_scale->triangles[i].vertices[j].x *= scale_factor;
            to_scale->triangles[i].vertices[j].y *= scale_factor;
            to_scale->triangles[i].vertices[j].z *= scale_factor;
        }
    }
}

zPolygon * get_box() {
    zPolygon * box = malloc(sizeof(zPolygon));
    box->triangles_size = 6 * 2; // 6 faces, 2 per face
    
    box->triangles = malloc(
        sizeof(zTriangle) * box->triangles_size);
    
    box->x = -30.0f;
    box->y = 0.0f;
    box->z = 20.0f;
    box->x_angle = 0.0f;
    box->y_angle = 0.0f;
    box->z_angle = 0.0f;
    
    // SOUTH face
    box->triangles[0].vertices[0] =
        (zVertex){ -2.5f, -2.5f, -2.5f };
    box->triangles[0].vertices[1] =
        (zVertex){ -2.5f, 2.5f, -2.5f };
    box->triangles[0].vertices[2] =
        (zVertex){ 2.5f, 2.5f, -2.5f };
    box->triangles[0].texturearray_i = 1;
    box->triangles[0].texture_i = 6;
    
    box->triangles[1].vertices[0] =
        (zVertex){ -2.5f, -2.5f, -2.5f };
    box->triangles[1].vertices[1] =
        (zVertex){ 2.5f, 2.5f, -2.5f };
    box->triangles[1].vertices[2] =
        (zVertex){ 2.5f, -2.5f, -2.5f };
    box->triangles[1].texturearray_i = 1;
    box->triangles[1].texture_i = 6;
    
    // EAST face
    box->triangles[2].vertices[0] =
        (zVertex){ 2.5f, -2.5f, -2.5f };
    box->triangles[2].vertices[1] =
        (zVertex){ 2.5f, 2.5f, -2.5f };
    box->triangles[2].vertices[2] =
        (zVertex){ 2.5f, 2.5f, 2.5f };
    box->triangles[2].texturearray_i = 1;
    box->triangles[2].texture_i = 6;
    
    box->triangles[3].vertices[0] =
        (zVertex){ 2.5f, -2.5f, -2.5f };
    box->triangles[3].vertices[1] =
        (zVertex){ 2.5f, 2.5f, 2.5f };
    box->triangles[3].vertices[2] =
        (zVertex){ 2.5f, -2.5f, 2.5f };
    box->triangles[3].texturearray_i = 1;
    box->triangles[3].texture_i = 6;
    
    // NORTH face
    box->triangles[4].vertices[0] =
        (zVertex){ 2.5f, -2.5f, 2.5f };
    box->triangles[4].vertices[1] =
        (zVertex){ 2.5f, 2.5f, 2.5f };
    box->triangles[4].vertices[2] =
        (zVertex){ -2.5f, 2.5f, 2.5f };
    box->triangles[4].texturearray_i = 1;
    box->triangles[4].texture_i = 6;
    float red[4] = { 1.0f, 0.2f, 0.2, 1.0f };
    for (uint32_t i = 0; i < 4; i++) {
        box->triangles[4].color[i] = red[i]; 
        box->triangles[4].color[i] = red[i];
        box->triangles[4].color[i] = red[i];
    }
    
    box->triangles[5].vertices[0] =
        (zVertex){ 2.5f, -2.5f, 2.5f };
    box->triangles[5].vertices[1] =
        (zVertex){ -2.5f, 2.5f, 2.5f };
    box->triangles[5].vertices[2] =
        (zVertex){ -2.5f, -2.5f, 2.5f };
    box->triangles[5].texturearray_i = 1;
    box->triangles[5].texture_i = 6;
    for (uint32_t i = 0; i < 4; i++) {
        box->triangles[5].color[i] = red[i]; 
        box->triangles[5].color[i] = red[i];
        box->triangles[5].color[i] = red[i];
    }
    
    // WEST face
    box->triangles[6].vertices[0] =
        (zVertex){ -2.5f, -2.5f, 2.5f };
    box->triangles[6].vertices[1] =
        (zVertex){ -2.5f, 2.5f, 2.5f };
    box->triangles[6].vertices[2] =
        (zVertex){ -2.5f, 2.5f, -2.5f };
    box->triangles[6].texturearray_i = 1;
    box->triangles[6].texture_i = 6;
    
    box->triangles[7].vertices[0] =
        (zVertex){ -2.5f, -2.5f, 2.5f };
    box->triangles[7].vertices[1] =
        (zVertex){ -2.5f, 2.5f, -2.5f };
    box->triangles[7].vertices[2] =
        (zVertex){ -2.5f, -2.5f, -2.5f };
    box->triangles[7].texturearray_i = 1;
    box->triangles[7].texture_i = 6;
    
    // TOP face
    box->triangles[8].vertices[0] =
        (zVertex){ -2.5f, 2.5f, -2.5f };
    box->triangles[8].vertices[1] =
        (zVertex){ -2.5f, 2.5f, 2.5f };
    box->triangles[8].vertices[2] =
        (zVertex){ 2.5f, 2.5f, 2.5f };
    box->triangles[8].texturearray_i = 1;
    box->triangles[8].texture_i = 6;
    
    box->triangles[9].vertices[0] =
        (zVertex){ -2.5f, 2.5f, -2.5f };
    box->triangles[9].vertices[1] =
        (zVertex){ 2.5f, 2.5f, 2.5f };
    box->triangles[9].vertices[2] =
        (zVertex){ 2.5f, 2.5f, -2.5f };
    box->triangles[9].texturearray_i = 1;
    box->triangles[9].texture_i = 6;
    
    // BOTTOM face
    box->triangles[10].vertices[0] =
        (zVertex){ 2.5f, -2.5f, 2.5f };
    box->triangles[10].vertices[1] =
        (zVertex){ -2.5f, -2.5f, 2.5f };
    box->triangles[10].vertices[2] =
        (zVertex){ -2.5f, -2.5f, -2.5f };
    box->triangles[10].texturearray_i = 1;
    box->triangles[10].texture_i = 6;
    
    box->triangles[11].vertices[0] =
        (zVertex){ 2.5f, -2.5f, 2.5f };
    box->triangles[11].vertices[1] =
        (zVertex){ -2.5f, -2.5f, -2.5f };
    box->triangles[11].vertices[2] =
        (zVertex){ 2.5f, -2.5f, -2.5f };
    box->triangles[11].texturearray_i = 1;
    box->triangles[11].texture_i = 6;
    
    for (uint32_t i = 0; i < 12; i += 2) {
        box->triangles[i].draw_normals = 0;
        box->triangles[i].visible = 1;
        box->triangles[i+1].draw_normals = 0;
        box->triangles[i+1].visible = 1;
        
        box->triangles[i].vertices[0].uv[0] = 0.0f; 
        box->triangles[i].vertices[0].uv[1] = 1.0f;
        box->triangles[i].vertices[1].uv[0] = 0.0f; 
        box->triangles[i].vertices[1].uv[1] = 0.0f;
        box->triangles[i].vertices[2].uv[0] = 1.0f; 
        box->triangles[i].vertices[2].uv[1] = 0.0f;
        box->triangles[i+1].vertices[0].uv[0] = 0.0f; 
        box->triangles[i+1].vertices[0].uv[1] = 1.0f;
        box->triangles[i+1].vertices[1].uv[0] = 1.0f; 
        box->triangles[i+1].vertices[1].uv[1] = 0.0f;
        box->triangles[i+1].vertices[2].uv[0] = 1.0f; 
        box->triangles[i+1].vertices[2].uv[1] = 1.0f;
    }
    
    return box;
}

void ztriangle_apply_lighting(
    Vertex recipient[3],
    zTriangle * input,
    zLightSource * zlight_source)
{
    assert(zlight_source != NULL);
    
    // add lighting to the 3 vertices
    for (uint32_t m = 0; m < 3; m++) {
        recipient[m].lighting = 0.0f;
        
        zVertex light_source_pos;
        light_source_pos.x = zlight_source->x;
        light_source_pos.y = zlight_source->y;
        light_source_pos.z = zlight_source->z;
        
        float distance =
            distance_to_zvertex(
                light_source_pos,
                input->vertices[m]);
        
        float distance_mod = zlight_source->reach / distance;
        if (distance_mod < 0.0f) {
            distance_mod = 0.0f;
        }
        
        // *******************************************
        // add ambient lighting 
        recipient[m].lighting +=
            zlight_source->ambient * distance_mod;
        
        // *******************************************
        // add diffuse lighting
        float diffuse_dot = get_visibility_rating(
            light_source_pos,
            input,
            m);
        
        if (diffuse_dot < 0.0f)
        {
            recipient[m].lighting +=
                (diffuse_dot
                    * -1
                    * distance_mod
                    * zlight_source->diffuse);
        }
        // *******************************************
    }
}

void ztriangle_to_3d(
    Vertex recipient[3],
    zTriangle * input)
{
    ProjectionConstants * pjc = &projection_constants;
    
    for (uint32_t i = 0; i < 3; i++) {
        recipient[i].x =
            (pjc->aspect_ratio
            * pjc->field_of_view_modifier
            * input->vertices[i].x); 
        recipient[i].y = input->vertices[i].y;
        recipient[i].z = 0.0f;
        
        recipient[i].uv[0] = input->vertices[i].uv[0];
        recipient[i].uv[1] = input->vertices[i].uv[1];
        
        for (uint32_t j = 0; j < 3; j++) {
            recipient[i].RGBA[j] = input->color[j];
        }
        
        recipient[i].texturearray_i = input->texturearray_i;
        recipient[i].texture_i = input->texture_i;
    }
}

void ztriangle_to_2d(
    Vertex recipient[3],
    zTriangle * input)
{
    ProjectionConstants * pjc = &projection_constants;
    
    for (uint32_t i = 0; i < 3; i++) {
        
        recipient[i].x =
            input->vertices[i].x *
            pjc->aspect_ratio *
            pjc->field_of_view_modifier; 
        
        // note to self: y transformation
        // doesn't use aspect ratio
        recipient[i].y =
            input->vertices[i].y *
            pjc->field_of_view_modifier;
        
        recipient[i].z =
            input->vertices[i].z *
                pjc->far / (pjc->far - pjc->near) +
            1.0f * (-pjc->far * pjc->near) /
                (pjc->far - pjc->near);
        
        recipient[i].w = input->vertices[i].z == 0.0f ?
           0.0001f
           : input->vertices[i].z;
        
        // recipient[i].x /= recipient[i].w;
        // recipient[i].y /= recipient[i].w;
        // recipient[i].z /= recipient[i].w;
        
        recipient[i].uv[0] = input->vertices[i].uv[0];
        recipient[i].uv[1] = input->vertices[i].uv[1];
        
        for (uint32_t j = 0; j < 3; j++) {
            recipient[i].RGBA[j] = input->color[j];
        }
        
        recipient[i].texturearray_i = input->texturearray_i;
        recipient[i].texture_i = input->texture_i;
    }
}

void free_zpolygon(
    zPolygon * to_free)
{
    free(to_free->triangles);
    free(to_free);
}

zVertex x_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    zVertex return_value = *input;
    
    return_value.y =
        (input->y * cosf(angle))
        - (input->z * sinf(angle));
    
    return_value.z =
        (input->y * sinf(angle)) +
        (input->z * cosf(angle));
    
    return return_value;
}

zTriangle x_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = x_rotate_zvertex(
            &return_value.vertices[i],
            angle);
    }
    
    return return_value;
}

zVertex z_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    zVertex return_value = *input;
    
    return_value.x =
        (input->x * cosf(angle))
        - (input->y * sinf(angle));
    
    return_value.y =
        (input->y * cosf(angle)) +
        (input->x * sinf(angle));
    
    return return_value;
}

zTriangle z_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = z_rotate_zvertex(
            &return_value.vertices[i],
            angle);
    }
    
    return return_value;
}

zVertex y_rotate_zvertex(
    const zVertex * input,
    const float angle)
{
    zVertex return_value = *input;
    
    return_value.x =
        (input->x * cosf(angle))
        + (input->z * sinf(angle));
    
    return_value.z =
        (input->z * cosf(angle)) -
        (input->x * sinf(angle));
    
    return return_value;
}

zTriangle y_rotate_triangle(
    const zTriangle * input,
    const float angle)
{
    zTriangle return_value = *input;
    
    if (angle == 0.0f) {
        return return_value;
    }
    
    for (
        uint32_t i = 0;
        i < 3;
        i++)
    {
        return_value.vertices[i] = y_rotate_zvertex(
            &return_value.vertices[i],
            angle);
    }
    
    return return_value;
}

zTriangle translate_ztriangle(
    const zTriangle * input,
    const float by_x,
    const float by_y,
    const float by_z)
{
    zTriangle return_value = *input;
    
    for (uint32_t i = 0; i < 3; i++) {
        return_value.vertices[i].x += by_x;
        return_value.vertices[i].y += by_y;
        return_value.vertices[i].z += by_z;
    }
    
    return return_value;
}

float get_avg_z(
    const zTriangle * of_triangle)
{
    return (
        of_triangle->vertices[0].z +
        of_triangle->vertices[1].z +
        of_triangle->vertices[2].z) / 3.0f;
}

int sorter_cmpr_lowest_z(
    const void * a,
    const void * b)
{
    return get_avg_z(a) < get_avg_z(b) ? -1 : 1;
}

float get_magnitude(zVertex input) {
    float sum_squares =
        (input.x * input.x) +
        (input.y * input.y) +
        (input.z * input.z);
    return sqrtf(sum_squares);
}

void normalize_zvertex(
    zVertex * to_normalize)
{
    float magnitude = get_magnitude(*to_normalize);
    to_normalize->x /= magnitude;
    to_normalize->y /= magnitude;
    to_normalize->z /= magnitude;
}

float dot_of_vertices(
    const zVertex vertex_1,
    const zVertex vertex_2)
{
    float dot_x = (vertex_1.x * vertex_2.x);
    float dot_y = (vertex_1.y * vertex_2.y);
    float dot_z = (vertex_1.z * vertex_2.z);
    
    return dot_x + dot_y + dot_z;
}

float get_distance(
    const zVertex p1,
    const zVertex p2)
{
    return sqrtf(
        ((p1.x - p2.x) * (p1.x - p2.x))
        + ((p1.y - p2.y) * (p1.y - p2.y))
        + ((p1.z - p2.z) * (p1.z - p2.z)));
}

float distance_to_ztriangle(
    const zVertex p1,
    const zTriangle p2)
{
    return (
        get_distance(p1, p2.vertices[0]) +
        get_distance(p1, p2.vertices[1]) +
        get_distance(p1, p2.vertices[2])) / 3.0f;
}

float distance_to_zvertex(
    const zVertex p1,
    const zVertex p2)
{
    return (
        get_distance(p1, p2) +
        get_distance(p1, p2) +
        get_distance(p1, p2)) / 3.0f;
}

zVertex get_ztriangle_normal(
    const zTriangle * input,
    const uint32_t at_vertex_i)
{
    uint32_t vertex_0 = at_vertex_i % 3;
    assert(vertex_0 == at_vertex_i);
    uint32_t vertex_1 = (at_vertex_i + 1) % 3;
    uint32_t vertex_2 = (at_vertex_i + 2) % 3;
    
    zVertex normal; 
    zVertex vector1;
    zVertex vector2;
    
    vector1.x =
        input->vertices[vertex_1].x
            - input->vertices[vertex_0].x;
    vector1.y =
        input->vertices[vertex_1].y
            - input->vertices[vertex_0].y;
    vector1.z =
        input->vertices[vertex_1].z
            - input->vertices[vertex_0].z;
    // normalize_zvertex(&vector1);
    
    vector2.x =
        input->vertices[vertex_2].x
            - input->vertices[vertex_0].x;
    vector2.y =
        input->vertices[vertex_2].y
            - input->vertices[vertex_0].y;
    vector2.z =
        input->vertices[vertex_2].z
            - input->vertices[vertex_0].z;
    // normalize_zvertex(&vector2);
    
    normal.x =
        (vector1.y * vector2.z) - (vector1.z * vector2.y);
    normal.y =
        (vector1.z * vector2.x) - (vector1.x * vector2.z);
    normal.z =
        (vector1.x * vector2.y) - (vector1.y * vector2.x);
    
    return normal;
}

float get_visibility_rating(
    const zVertex observer,
    const zTriangle * observed,
    const uint32_t observed_vertex_i)
{
    // let's move everything so that observer is at {0,0,0}
    // we'll leave the observer as is and just use {0,0,0} where
    // we would have used it
    zTriangle observed_adj = *observed;
    for (uint32_t i = 0; i < 3; i++) {
        observed_adj.vertices[i].x =
            observed->vertices[i].x - observer.x;
        observed_adj.vertices[i].y =
            observed->vertices[i].y - observer.y;
        observed_adj.vertices[i].z =
            observed->vertices[i].z - observer.z;
    }
    
    zVertex normal = get_ztriangle_normal(
        observed,
        observed_vertex_i);
    normalize_zvertex(&normal);
    
    // compare normal's similarity to a vector straight
    // from observer to triangle location 
    zVertex triangle_minus_observer;
    triangle_minus_observer.x =
        observed_adj.vertices[observed_vertex_i].x;
    triangle_minus_observer.y =
        observed_adj.vertices[observed_vertex_i].y;
    triangle_minus_observer.z =
        observed_adj.vertices[observed_vertex_i].z;
    normalize_zvertex(&triangle_minus_observer);
    
    if (get_magnitude(triangle_minus_observer) > 1.01f) {
        printf(
            "ERROR: normalized triangle_minus_observer still has magnitude of %f\n",
            get_magnitude(triangle_minus_observer));
        printf(
            "triangle_minus_observer coords were: {%f, %f, %f}\n",
            triangle_minus_observer.x,
            triangle_minus_observer.y,
            triangle_minus_observer.z);
        assert(0);
    }
    
    return dot_of_vertices(
        normal,
        triangle_minus_observer);
}

void zcamera_move_forward(
    zCamera * to_move,
    const float distance)
{
    // pick a point that would be in front of the camera
    // if it was not angled in any way, and if it was at
    // the origin
    zVertex forward_if_camera_was_unrotated_at_origin;
    forward_if_camera_was_unrotated_at_origin.x = 0.0f;
    forward_if_camera_was_unrotated_at_origin.y = 0.0f;
    forward_if_camera_was_unrotated_at_origin.z = distance;
    
    zVertex x_rotated = x_rotate_zvertex(
        &forward_if_camera_was_unrotated_at_origin,
        camera.x_angle);
    zVertex y_rotated = y_rotate_zvertex(
        &x_rotated,
        camera.y_angle);
    zVertex final = z_rotate_zvertex(
        &y_rotated,
        camera.z_angle);
    
    // add to the camera's current position
    to_move->x += final.x;
    to_move->y += final.y;
    to_move->z += final.z;
}

