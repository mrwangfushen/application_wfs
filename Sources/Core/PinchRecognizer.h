#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    double x;
    double y;
} MPZPoint;

typedef enum {
    MPZGestureNone = 0,
    MPZGestureBegan,
    MPZGestureChanged,
    MPZGestureEnded,
} MPZGesturePhase;

typedef struct {
    MPZGesturePhase phase;
    double magnification;
    bool suppressScroll;
} MPZPinchOutput;

typedef enum {
    MPZRecognizerIdle = 0,
    MPZRecognizerPossible,
    MPZRecognizerActive,
    MPZRecognizerBlocked,
} MPZRecognizerState;

typedef struct {
    MPZRecognizerState state;
    double startDistance;
    double previousDistance;
    MPZPoint startCentroid;
} MPZPinchRecognizer;

void MPZPinchRecognizerReset(MPZPinchRecognizer *recognizer);

MPZPinchOutput MPZPinchRecognizerProcess(
    MPZPinchRecognizer *recognizer,
    const MPZPoint *points,
    size_t pointCount,
    double sensitivity
);
