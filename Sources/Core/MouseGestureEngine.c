#include "MouseGestureEngine.h"

#include "MultitouchSupport.h"
#include "ControlScrollRecognizer.h"
#include "PinchRecognizer.h"
#include "ScrollEventNormalizer.h"
#include "TapRecognizer.h"

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <os/lock.h>
#include <stdlib.h>

enum {
    MPZCGEventGesture = 29,
    MPZCGGestureEventHIDType = 110,
    MPZCGGestureEventZoomValue = 113,
    MPZCGGestureEventPhase = 132,
    MPZIOHIDEventTypeZoom = 8,
    MPZSyntheticEventTag = 0x4D505A,
};

static const uint64_t kDefaultClickGuardNanoseconds = 80 * NSEC_PER_MSEC;
static const uint64_t kDefaultTapClickDelayNanoseconds = 150 * NSEC_PER_MSEC;
static const CFTimeInterval kRecentMagicMouseTouchInterval = 0.35;

typedef struct {
    os_unfair_lock lock;
    bool alive;
    size_t references;
    uint64_t nextSerial;
    uint64_t cancelledThroughSerial;
} MPZPendingTapState;

typedef struct {
    MPZPendingTapState *state;
    uint64_t serial;
    MPZTapAction action;
} MPZPendingTapContext;

typedef struct MPZDeviceEntry {
    struct MPZEngine *engine;
    MTDeviceRef device;
    uint64_t identifier;
    bool attached;
    MPZPinchRecognizer recognizer;
    MPZTapRecognizer tapRecognizer;
} MPZDeviceEntry;

extern void CoreDockSendNotification(CFStringRef notification, int unknown);

struct MPZEngine {
    os_unfair_lock lock;
    bool enabled;
    bool pinchEnabled;
    bool pinchDirectionLocked;
    bool tapClickEnabled;
    bool middleTapEnabled;
    bool controlScrollEnabled;
    double sensitivity;
    uint64_t tapClickDelayNanoseconds;
    CFTimeInterval lastMagicMouseTouchTimestamp;
    MPZControlScrollRecognizer controlScrollRecognizer;
    MPZDeviceEntry **devices;
    size_t deviceCount;
    MPZDeviceEntry *activeDevice;
    CFMachPortRef eventTap;
    CFRunLoopSourceRef eventTapSource;
    MPZPendingTapState *pendingTapState;
};

static int touchCallback(
    MTDeviceRef device,
    const MPZMTTouch *touches,
    CFIndex touchCount,
    CFTimeInterval timestamp,
    uint32_t frame,
    void *context
);

static CGEventRef eventTapCallback(
    CGEventTapProxy proxy,
    CGEventType type,
    CGEventRef event,
    void *context
);

static void cancelPendingTaps(MPZPendingTapState *state);

static void showWindowOverview(MPZControlScrollAction action) {
    CFStringRef notification = NULL;
    if (action == MPZControlScrollShowApplicationWindows) {
        notification = CFSTR("com.apple.expose.front.awake");
    } else if (action == MPZControlScrollShowAllWindows) {
        notification = CFSTR("com.apple.expose.awake");
    } else {
        return;
    }
    CoreDockSendNotification(notification, 0);
}

static void postGesture(MPZPinchOutput output) {
    if (output.phase == MPZGestureNone) {
        return;
    }

    CGEventRef event = CGEventCreate(NULL);
    if (!event) {
        return;
    }

    CGGesturePhase phase = kCGGesturePhaseChanged;
    switch (output.phase) {
        case MPZGestureBegan:
            phase = kCGGesturePhaseBegan;
            break;
        case MPZGestureChanged:
            phase = kCGGesturePhaseChanged;
            break;
        case MPZGestureEnded:
            phase = kCGGesturePhaseEnded;
            break;
        case MPZGestureNone:
            break;
    }

    CGEventSetType(event, (CGEventType)MPZCGEventGesture);
    CGEventSetIntegerValueField(
        event,
        (CGEventField)MPZCGGestureEventHIDType,
        MPZIOHIDEventTypeZoom
    );
    CGEventSetIntegerValueField(
        event,
        (CGEventField)MPZCGGestureEventPhase,
        phase
    );
    CGEventSetDoubleValueField(
        event,
        (CGEventField)MPZCGGestureEventZoomValue,
        output.magnification
    );
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

static void postMouseClick(MPZTapAction action) {
    if (action == MPZTapNone) {
        return;
    }

    CGEventType downType = kCGEventLeftMouseDown;
    CGEventType upType = kCGEventLeftMouseUp;
    CGMouseButton button = kCGMouseButtonLeft;
    if (action == MPZTapRightClick) {
        downType = kCGEventRightMouseDown;
        upType = kCGEventRightMouseUp;
        button = kCGMouseButtonRight;
    } else if (action == MPZTapMiddleClick) {
        downType = kCGEventOtherMouseDown;
        upType = kCGEventOtherMouseUp;
        button = kCGMouseButtonCenter;
    }

    CGEventRef reference = CGEventCreate(NULL);
    if (!reference) {
        return;
    }
    CGPoint location = CGEventGetLocation(reference);
    CFRelease(reference);

    CGEventRef down = CGEventCreateMouseEvent(NULL, downType, location, button);
    CGEventRef up = CGEventCreateMouseEvent(NULL, upType, location, button);
    if (down && up) {
        CGEventSetIntegerValueField(down, kCGEventSourceUserData, MPZSyntheticEventTag);
        CGEventSetIntegerValueField(up, kCGEventSourceUserData, MPZSyntheticEventTag);
        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);
    }
    if (down) {
        CFRelease(down);
    }
    if (up) {
        CFRelease(up);
    }
}

static MPZPendingTapState *pendingTapStateCreate(void) {
    MPZPendingTapState *state = calloc(1, sizeof(*state));
    if (!state) {
        return NULL;
    }
    state->lock = OS_UNFAIR_LOCK_INIT;
    state->alive = true;
    state->references = 1;
    return state;
}

static void pendingTapStateRelease(MPZPendingTapState *state) {
    os_unfair_lock_lock(&state->lock);
    state->references--;
    bool shouldFree = state->references == 0;
    os_unfair_lock_unlock(&state->lock);
    if (shouldFree) {
        free(state);
    }
}

static void firePendingTap(void *rawContext) {
    MPZPendingTapContext *context = rawContext;
    MPZPendingTapState *state = context->state;

    os_unfair_lock_lock(&state->lock);
    bool shouldPost = state->alive
        && context->serial > state->cancelledThroughSerial;
    os_unfair_lock_unlock(&state->lock);

    if (shouldPost) {
        postMouseClick(context->action);
    }
    pendingTapStateRelease(state);
    free(context);
}

static void scheduleMouseClick(
    MPZPendingTapState *state,
    MPZTapAction action,
    uint64_t delayNanoseconds
) {
    if (!state || action == MPZTapNone) {
        return;
    }

    MPZPendingTapContext *context = calloc(1, sizeof(*context));
    if (!context) {
        return;
    }

    os_unfair_lock_lock(&state->lock);
    if (!state->alive) {
        os_unfair_lock_unlock(&state->lock);
        free(context);
        return;
    }
    context->state = state;
    context->serial = ++state->nextSerial;
    context->action = action;
    state->references++;
    os_unfair_lock_unlock(&state->lock);

    dispatch_after_f(
        dispatch_time(DISPATCH_TIME_NOW, (int64_t)delayNanoseconds),
        dispatch_get_main_queue(),
        context,
        firePendingTap
    );
}

static void cancelPendingTaps(MPZPendingTapState *state) {
    if (!state) {
        return;
    }
    os_unfair_lock_lock(&state->lock);
    state->cancelledThroughSerial = state->nextSerial;
    os_unfair_lock_unlock(&state->lock);
}

static void pendingTapStateInvalidate(MPZPendingTapState *state) {
    if (!state) {
        return;
    }
    os_unfair_lock_lock(&state->lock);
    state->alive = false;
    state->cancelledThroughSerial = state->nextSerial;
    os_unfair_lock_unlock(&state->lock);
    pendingTapStateRelease(state);
}

static uint64_t deviceIdentifier(MTDeviceRef device) {
    io_service_t service = MTDeviceGetService(device);
    uint64_t identifier = 0;
    if (service != IO_OBJECT_NULL
        && IORegistryEntryGetRegistryEntryID(service, &identifier) == KERN_SUCCESS) {
        return identifier;
    }
    return (uint64_t)service;
}

static CFArrayRef createMagicMouseDeviceList(void) {
    CFArrayRef allDevices = MTDeviceCreateList();
    if (!allDevices) {
        return NULL;
    }

    CFMutableArrayRef magicMice = CFArrayCreateMutable(
        kCFAllocatorDefault,
        0,
        &kCFTypeArrayCallBacks
    );
    if (!magicMice) {
        CFRelease(allDevices);
        return NULL;
    }

    CFIndex count = CFArrayGetCount(allDevices);
    for (CFIndex index = 0; index < count; index++) {
        MTDeviceRef device = (MTDeviceRef)CFArrayGetValueAtIndex(allDevices, index);
        uint32_t familyID = 0;
        if (MTDeviceGetFamilyID(device, &familyID) == noErr
            && familyID == MPZMTDeviceFamilyMagicMouse) {
            CFArrayAppendValue(magicMice, device);
        }
    }

    CFRelease(allDevices);
    return magicMice;
}

static bool deviceListMatches(MPZEngine *engine, CFArrayRef deviceList) {
    size_t listCount = (size_t)CFArrayGetCount(deviceList);
    os_unfair_lock_lock(&engine->lock);
    bool matches = engine->deviceCount == listCount;
    for (size_t index = 0; matches && index < engine->deviceCount; index++) {
        uint64_t identifier = engine->devices[index]->identifier;
        bool found = false;
        for (CFIndex listIndex = 0; listIndex < (CFIndex)listCount; listIndex++) {
            MTDeviceRef device = (MTDeviceRef)CFArrayGetValueAtIndex(
                deviceList,
                listIndex
            );
            if (deviceIdentifier(device) == identifier) {
                found = true;
                break;
            }
        }
        matches = found;
    }
    os_unfair_lock_unlock(&engine->lock);
    return matches;
}

static void stopDeviceEntries(MPZDeviceEntry **devices, size_t deviceCount) {
    for (size_t index = 0; index < deviceCount; index++) {
        MPZDeviceEntry *entry = devices[index];
        MTUnregisterContactFrameCallback(entry->device, touchCallback);
        MTDeviceStop(entry->device);
        CFRelease(entry->device);
        free(entry);
    }
    free(devices);
}

static bool stopDevices(MPZEngine *engine) {
    os_unfair_lock_lock(&engine->lock);
    MPZDeviceEntry **devices = engine->devices;
    size_t deviceCount = engine->deviceCount;
    bool shouldEndGesture = engine->activeDevice != NULL;
    for (size_t index = 0; index < deviceCount; index++) {
        devices[index]->attached = false;
    }
    engine->devices = NULL;
    engine->deviceCount = 0;
    engine->activeDevice = NULL;
    os_unfair_lock_unlock(&engine->lock);

    stopDeviceEntries(devices, deviceCount);
    return shouldEndGesture;
}

static void startDevicesFromList(MPZEngine *engine, CFArrayRef deviceList) {
    CFIndex count = CFArrayGetCount(deviceList);
    MPZDeviceEntry **devices = NULL;
    if (count > 0) {
        devices = calloc((size_t)count, sizeof(*devices));
        if (!devices) {
            return;
        }
    }

    size_t deviceCount = 0;
    for (CFIndex index = 0; index < count; index++) {
        MTDeviceRef device = (MTDeviceRef)CFArrayGetValueAtIndex(deviceList, index);
        MPZDeviceEntry *entry = calloc(1, sizeof(*entry));
        if (!entry) {
            continue;
        }

        entry->engine = engine;
        entry->device = (MTDeviceRef)CFRetain(device);
        entry->identifier = deviceIdentifier(device);
        MPZPinchRecognizerReset(&entry->recognizer);
        MPZTapRecognizerReset(&entry->tapRecognizer);

        MTRegisterContactFrameCallbackWithRefcon(device, touchCallback, entry);
        if (MTDeviceStart(device, 0) != noErr) {
            MTUnregisterContactFrameCallback(device, touchCallback);
            CFRelease(entry->device);
            free(entry);
            continue;
        }
        devices[deviceCount++] = entry;
    }

    os_unfair_lock_lock(&engine->lock);
    bool shouldAttach = engine->enabled && engine->devices == NULL;
    if (shouldAttach) {
        engine->devices = devices;
        engine->deviceCount = deviceCount;
        for (size_t index = 0; index < deviceCount; index++) {
            devices[index]->attached = true;
        }
    }
    os_unfair_lock_unlock(&engine->lock);

    if (!shouldAttach) {
        stopDeviceEntries(devices, deviceCount);
    }
}

static void startDevices(MPZEngine *engine) {
    CFArrayRef deviceList = createMagicMouseDeviceList();
    if (!deviceList) {
        return;
    }
    startDevicesFromList(engine, deviceList);
    CFRelease(deviceList);
}

static bool startEventTap(MPZEngine *engine) {
    CGEventMask mask = CGEventMaskBit(kCGEventScrollWheel)
        | CGEventMaskBit(kCGEventLeftMouseDown)
        | CGEventMaskBit(kCGEventRightMouseDown)
        | CGEventMaskBit(kCGEventOtherMouseDown);
    engine->eventTap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        eventTapCallback,
        engine
    );
    if (!engine->eventTap) {
        return false;
    }

    engine->eventTapSource = CFMachPortCreateRunLoopSource(
        kCFAllocatorDefault,
        engine->eventTap,
        0
    );
    if (!engine->eventTapSource) {
        CFRelease(engine->eventTap);
        engine->eventTap = NULL;
        return false;
    }

    CFRunLoopAddSource(
        CFRunLoopGetMain(),
        engine->eventTapSource,
        kCFRunLoopCommonModes
    );
    CGEventTapEnable(engine->eventTap, true);
    return true;
}

static void stopEventTap(MPZEngine *engine) {
    if (engine->eventTapSource) {
        CFRunLoopRemoveSource(
            CFRunLoopGetMain(),
            engine->eventTapSource,
            kCFRunLoopCommonModes
        );
        CFRelease(engine->eventTapSource);
        engine->eventTapSource = NULL;
    }
    if (engine->eventTap) {
        CFMachPortInvalidate(engine->eventTap);
        CFRelease(engine->eventTap);
        engine->eventTap = NULL;
    }
}

MPZEngine *MPZEngineCreate(void) {
    MPZEngine *engine = calloc(1, sizeof(*engine));
    if (!engine) {
        return NULL;
    }
    engine->lock = OS_UNFAIR_LOCK_INIT;
    engine->sensitivity = 1.0;
    engine->tapClickDelayNanoseconds = kDefaultTapClickDelayNanoseconds;
    engine->pendingTapState = pendingTapStateCreate();
    if (!engine->pendingTapState) {
        free(engine);
        return NULL;
    }
    return engine;
}

void MPZEngineDestroy(MPZEngine *engine) {
    if (!engine) {
        return;
    }
    MPZEngineSetEnabled(engine, false);
    pendingTapStateInvalidate(engine->pendingTapState);
    free(engine);
}

bool MPZEngineSetEnabled(MPZEngine *engine, bool enabled) {
    if (!engine) {
        return false;
    }

    os_unfair_lock_lock(&engine->lock);
    bool currentlyEnabled = engine->enabled;
    os_unfair_lock_unlock(&engine->lock);
    if (enabled == currentlyEnabled) {
        return true;
    }

    if (!enabled) {
        os_unfair_lock_lock(&engine->lock);
        engine->enabled = false;
        os_unfair_lock_unlock(&engine->lock);
        cancelPendingTaps(engine->pendingTapState);
        bool shouldEndGesture = stopDevices(engine);
        if (shouldEndGesture) {
            postGesture((MPZPinchOutput){
                .phase = MPZGestureEnded,
                .magnification = 0,
                .suppressScroll = false,
            });
        }
        stopEventTap(engine);
        return true;
    }

    if (!startEventTap(engine)) {
        return false;
    }

    os_unfair_lock_lock(&engine->lock);
    engine->enabled = true;
    os_unfair_lock_unlock(&engine->lock);
    startDevices(engine);
    return true;
}

bool MPZEngineIsEnabled(MPZEngine *engine) {
    if (!engine) {
        return false;
    }
    os_unfair_lock_lock(&engine->lock);
    bool enabled = engine->enabled;
    os_unfair_lock_unlock(&engine->lock);
    return enabled;
}

void MPZEngineSetSensitivity(MPZEngine *engine, double sensitivity) {
    if (!engine) {
        return;
    }
    os_unfair_lock_lock(&engine->lock);
    engine->sensitivity = sensitivity;
    os_unfair_lock_unlock(&engine->lock);
}

void MPZEngineSetTapClickDelayMilliseconds(MPZEngine *engine, double milliseconds) {
    if (!engine) {
        return;
    }
    double clamped = MPZTapClickDelayClampMilliseconds(milliseconds);
    os_unfair_lock_lock(&engine->lock);
    engine->tapClickDelayNanoseconds = (uint64_t)(clamped * NSEC_PER_MSEC);
    os_unfair_lock_unlock(&engine->lock);
    cancelPendingTaps(engine->pendingTapState);
}

void MPZEngineSetPinchEnabled(MPZEngine *engine, bool enabled) {
    if (!engine) {
        return;
    }

    os_unfair_lock_lock(&engine->lock);
    bool shouldEndGesture = engine->activeDevice != NULL && !enabled;
    engine->pinchEnabled = enabled;
    if (!enabled) {
        engine->activeDevice = NULL;
        for (size_t index = 0; index < engine->deviceCount; index++) {
            MPZPinchRecognizerReset(&engine->devices[index]->recognizer);
        }
    }
    os_unfair_lock_unlock(&engine->lock);

    if (shouldEndGesture) {
        postGesture((MPZPinchOutput){
            .phase = MPZGestureEnded,
            .magnification = 0,
            .suppressScroll = false,
        });
    }
}

void MPZEngineSetPinchDirectionLocked(MPZEngine *engine, bool locked) {
    if (!engine) {
        return;
    }
    os_unfair_lock_lock(&engine->lock);
    engine->pinchDirectionLocked = locked;
    os_unfair_lock_unlock(&engine->lock);
}

void MPZEngineSetTapClickEnabled(MPZEngine *engine, bool enabled) {
    if (!engine) {
        return;
    }
    os_unfair_lock_lock(&engine->lock);
    engine->tapClickEnabled = enabled;
    for (size_t index = 0; index < engine->deviceCount; index++) {
        MPZTapRecognizerReset(&engine->devices[index]->tapRecognizer);
    }
    os_unfair_lock_unlock(&engine->lock);
    cancelPendingTaps(engine->pendingTapState);
}

void MPZEngineSetMiddleTapEnabled(MPZEngine *engine, bool enabled) {
    if (!engine) {
        return;
    }
    os_unfair_lock_lock(&engine->lock);
    engine->middleTapEnabled = enabled;
    for (size_t index = 0; index < engine->deviceCount; index++) {
        MPZTapRecognizerReset(&engine->devices[index]->tapRecognizer);
    }
    os_unfair_lock_unlock(&engine->lock);
    cancelPendingTaps(engine->pendingTapState);
}

void MPZEngineSetControlScrollEnabled(MPZEngine *engine, bool enabled) {
    if (!engine) {
        return;
    }
    os_unfair_lock_lock(&engine->lock);
    engine->controlScrollEnabled = enabled;
    MPZControlScrollRecognizerReset(&engine->controlScrollRecognizer);
    os_unfair_lock_unlock(&engine->lock);
}

void MPZEngineRefreshDevices(MPZEngine *engine) {
    if (!engine || !MPZEngineIsEnabled(engine)) {
        return;
    }

    CFArrayRef deviceList = createMagicMouseDeviceList();
    if (!deviceList) {
        return;
    }
    if (deviceListMatches(engine, deviceList)) {
        CFRelease(deviceList);
        return;
    }

    bool shouldEndGesture = stopDevices(engine);
    cancelPendingTaps(engine->pendingTapState);
    if (shouldEndGesture) {
        postGesture((MPZPinchOutput){
            .phase = MPZGestureEnded,
            .magnification = 0,
            .suppressScroll = false,
        });
    }
    startDevicesFromList(engine, deviceList);
    CFRelease(deviceList);
}

size_t MPZEngineGetDeviceCount(MPZEngine *engine) {
    if (!engine) {
        return 0;
    }
    os_unfair_lock_lock(&engine->lock);
    size_t count = engine->deviceCount;
    os_unfair_lock_unlock(&engine->lock);
    return count;
}

static int touchCallback(
    MTDeviceRef device,
    const MPZMTTouch *touches,
    CFIndex touchCount,
    CFTimeInterval timestamp,
    uint32_t frame,
    void *context
) {
    (void)device;
    (void)frame;

    MPZDeviceEntry *entry = context;
    MPZEngine *engine = entry->engine;
    MPZPoint points[3];
    MPZTapTouch tapTouches[4];
    size_t pointCount = 0;
    size_t tapTouchCount = 0;
    size_t activeTouchCount = 0;

    for (CFIndex index = 0; index < touchCount; index++) {
        const MPZMTTouch *touch = &touches[index];
        if (touch->phase < MPZMTTouchPhaseDidDown
            || touch->phase > MPZMTTouchPhaseWillUp
            || touch->zTotal < 0.15
            || touch->location.x < 0.02
            || touch->location.x > 0.98) {
            continue;
        }
        activeTouchCount++;
        if (pointCount < 3) {
            points[pointCount++] = (MPZPoint){
                .x = touch->location.x,
                .y = touch->location.y,
            };
        }
        if (tapTouchCount < 4) {
            tapTouches[tapTouchCount++] = (MPZTapTouch){
                .identifier = touch->fingerID,
                .location = {touch->location.x, touch->location.y},
            };
        }
    }

    if (activeTouchCount > pointCount) {
        pointCount = 3;
    }
    if (activeTouchCount > tapTouchCount) {
        tapTouchCount = 4;
    }

    os_unfair_lock_lock(&engine->lock);
    if (!engine->enabled || !entry->attached) {
        os_unfair_lock_unlock(&engine->lock);
        return 0;
    }
    if (activeTouchCount > 0) {
        engine->lastMagicMouseTouchTimestamp = CFAbsoluteTimeGetCurrent();
    }

    MPZPinchOutput output = {0};
    if (engine->pinchEnabled) {
        output = MPZPinchRecognizerProcess(
            &entry->recognizer,
            points,
            pointCount,
            engine->sensitivity,
            timestamp,
            engine->pinchDirectionLocked
        );
    } else {
        MPZPinchRecognizerReset(&entry->recognizer);
    }

    if (output.suppressScroll) {
        engine->activeDevice = entry;
    } else if (output.phase == MPZGestureEnded && engine->activeDevice == entry) {
        engine->activeDevice = NULL;
    }

    MPZTapAction tapAction = MPZTapNone;
    if (engine->tapClickEnabled || engine->middleTapEnabled) {
        bool pinchConflict = output.suppressScroll
            || entry->recognizer.state == MPZRecognizerActive;
        tapAction = MPZTapRecognizerProcess(
            &entry->tapRecognizer,
            tapTouches,
            tapTouchCount,
            timestamp,
            pinchConflict
        );
        if ((tapAction == MPZTapLeftClick || tapAction == MPZTapRightClick)
            && !engine->tapClickEnabled) {
            tapAction = MPZTapNone;
        } else if (tapAction == MPZTapMiddleClick && !engine->middleTapEnabled) {
            tapAction = MPZTapNone;
        }
    } else {
        MPZTapRecognizerReset(&entry->tapRecognizer);
    }
    uint64_t clickDelayNanoseconds = kDefaultClickGuardNanoseconds;
    if (tapAction == MPZTapLeftClick || tapAction == MPZTapRightClick) {
        clickDelayNanoseconds = engine->tapClickDelayNanoseconds;
    }
    scheduleMouseClick(engine->pendingTapState, tapAction, clickDelayNanoseconds);
    postGesture(output);
    os_unfair_lock_unlock(&engine->lock);
    return 0;
}

static CGEventRef eventTapCallback(
    CGEventTapProxy proxy,
    CGEventType type,
    CGEventRef event,
    void *context
) {
    (void)proxy;
    MPZEngine *engine = context;

    if (type == kCGEventTapDisabledByTimeout
        || type == kCGEventTapDisabledByUserInput) {
        if (engine->eventTap) {
            CGEventTapEnable(engine->eventTap, true);
        }
        return event;
    }

    if (type == kCGEventLeftMouseDown
        || type == kCGEventRightMouseDown
        || type == kCGEventOtherMouseDown) {
        if (CGEventGetIntegerValueField(event, kCGEventSourceUserData)
            == MPZSyntheticEventTag) {
            return event;
        }
        os_unfair_lock_lock(&engine->lock);
        cancelPendingTaps(engine->pendingTapState);
        for (size_t index = 0; index < engine->deviceCount; index++) {
            MPZTapRecognizerInvalidate(&engine->devices[index]->tapRecognizer);
        }
        os_unfair_lock_unlock(&engine->lock);
        return event;
    }

    os_unfair_lock_lock(&engine->lock);
    bool suppress = engine->enabled && engine->activeDevice != NULL;
    bool suppressControlScroll = false;
    MPZControlScrollAction controlScrollAction = MPZControlScrollNone;
    if (type == kCGEventScrollWheel && !suppress) {
        cancelPendingTaps(engine->pendingTapState);
        for (size_t index = 0; index < engine->deviceCount; index++) {
            MPZTapRecognizerInvalidate(&engine->devices[index]->tapRecognizer);
        }

        CGEventFlags flags = CGEventGetFlags(event);
        bool controlPressed = (flags & kCGEventFlagMaskControl) != 0;
        bool continuous = CGEventGetIntegerValueField(
            event,
            kCGScrollWheelEventIsContinuous
        ) != 0;
        CFTimeInterval elapsed = CFAbsoluteTimeGetCurrent()
            - engine->lastMagicMouseTouchTimestamp;
        bool fromMagicMouse = continuous
            && engine->lastMagicMouseTouchTimestamp > 0
            && elapsed >= 0
            && elapsed <= kRecentMagicMouseTouchInterval;

        if (engine->controlScrollEnabled && controlPressed && fromMagicMouse) {
            suppressControlScroll = true;
            int64_t scrollPhase = CGEventGetIntegerValueField(
                event,
                kCGScrollWheelEventScrollPhase
            );
            if ((scrollPhase & kCGScrollPhaseBegan) != 0) {
                MPZControlScrollRecognizerReset(&engine->controlScrollRecognizer);
            }
            int64_t momentumPhase = CGEventGetIntegerValueField(
                event,
                kCGScrollWheelEventMomentumPhase
            );
            if (momentumPhase == 0) {
                controlScrollAction = MPZControlScrollRecognizerProcess(
                    &engine->controlScrollRecognizer,
                    MPZPhysicalScrollDeltaY(event),
                    CFAbsoluteTimeGetCurrent(),
                    true,
                    true
                );
            }
        } else {
            MPZControlScrollRecognizerReset(&engine->controlScrollRecognizer);
        }
    } else if (type == kCGEventScrollWheel) {
        MPZControlScrollRecognizerReset(&engine->controlScrollRecognizer);
    }
    os_unfair_lock_unlock(&engine->lock);

    showWindowOverview(controlScrollAction);
    if ((suppress || suppressControlScroll) && type == kCGEventScrollWheel) {
        return NULL;
    }
    return event;
}
