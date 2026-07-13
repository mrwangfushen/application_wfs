#include "MultitouchSupport.h"

#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>

int main(void) {
    CFArrayRef devices = MTDeviceCreateList();
    if (!devices) {
        fputs("MagicMouseProbe: unable to enumerate multitouch devices\n", stderr);
        return 1;
    }

    size_t magicMouseCount = 0;
    CFIndex count = CFArrayGetCount(devices);
    for (CFIndex index = 0; index < count; index++) {
        MTDeviceRef device = (MTDeviceRef)CFArrayGetValueAtIndex(devices, index);
        uint32_t familyID = 0;
        if (MTDeviceGetFamilyID(device, &familyID) == noErr
            && familyID == MPZMTDeviceFamilyMagicMouse) {
            magicMouseCount++;
        }
    }

    CFRelease(devices);
    printf("MagicMouseProbe: detected %zu Magic Mouse device(s)\n", magicMouseCount);
    return magicMouseCount > 0 ? 0 : 2;
}
