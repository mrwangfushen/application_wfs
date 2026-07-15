#include "PinchRecognizer.h"

#include <math.h>

static const double kMinimumDistance = 0.03;
static const double kPinchStartDistance = 0.018;
static const double kPinchActivationInterval = 0.35;
static const double kMaximumCentroidToRadialRatio = 0.4;
static const double kScrollCancelDistance = 0.025;
static const double kMinimumChangedValue = 0.0002;
static const double kMaximumChangedValue = 0.08;

static double clamp(double value, double lower, double upper) {
    return fmin(fmax(value, lower), upper);
}

static double distanceBetween(MPZPoint a, MPZPoint b) {
    return hypot(a.x - b.x, a.y - b.y);
}

static MPZPoint centroidOf(MPZPoint a, MPZPoint b) {
    return (MPZPoint){
        .x = (a.x + b.x) / 2.0,
        .y = (a.y + b.y) / 2.0,
    };
}

static MPZPinchOutput noOutput(bool suppressScroll) {
    return (MPZPinchOutput){
        .phase = MPZGestureNone,
        .magnification = 0,
        .suppressScroll = suppressScroll,
    };
}

void MPZPinchRecognizerReset(MPZPinchRecognizer *recognizer) {
    *recognizer = (MPZPinchRecognizer){0};
}

MPZPinchOutput MPZPinchRecognizerProcess(
    MPZPinchRecognizer *recognizer,
    const MPZPoint *points,
    size_t pointCount,
    double sensitivity,
    double timestamp
) {
    if (pointCount != 2) {
        bool wasActive = recognizer->state == MPZRecognizerActive;
        if (pointCount > 2 || (recognizer->state == MPZRecognizerBlocked && pointCount > 0)) {
            recognizer->state = MPZRecognizerBlocked;
        } else if (wasActive && pointCount > 0) {
            recognizer->state = MPZRecognizerBlocked;
        } else {
            recognizer->state = MPZRecognizerIdle;
        }
        recognizer->startDistance = 0;
        recognizer->previousDistance = 0;
        recognizer->startTimestamp = 0;

        if (wasActive) {
            return (MPZPinchOutput){
                .phase = MPZGestureEnded,
                .magnification = 0,
                .suppressScroll = false,
            };
        }
        return noOutput(false);
    }

    if (recognizer->state == MPZRecognizerBlocked) {
        return noOutput(false);
    }

    double distance = distanceBetween(points[0], points[1]);
    MPZPoint centroid = centroidOf(points[0], points[1]);
    if (distance < kMinimumDistance) {
        bool wasActive = recognizer->state == MPZRecognizerActive;
        recognizer->state = MPZRecognizerBlocked;
        if (wasActive) {
            return (MPZPinchOutput){
                .phase = MPZGestureEnded,
                .magnification = 0,
                .suppressScroll = false,
            };
        }
        return noOutput(false);
    }

    if (recognizer->state == MPZRecognizerIdle) {
        recognizer->state = MPZRecognizerPossible;
        recognizer->startDistance = distance;
        recognizer->previousDistance = distance;
        recognizer->startCentroid = centroid;
        recognizer->startTimestamp = timestamp;
        return noOutput(false);
    }

    if (recognizer->state == MPZRecognizerPossible) {
        if (timestamp < recognizer->startTimestamp
            || timestamp - recognizer->startTimestamp > kPinchActivationInterval) {
            recognizer->state = MPZRecognizerBlocked;
            return noOutput(false);
        }

        double radialMovement = distance - recognizer->startDistance;
        double centroidMovement = distanceBetween(centroid, recognizer->startCentroid);
        double absoluteRadialMovement = fabs(radialMovement);

        if (absoluteRadialMovement >= kPinchStartDistance
            && centroidMovement
                < absoluteRadialMovement * kMaximumCentroidToRadialRatio) {
            recognizer->state = MPZRecognizerActive;
            recognizer->previousDistance = distance;
            double value = log(distance / recognizer->startDistance) * sensitivity;
            return (MPZPinchOutput){
                .phase = MPZGestureBegan,
                .magnification = clamp(value, -kMaximumChangedValue, kMaximumChangedValue),
                .suppressScroll = true,
            };
        }

        if (centroidMovement >= kScrollCancelDistance
            && absoluteRadialMovement < kPinchStartDistance) {
            recognizer->state = MPZRecognizerBlocked;
        }
        return noOutput(false);
    }

    double value = log(distance / recognizer->previousDistance) * sensitivity;
    recognizer->previousDistance = distance;
    if (fabs(value) < kMinimumChangedValue) {
        return noOutput(true);
    }

    return (MPZPinchOutput){
        .phase = MPZGestureChanged,
        .magnification = clamp(value, -kMaximumChangedValue, kMaximumChangedValue),
        .suppressScroll = true,
    };
}
