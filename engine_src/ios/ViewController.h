#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#import "gpu.h"
#import "apple_audio.h"
#import "renderer.h"
#import "init_application.h"

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
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
