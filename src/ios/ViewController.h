#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#import "../shared_apple/gpu.h"
#import "../shared/software_renderer.h"

@interface TouchableMTKView : MTKView
@end

@implementation TouchableMTKView
- (void)
    touchesBegan:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    register_interaction(
        /* interaction : */
            &previous_touch_start,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &previous_touch_or_leftclick_start,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
}

- (void)
    touchesEnded:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    register_interaction(
        /* interaction : */
            &previous_touch_end,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &previous_touch_or_leftclick_end,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
}
@end

@interface ViewController : UIViewController

@property (retain, strong, nonatomic) MetalKitViewDelegate * mtk_view_delegate;
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
