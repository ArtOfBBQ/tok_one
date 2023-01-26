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
            &user_interactions[INTR_PREVIOUS_TOUCH_START],
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START],
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
            &user_interactions[INTR_PREVIOUS_TOUCH_MOVE],
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE],
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
            &user_interactions[INTR_PREVIOUS_TOUCH_MOVE],
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE],
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
            &user_interactions[INTR_PREVIOUS_TOUCH_END],
        /* screenspace_x: */
            platform_x_to_x((float)touchLocation.x
                - platform_get_current_window_left()),
        /* screenspace_y: */
            platform_y_to_y((float)touchLocation.y
                - platform_get_current_window_bottom()));
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END],
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
    _my_mtk_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _my_mtk_view.clearDepth = 1.0f;
    
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
    window_globals->last_resize_request_at = platform_get_current_time_microsecs();
    
    // _my_mtk_view.drawableSize = size;
    
    zpolygons_to_render_size = 0; 
    zlights_to_apply_size = 0;
}

@end
