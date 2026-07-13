#include "TapRecognizer.h"

#include <math.h>
#include <string.h>

static const double kMaximumTapDuration = 0.28;
static const double kMaximumTapMovement = 0.04;
static const double kMinimumTapClickDelayMilliseconds = 50.0;
static const double kMaximumTapClickDelayMilliseconds = 500.0;

double MPZTapClickDelayClampMilliseconds(double milliseconds) {
    return fmin(
        kMaximumTapClickDelayMilliseconds,
        fmax(kMinimumTapClickDelayMilliseconds, milliseconds)
    );
}

void MPZTapRecognizerReset(MPZTapRecognizer *recognizer) {
    memset(recognizer, 0, sizeof(*recognizer));
}

void MPZTapRecognizerInvalidate(MPZTapRecognizer *recognizer) {
    if (recognizer->tracking) {
        recognizer->invalid = true;
    }
}

static double distanceBetween(MPZPoint a, MPZPoint b) {
    return hypot(a.x - b.x, a.y - b.y);
}

MPZTapAction MPZTapRecognizerProcess(
    MPZTapRecognizer *recognizer,
    const MPZTapTouch *touches,
    size_t touchCount,
    double timestamp,
    bool conflictingGesture
) {
    if (touchCount == 0) {
        if (!recognizer->tracking) {
            return MPZTapNone;
        }

        bool valid = !recognizer->invalid
            && timestamp - recognizer->startTimestamp <= kMaximumTapDuration;
        size_t maximumTouchCount = recognizer->maximumTouchCount;
        double firstTouchX = recognizer->firstTouchX;
        MPZTapRecognizerReset(recognizer);

        if (!valid) {
            return MPZTapNone;
        }
        if (maximumTouchCount == 1) {
            return firstTouchX < 0.5 ? MPZTapLeftClick : MPZTapRightClick;
        }
        if (maximumTouchCount == 3) {
            return MPZTapMiddleClick;
        }
        return MPZTapNone;
    }

    if (!recognizer->tracking) {
        MPZTapRecognizerReset(recognizer);
        recognizer->tracking = true;
        recognizer->startTimestamp = timestamp;
        recognizer->firstTouchX = touches[0].location.x;
    }

    if (timestamp - recognizer->startTimestamp > kMaximumTapDuration
        || touchCount > 3
        || conflictingGesture) {
        recognizer->invalid = true;
    }
    if (touchCount > recognizer->maximumTouchCount) {
        recognizer->maximumTouchCount = touchCount;
    }

    for (size_t index = 0; index < touchCount; index++) {
        uint32_t identifier = touches[index].identifier;
        if (identifier >= 16) {
            recognizer->invalid = true;
            continue;
        }

        if (!recognizer->hasInitialLocation[identifier]) {
            recognizer->hasInitialLocation[identifier] = true;
            recognizer->initialLocations[identifier] = touches[index].location;
        } else if (distanceBetween(
            touches[index].location,
            recognizer->initialLocations[identifier]
        ) > kMaximumTapMovement) {
            recognizer->invalid = true;
        }
    }

    return MPZTapNone;
}
