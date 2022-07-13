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
        
    register_touchstart(
        /* x: */ platform_x_to_x((float)touchLocation.x),
        /* y: */ platform_y_to_y((float)touchLocation.y));
}

- (void)
    touchesEnded:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    register_touchend(
        /* x: */ platform_x_to_x((float)touchLocation.x),
        /* y: */ platform_y_to_y((float)touchLocation.y));
}
@end

@interface ViewController : UIViewController

@property (retain, strong, nonatomic) MetalKitViewDelegate * mtk_view_delegate;
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
