#include "gft/platform/macos.hpp"
#import <Cocoa/Cocoa.h>
#import <MetalKit/MTKView.h>

@interface GraphiTViewController : NSViewController
@end
@implementation GraphiTViewController
{
  MTKView *_view;
  NSRect _frame;
}

- (instancetype)initWithFrame:(NSRect)frame
{
  self = [super init];
  _frame = frame;
  return self;
}

- (void)loadView
{
  self.view = [[MTKView alloc] initWithFrame:_frame];
  self.title = @"GraphiT";
  self.preferredContentSize = _frame.size;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  _view = (MTKView *)self.view;
  _view.device = MTLCreateSystemDefaultDevice();

  if(!_view.device)
  {
    NSLog(@"Metal is not supported on this device");
    self.view = [[NSView alloc] initWithFrame:self.view.frame];
    return;
  }
}
@end
@interface GraphiTWindow : NSWindow
@end
@implementation GraphiTWindow
-(void)close
{
  [super close];
}
-(void)sendEvent:(NSEvent *)event
{
  [super sendEvent:event];
  [NSApp stopModal];
}
@end

namespace liong {
namespace macos {

Window create_window(uint32_t width, uint32_t height) {
  NSRect frame = NSMakeRect(0, 0, width, height);
  
  [NSApplication sharedApplication];
  NSViewController* viewController = [[GraphiTViewController alloc] initWithFrame:frame];
  CAMetalLayer* metal_layer = (CAMetalLayer*)viewController.view.layer;
  NSWindow* window = [GraphiTWindow windowWithContentViewController:viewController];
  [NSApp finishLaunching];
  
  // The following is basically the same as one iteration of `[NSApp run]`;
  // A first iteraion is required, otherwise the window won't show.
  do {
    // This automatically calls `[window makeKeyAndOrderFront:nil]`.
    [NSApp runModalForWindow:window];
  } while (false);

  Window out {};
  out.window = (void*)window;
  out.metal_layer = (void*)metal_layer;
  return out;
}
Window create_window() {
  return create_window(1024, 768);
}

} // namespace macos
} // namespace liong
