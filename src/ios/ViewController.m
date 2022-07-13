#import "ViewController.h"

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
    
    log_assert(window_width == 0.0f);
    log_assert(window_height == 0.0f);
    window_height = platform_get_current_window_height(); 
    window_width = platform_get_current_window_width();
        
    init_projection_constants();
    init_renderer();
    
    _metal_device = MTLCreateSystemDefaultDevice();
    
    if (_metal_device == nil) {
        return;
    }
    
    [_my_mtk_view setDevice: _metal_device];
    
    NSString * shader_lib_filepath =
        [[NSBundle mainBundle]
            pathForResource: @"default"
            ofType: @"metallib"];
    
    _mtk_view_delegate =
        [[MetalKitViewDelegate alloc] init];
    _my_mtk_view.delegate = _mtk_view_delegate;
    [_mtk_view_delegate
        configureMetalWithDevice: _metal_device
        andPixelFormat: _my_mtk_view.colorPixelFormat
        fromFolder: shader_lib_filepath];
}

@end
