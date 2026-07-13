#import "ScrollEventNormalizer.h"

#import <AppKit/AppKit.h>

double MPZPhysicalScrollDeltaY(CGEventRef event) {
    NSEvent *appKitEvent = [NSEvent eventWithCGEvent:event];
    if (!appKitEvent || appKitEvent.type != NSEventTypeScrollWheel) {
        return 0;
    }

    double deltaY = appKitEvent.scrollingDeltaY;
    return appKitEvent.isDirectionInvertedFromDevice ? -deltaY : deltaY;
}
