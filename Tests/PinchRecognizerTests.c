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
    return MPZPinchRecognizerProcess(recognizer, points, 2, sensitivity);
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

    output = MPZPinchRecognizerProcess(&recognizer, NULL, 0, 1.0);
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
        1.0
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

    process(&normal, (MPZPoint){0.30, 0.50}, (MPZPoint){0.70, 0.50}, 1.0);
    process(&fast, (MPZPoint){0.30, 0.50}, (MPZPoint){0.70, 0.50}, 2.0);

    MPZPinchOutput normalOutput = process(
        &normal,
        (MPZPoint){0.293, 0.50},
        (MPZPoint){0.707, 0.50},
        1.0
    );
    MPZPinchOutput fastOutput = process(
        &fast,
        (MPZPoint){0.293, 0.50},
        (MPZPoint){0.707, 0.50},
        2.0
    );

    assert(normalOutput.phase == MPZGestureBegan);
    assert(fastOutput.phase == MPZGestureBegan);
    assert(fabs(fastOutput.magnification - normalOutput.magnification * 2.0) < 0.000001);
}

int main(void) {
    testSpreadGesture();
    testPinchGesture();
    testParallelScrollIsRejected();
    testJitterDoesNotBegin();
    testSecondFingerMayArriveLater();
    testSensitivityScalesOutput();
    puts("PinchRecognizerTests: all tests passed");
    return 0;
}
