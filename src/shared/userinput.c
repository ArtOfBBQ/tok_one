#include "userinput.h"

void register_keydown(uint32_t key_id)
{
    float cam_speed = 1.0f;
    float cam_rotation_speed = 0.15f;
    
    switch (key_id)
    {
        case 0:
            // 'A' key
            camera.x_angle += cam_rotation_speed;
            break;
        case 12:
            // 'Q' key
            camera.x_angle -= cam_rotation_speed;
            break;
        case 6:
            // 'Z' key
            camera.z_angle -= cam_rotation_speed;
            break;
        case 7:
            // 'X' key
            camera.z_angle += cam_rotation_speed;
            break;
        case 123:
            // left arrow key
            camera.y_angle -= cam_rotation_speed;
            break;
        case 124:
            // right arrow key
            camera.y_angle += cam_rotation_speed;
            break;
        case 125:
            // down arrow key
            camera.z -= cosf(camera.y_angle) * cam_speed;
            camera.x -= sinf(camera.y_angle) * cam_speed;
            break;
        case 126:
            // up arrow key
            
            zcamera_move_forward(
                &camera,
                cam_speed);
            
            break;
        default:
            printf("unrecognized code\n");
            break;
    }
}

