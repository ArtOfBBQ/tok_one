#import "ViewController.h"

@implementation TouchableMTKView
- (void)
    touchesBegan:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch       = [touches anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    /*
    platform_x_to_x((float)touchLocation.x -
        window_globals->window_left),
    
    platform_y_to_y((float)touchLocation.y
        - window_globals->window_bottom)
    */
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_START]);
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_START]);
    
    // TODO: remove if this doesn't work out
    // TODO: was trying out move as well to combat some weird UI touch bug
    // TODO: that only happens on ios
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_MOVE]);
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE]);
}

- (void)
    touchesMoved:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch       = [touches anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_MOVE]);
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_MOUSE_OR_TOUCH_MOVE]);
}

- (void)
    touchesEnded:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch       = [touches anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_END]);
    
    register_interaction(
        /* interaction : */
            &user_interactions[INTR_PREVIOUS_TOUCH_OR_LEFTCLICK_END]);
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
    _my_mtk_view.clearDepth = CLEARDEPTH;
    
    _metal_device = MTLCreateSystemDefaultDevice();
    
    if (_metal_device == nil) {
        return;
    }
    
    [_my_mtk_view setDevice: _metal_device];
    
    NSString * shader_lib_filepath =
        [[NSBundle mainBundle]
            pathForResource: @"default"
            ofType: @"metallib"];
    
    apple_gpu_delegate = [[MetalKitViewDelegate alloc] init];
    
    _my_mtk_view.delegate = apple_gpu_delegate;
    
    char errmsg[256];
    bool32_t result = apple_gpu_init(
        /* void (*arg_funcptr_shared_gameloop_update)(GPUDataForSingleFrame *): */
            gameloop_update_before_render_pass,
            gameloop_update_after_render_pass,
        /* id<MTLDevice> with_metal_device: */
            _metal_device,
        /* NSString *shader_lib_filepath: */
            shader_lib_filepath,
        /* char * error_msg_string: */
            errmsg);
    log_assert(result);
        
    init_application_after_gpu_init();
    
    start_audio_loop();
}

- (void)
    viewWillTransitionToSize:
        (CGSize)size 
    withTransitionCoordinator:
        (id<UIViewControllerTransitionCoordinator>)coordinator
{
    engine_globals->last_resize_request_at =
        platform_get_current_time_microsecs();
    
    // _my_mtk_view.drawableSize = size;
    
    zsprites_to_render->size = 0;
    zlights_to_apply_size = 0;
}

@end
