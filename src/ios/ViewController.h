#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#import "../shared_apple/gpu.h"
#import "../shared/software_renderer.h"

@interface TouchableMTKView : MTKView
- (void)
    touchesBegan:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event;
- (void)
    touchesMoved:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event;
- (void)
    touchesEnded:(NSSet<UITouch *> *)touches 
    withEvent:(UIEvent *)event;
@end

@interface ViewController : UIViewController
@property (retain, strong, nonatomic) MetalKitViewDelegate * mtk_view_delegate;
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
