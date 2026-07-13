#pragma once

#include <stdbool.h>

typedef enum {
    MPZControlScrollNone = 0,
    MPZControlScrollShowApplicationWindows,
    MPZControlScrollShowAllWindows,
} MPZControlScrollAction;

typedef struct {
    double accumulatedDeltaY;
    double lastTimestamp;
    bool triggered;
} MPZControlScrollRecognizer;

void MPZControlScrollRecognizerReset(MPZControlScrollRecognizer *recognizer);

MPZControlScrollAction MPZControlScrollRecognizerProcess(
    MPZControlScrollRecognizer *recognizer,
    double physicalDeltaY,
    double timestamp,
    bool controlPressed,
    bool fromMagicMouse
);
