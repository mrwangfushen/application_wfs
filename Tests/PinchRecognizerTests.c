#include "PinchRecognizer.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static MPZPinchOutput process(
    MPZPinchRecognizer *recognizer,
    MPZPoint a,
    MPZPoint b,
    double sensitivity
) {
    MPZPoint points[] = {a, b};
    return MPZPinchRecognizerProcess(recognizer, points, 2, sensitivity, 0, false);
}

static MPZPinchOutput processAt(
    MPZPinchRecognizer *recognizer,
    MPZPoint a,
    MPZPoint b,
    double sensitivity,
    double timestamp
) {
    MPZPoint points[] = {a, b};
    return MPZPinchRecognizerProcess(
        recognizer,
        points,
        2,
        sensitivity,
        timestamp,
        false
    );
}

static void testSpreadGesture(void) {
    MPZPinchRecognizer recognizer = {0};
    MPZPinchOutput output;

    output = process(&recognizer, (MPZPoint){0.40, 0.50}, (MPZPoint){0.60, 0.50}, 1.0);
    assert(output.phase == MPZGestureNone);

    output = process(&recognizer, (MPZPoint){0.39, 0.50}, (MPZPoint){0.61, 0.50}, 1.0);
    assert(output.phase == MPZGestureBegan);
    assert(output.magnification > 0);
    assert(output.suppressScroll);

    output = process(&recognizer, (MPZPoint){0.38, 0.50}, (MPZPoint){0.62, 0.50}, 1.0);
    assert(output.phase == MPZGestureChanged);
    assert(output.magnification > 0);

    output = MPZPinchRecognizerProcess(&recognizer, NULL, 0, 1.0, 0, false);
    assert(output.phase == MPZGestureEnded);
}

static void testPinchGesture(void) {
    MPZPinchRecognizer recognizer = {0};
    MPZPinchOutput output;

    process(&recognizer, (MPZPoint){0.35, 0.50}, (MPZPoint){0.65, 0.50}, 1.0);
    output = process(&recognizer, (MPZPoint){0.37, 0.50}, (MPZPoint){0.63, 0.50}, 1.0);
    assert(output.phase == MPZGestureBegan);
    assert(output.magnification < 0);
}

static void testParallelScrollIsRejected(void) {
    MPZPinchRecognizer recognizer = {0};
    MPZPinchOutput output;

    process(&recognizer, (MPZPoint){0.40, 0.40}, (MPZPoint){0.60, 0.40}, 1.0);
    output = process(&recognizer, (MPZPoint){0.40, 0.44}, (MPZPoint){0.60, 0.44}, 1.0);
    assert(output.phase == MPZGestureNone);
    assert(recognizer.state == MPZRecognizerBlocked);

    output = process(&recognizer, (MPZPoint){0.38, 0.44}, (MPZPoint){0.62, 0.44}, 1.0);
    assert(output.phase == MPZGestureNone);
}

static void testJitterDoesNotBegin(void) {
    MPZPinchRecognizer recognizer = {0};

    process(&recognizer, (MPZPoint){0.40, 0.50}, (MPZPoint){0.60, 0.50}, 1.0);
    MPZPinchOutput output = process(
        &recognizer,
        (MPZPoint){0.397, 0.501},
        (MPZPoint){0.603, 0.501},
        1.0
    );
    assert(output.phase == MPZGestureNone);
    assert(recognizer.state == MPZRecognizerPossible);
}

static void testSecondFingerMayArriveLater(void) {
    MPZPinchRecognizer recognizer = {0};
    MPZPoint firstFinger = {0.40, 0.50};

    MPZPinchOutput output = MPZPinchRecognizerProcess(
        &recognizer,
        &firstFinger,
        1,
        1.0,
        0,
        false
    );
    assert(output.phase == MPZGestureNone);
    assert(recognizer.state == MPZRecognizerIdle);

    output = process(
        &recognizer,
        (MPZPoint){0.40, 0.50},
        (MPZPoint){0.60, 0.50},
        1.0
    );
    assert(output.phase == MPZGestureNone);
    assert(recognizer.state == MPZRecognizerPossible);
}

static void testSensitivityScalesOutput(void) {
    MPZPinchRecognizer normal = {0};
    MPZPinchRecognizer fast = {0};

    process(&normal, (MPZPoint){0.20, 0.50}, (MPZPoint){0.80, 0.50}, 1.0);
    process(&fast, (MPZPoint){0.20, 0.50}, (MPZPoint){0.80, 0.50}, 2.0);

    MPZPinchOutput normalOutput = process(
        &normal,
        (MPZPoint){0.19, 0.50},
        (MPZPoint){0.81, 0.50},
        1.0
    );
    MPZPinchOutput fastOutput = process(
        &fast,
        (MPZPoint){0.19, 0.50},
        (MPZPoint){0.81, 0.50},
        2.0
    );

    assert(normalOutput.phase == MPZGestureBegan);
    assert(fastOutput.phase == MPZGestureBegan);
    assert(fabs(fastOutput.magnification - normalOutput.magnification * 2.0) < 0.000001);
}

static void testSlowJitterCannotBeginGesture(void) {
    MPZPinchRecognizer recognizer = {0};

    processAt(
        &recognizer,
        (MPZPoint){0.40, 0.50},
        (MPZPoint){0.60, 0.50},
        1.0,
        1.00
    );
    processAt(
        &recognizer,
        (MPZPoint){0.395, 0.50},
        (MPZPoint){0.605, 0.50},
        1.0,
        1.20
    );
    MPZPinchOutput output = processAt(
        &recognizer,
        (MPZPoint){0.385, 0.50},
        (MPZPoint){0.615, 0.50},
        1.0,
        1.40
    );

    assert(output.phase == MPZGestureNone);
    assert(recognizer.state == MPZRecognizerBlocked);
}

static void testOneFingerMovementCannotBeginGesture(void) {
    MPZPinchRecognizer recognizer = {0};

    processAt(
        &recognizer,
        (MPZPoint){0.40, 0.50},
        (MPZPoint){0.60, 0.50},
        1.0,
        1.00
    );
    MPZPinchOutput output = processAt(
        &recognizer,
        (MPZPoint){0.40, 0.50},
        (MPZPoint){0.63, 0.50},
        1.0,
        1.10
    );

    assert(output.phase == MPZGestureNone);
    assert(recognizer.state == MPZRecognizerPossible);
}

static void testActiveGestureMayReverseDirection(void) {
    MPZPinchRecognizer recognizer = {0};

    processAt(
        &recognizer,
        (MPZPoint){0.40, 0.50},
        (MPZPoint){0.60, 0.50},
        1.0,
        1.00
    );
    MPZPinchOutput output = processAt(
        &recognizer,
        (MPZPoint){0.39, 0.50},
        (MPZPoint){0.61, 0.50},
        1.0,
        1.10
    );
    assert(output.phase == MPZGestureBegan);
    assert(output.magnification > 0);

    output = processAt(
        &recognizer,
        (MPZPoint){0.40, 0.50},
        (MPZPoint){0.60, 0.50},
        1.0,
        1.15
    );
    assert(output.phase == MPZGestureChanged);
    assert(output.magnification < 0);
}

static void testActiveGestureMayLockDirection(void) {
    MPZPinchRecognizer recognizer = {0};
    MPZPoint points[] = {
        {0.40, 0.50},
        {0.60, 0.50},
    };

    MPZPinchRecognizerProcess(&recognizer, points, 2, 1.0, 1.00, true);
    points[0].x = 0.39;
    points[1].x = 0.61;
    MPZPinchOutput output = MPZPinchRecognizerProcess(
        &recognizer,
        points,
        2,
        1.0,
        1.10,
        true
    );
    assert(output.phase == MPZGestureBegan);
    assert(output.magnification > 0);

    points[0].x = 0.40;
    points[1].x = 0.60;
    output = MPZPinchRecognizerProcess(&recognizer, points, 2, 1.0, 1.15, true);
    assert(output.phase == MPZGestureNone);
    assert(output.suppressScroll);

    points[0].x = 0.385;
    points[1].x = 0.615;
    output = MPZPinchRecognizerProcess(&recognizer, points, 2, 1.0, 1.20, true);
    assert(output.phase == MPZGestureChanged);
    assert(output.magnification > 0);

    output = MPZPinchRecognizerProcess(&recognizer, NULL, 0, 1.0, 1.25, true);
    assert(output.phase == MPZGestureEnded);

    points[0].x = 0.35;
    points[1].x = 0.65;
    MPZPinchRecognizerProcess(&recognizer, points, 2, 1.0, 2.00, true);
    points[0].x = 0.36;
    points[1].x = 0.64;
    output = MPZPinchRecognizerProcess(&recognizer, points, 2, 1.0, 2.10, true);
    assert(output.phase == MPZGestureBegan);
    assert(output.magnification < 0);
}

int main(void) {
    testSpreadGesture();
    testPinchGesture();
    testParallelScrollIsRejected();
    testJitterDoesNotBegin();
    testSecondFingerMayArriveLater();
    testSensitivityScalesOutput();
    testSlowJitterCannotBeginGesture();
    testOneFingerMovementCannotBeginGesture();
    testActiveGestureMayReverseDirection();
    testActiveGestureMayLockDirection();
    puts("PinchRecognizerTests: all tests passed");
    return 0;
}
