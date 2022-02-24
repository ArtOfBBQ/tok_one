#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#import <sys/mman.h>

#import "gpu.h"
#import "software_renderer.h"

@interface ViewController : UIViewController

@property (retain, strong, nonatomic) MetalKitViewDelegate * mtk_view_delegate;
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
