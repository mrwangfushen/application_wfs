#include "ControlScrollRecognizer.h"

#include <string.h>

static const double kActivationDelta = 30.0;
static const double kGestureIdleInterval = 0.30;

void MPZControlScrollRecognizerReset(MPZControlScrollRecognizer *recognizer) {
    memset(recognizer, 0, sizeof(*recognizer));
}

MPZControlScrollAction MPZControlScrollRecognizerProcess(
    MPZControlScrollRecognizer *recognizer,
    double physicalDeltaY,
    double timestamp,
    bool controlPressed,
    bool fromMagicMouse
) {
    if (!controlPressed || !fromMagicMouse) {
        MPZControlScrollRecognizerReset(recognizer);
        return MPZControlScrollNone;
    }

    if (recognizer->lastTimestamp > 0
        && (timestamp < recognizer->lastTimestamp
            || timestamp - recognizer->lastTimestamp > kGestureIdleInterval)) {
        MPZControlScrollRecognizerReset(recognizer);
    }
    recognizer->lastTimestamp = timestamp;

    if (recognizer->triggered || physicalDeltaY == 0) {
        return MPZControlScrollNone;
    }

    if (recognizer->accumulatedDeltaY * physicalDeltaY < 0) {
        recognizer->accumulatedDeltaY = physicalDeltaY;
    } else {
        recognizer->accumulatedDeltaY += physicalDeltaY;
    }

    if (recognizer->accumulatedDeltaY >= kActivationDelta) {
        recognizer->triggered = true;
        return MPZControlScrollShowAllWindows;
    }
    if (recognizer->accumulatedDeltaY <= -kActivationDelta) {
        recognizer->triggered = true;
        return MPZControlScrollShowApplicationWindows;
    }
    return MPZControlScrollNone;
}
