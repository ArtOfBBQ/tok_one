#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#import "gpu.h"
#import "software_renderer.h"

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
        /* x: */ touchLocation.x,
        /* y: */ touchLocation.y);
}

- (void)
    touchesEnded:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event
{
    UITouch * touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    
    register_touchend(
        /* x: */ touchLocation.x,
        /* y: */ touchLocation.y);
}
@end

@interface ViewController : UIViewController

@property (retain, strong, nonatomic) MetalKitViewDelegate * mtk_view_delegate;
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
