#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#include "gpu.h"
#include "software_renderer.h"

@interface ViewController : UIViewController

@property (retain, strong, nonatomic) MetalKitViewDelegate * mtk_view_delegate;
@property (retain, strong, nonatomic) id<MTLDevice> metal_device;
@end
