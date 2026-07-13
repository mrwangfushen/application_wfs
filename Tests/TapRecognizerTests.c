#include "TapRecognizer.h"

#include <assert.h>
#include <stdio.h>

static MPZTapTouch touch(uint32_t identifier, double x, double y) {
    return (MPZTapTouch){
        .identifier = identifier,
        .location = {x, y},
    };
}

static void testLeftAndRightTap(void) {
    MPZTapRecognizer recognizer = {0};
    MPZTapTouch left = touch(1, 0.25, 0.50);
    MPZTapRecognizerProcess(&recognizer, &left, 1, 1.00, false);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 1.12, false) == MPZTapLeftClick);

    MPZTapTouch right = touch(2, 0.75, 0.50);
    MPZTapRecognizerProcess(&recognizer, &right, 1, 2.00, false);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 2.11, false) == MPZTapRightClick);
}

static void testThreeFingerTap(void) {
    MPZTapRecognizer recognizer = {0};
    MPZTapTouch first = touch(1, 0.30, 0.50);
    MPZTapTouch all[] = {
        first,
        touch(2, 0.50, 0.50),
        touch(3, 0.70, 0.50),
    };

    MPZTapRecognizerProcess(&recognizer, &first, 1, 1.00, false);
    MPZTapRecognizerProcess(&recognizer, all, 3, 1.04, false);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 1.15, false) == MPZTapMiddleClick);
}

static void testMovementAndHoldAreRejected(void) {
    MPZTapRecognizer recognizer = {0};
    MPZTapTouch start = touch(1, 0.25, 0.50);
    MPZTapTouch moved = touch(1, 0.25, 0.56);

    MPZTapRecognizerProcess(&recognizer, &start, 1, 1.00, false);
    MPZTapRecognizerProcess(&recognizer, &moved, 1, 1.08, false);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 1.12, false) == MPZTapNone);

    MPZTapRecognizerProcess(&recognizer, &start, 1, 2.00, false);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 2.30, false) == MPZTapNone);
}

static void testConflictsAndUnsupportedCountsAreRejected(void) {
    MPZTapRecognizer recognizer = {0};
    MPZTapTouch one = touch(1, 0.25, 0.50);
    MPZTapTouch two[] = {one, touch(2, 0.75, 0.50)};

    MPZTapRecognizerProcess(&recognizer, &one, 1, 1.00, true);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 1.10, false) == MPZTapNone);

    MPZTapRecognizerProcess(&recognizer, two, 2, 2.00, false);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 2.10, false) == MPZTapNone);

    MPZTapRecognizerProcess(&recognizer, &one, 1, 3.00, false);
    MPZTapRecognizerInvalidate(&recognizer);
    assert(MPZTapRecognizerProcess(&recognizer, NULL, 0, 3.10, false) == MPZTapNone);
}

static void testTapClickDelayBounds(void) {
    assert(MPZTapClickDelayClampMilliseconds(49.0) == 50.0);
    assert(MPZTapClickDelayClampMilliseconds(50.0) == 50.0);
    assert(MPZTapClickDelayClampMilliseconds(275.0) == 275.0);
    assert(MPZTapClickDelayClampMilliseconds(500.0) == 500.0);
    assert(MPZTapClickDelayClampMilliseconds(501.0) == 500.0);
}

int main(void) {
    testLeftAndRightTap();
    testThreeFingerTap();
    testMovementAndHoldAreRejected();
    testConflictsAndUnsupportedCountsAreRejected();
    testTapClickDelayBounds();
    puts("TapRecognizerTests: all tests passed");
    return 0;
}
