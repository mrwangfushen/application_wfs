#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct __MTDevice *MTDeviceRef;

typedef enum {
    MPZMTDeviceFamilyMagicMouse = 112,
} MPZMTDeviceFamily;

typedef enum {
    MPZMTTouchPhaseNone = 0,
    MPZMTTouchPhaseBegan = 1,
    MPZMTTouchPhaseWillDown = 2,
    MPZMTTouchPhaseDidDown = 3,
    MPZMTTouchPhaseMoved = 4,
    MPZMTTouchPhaseWillUp = 5,
    MPZMTTouchPhaseDidUp = 6,
    MPZMTTouchPhaseEnded = 7,
} MPZMTTouchPhase;

typedef struct {
    float x;
    float y;
} MPZMTPoint;

typedef struct {
    uint32_t frame;
    uint32_t padding1;
    CFTimeInterval timestamp;
    uint32_t pathID;
    MPZMTTouchPhase phase;
    uint32_t fingerID;
    uint32_t padding2;
    MPZMTPoint location;
    MPZMTPoint velocity;
    float zTotal;
    uint32_t padding3;
    float angle;
    float majorAxis;
    float minorAxis;
    MPZMTPoint locationMM;
    MPZMTPoint velocityMM;
    uint32_t padding4[2];
    float zDensity;
} MPZMTTouch;

typedef int (*MPZMTContactCallback)(
    MTDeviceRef device,
    const MPZMTTouch *touches,
    CFIndex touchCount,
    CFTimeInterval timestamp,
    uint32_t frame,
    void *context
);

CFArrayRef MTDeviceCreateList(void);
OSStatus MTDeviceStart(MTDeviceRef device, int unknown);
OSStatus MTDeviceStop(MTDeviceRef device);
OSStatus MTDeviceGetFamilyID(MTDeviceRef device, uint32_t *familyID);
io_service_t MTDeviceGetService(MTDeviceRef device);

void MTRegisterContactFrameCallbackWithRefcon(
    MTDeviceRef device,
    MPZMTContactCallback callback,
    void *context
);

void MTUnregisterContactFrameCallback(
    MTDeviceRef device,
    MPZMTContactCallback callback
);
