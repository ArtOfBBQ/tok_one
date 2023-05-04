#include "uielement.h"

NextUIElementSettings next_ui_element_settings;

void request_float_slider(
    const float x_screenspace,
    const float y_screenspace,
    const float z,
    const float min_value,
    const float max_value,
    const float * linked_value)
{
    zPolygon slider_back;
    construct_zpolygon(
	/* zPolygon * to_construct: */
	    &slider_back);
    
    construct_quad_around(
	/* const float mid_x: */
            screenspace_x_to_x(x_screenspace, z),
        /* const float mid_y: */
            screenspace_y_to_y(y_screenspace, z),
	/* const float z: */
	    z,
	/* const float width: */
            screenspace_width_to_width(
                next_ui_element_settings.slider_width_screenspace,
                z),
	/* const float height: */
            screenspace_height_to_height(
                next_ui_element_settings.slider_height_screenspace,
                z),
	/* zPolygon * recipient: */
	    &slider_back);
    slider_back.triangle_materials[0].texturearray_i =
        next_ui_element_settings.slider_background_texturearray_i;
    slider_back.triangle_materials[0].texture_i =
        next_ui_element_settings.slider_background_texture_i;
    
    request_zpolygon_to_render(&slider_back);
}

