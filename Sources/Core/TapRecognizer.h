#pragma once

#include "PinchRecognizer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t identifier;
    MPZPoint location;
} MPZTapTouch;

typedef enum {
    MPZTapNone = 0,
    MPZTapLeftClick,
    MPZTapRightClick,
    MPZTapMiddleClick,
} MPZTapAction;

typedef struct {
    bool tracking;
    bool invalid;
    size_t maximumTouchCount;
    double startTimestamp;
    double firstTouchX;
    bool hasInitialLocation[16];
    MPZPoint initialLocations[16];
} MPZTapRecognizer;

double MPZTapClickDelayClampMilliseconds(double milliseconds);

void MPZTapRecognizerReset(MPZTapRecognizer *recognizer);
void MPZTapRecognizerInvalidate(MPZTapRecognizer *recognizer);

MPZTapAction MPZTapRecognizerProcess(
    MPZTapRecognizer *recognizer,
    const MPZTapTouch *touches,
    size_t touchCount,
    double timestamp,
    bool conflictingGesture
);
