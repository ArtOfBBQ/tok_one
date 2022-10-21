#import "ViewController.h"

@implementation TouchableMTKView
- (void)
    touchesBegan:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch       = [touches anyObject];
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
    
    // TODO: remove if this doesn't work out
    // TODO: was trying out move as well to combat some weird UI touch bug that only happens on ios
    register_interaction(
        /* interaction : */
            &previous_touch_move,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &previous_mouse_or_touch_move,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
}

- (void)
    touchesMoved:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch       = [touches anyObject];
    CGPoint touchLocation = [touch locationInView:self];
        
    register_interaction(
        /* interaction : */
            &previous_touch_move,
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &previous_mouse_or_touch_move,
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
    UITouch * touch       = [touches anyObject];
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

@implementation ViewController
TouchableMTKView * _my_mtk_view;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    //_my_mtk_view = (TouchableMTKView *)self.view;
    _my_mtk_view = [[TouchableMTKView alloc] init];
    _my_mtk_view.preferredFramesPerSecond = 60;
    
    self.view = _my_mtk_view;
    
    _my_mtk_view.paused = NO;
    _my_mtk_view.enableSetNeedsDisplay = NO;
    
    _metal_device = MTLCreateSystemDefaultDevice();
    
    if (_metal_device == nil) {
        return;
    }
    
    [_my_mtk_view setDevice: _metal_device];
    
    NSString * shader_lib_filepath =
        [[NSBundle mainBundle]
            pathForResource: @"default"
            ofType: @"metallib"];
    
    apple_gpu_delegate =
        [[MetalKitViewDelegate alloc] init];
    _my_mtk_view.delegate = apple_gpu_delegate;
    [apple_gpu_delegate
        configureMetalWithDevice: _metal_device
        andPixelFormat: _my_mtk_view.colorPixelFormat
        fromFolder: shader_lib_filepath];
}

- (void)
    viewWillTransitionToSize:
        (CGSize)size 
    withTransitionCoordinator:
        (id<UIViewControllerTransitionCoordinator>)coordinator
{
    last_resize_request_at = platform_get_current_time_microsecs();
    
    // _my_mtk_view.drawableSize = size;
    
    texquads_to_render_size = 0;
    zpolygons_to_render_size = 0; 
    zlights_to_apply_size = 0;
}

@end
