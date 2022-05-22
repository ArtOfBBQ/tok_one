#import "ViewController.h"

@implementation ViewController
TouchableMTKView * _my_mtk_view;

- (void)viewDidLoad {
    printf("running viewcontroller viewDidLoad...\n");
    [super viewDidLoad];
    
    window_height = [UIScreen mainScreen].bounds.size.height; 
    window_width = [UIScreen mainScreen].bounds.size.width;    
    
    printf("setting up projection constants...¥n");
    init_projection_constants();
    printf("setting up renderer...¥n");
    init_renderer();
    
    //_my_mtk_view = (TouchableMTKView *)self.view;
    _my_mtk_view = [[TouchableMTKView alloc] init];
    _my_mtk_view.preferredFramesPerSecond = 60;
    
    self.view = _my_mtk_view;
    
    _my_mtk_view.paused = NO;
    _my_mtk_view.enableSetNeedsDisplay = NO;
    
    _metal_device = MTLCreateSystemDefaultDevice();
    
    if (_metal_device == nil) {
        printf("error - _metal_device was nil after MTLCreateSystemDefaultDevice()\n");
        return;
    } else {
        printf(
            "_metal_device address: %p\n",
            _metal_device);
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
