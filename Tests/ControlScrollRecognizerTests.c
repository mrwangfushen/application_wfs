#include "ControlScrollRecognizer.h"

#include <assert.h>
#include <stdio.h>

static void testDirections(void) {
    MPZControlScrollRecognizer recognizer = {0};
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, 12, 1.00, true, true
    ) == MPZControlScrollNone);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, 19, 1.05, true, true
    ) == MPZControlScrollShowAllWindows);

    MPZControlScrollRecognizerReset(&recognizer);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, -15, 2.00, true, true
    ) == MPZControlScrollNone);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, -16, 2.05, true, true
    ) == MPZControlScrollShowApplicationWindows);
}

static void testOnlyOneActionPerGesture(void) {
    MPZControlScrollRecognizer recognizer = {0};
    MPZControlScrollRecognizerProcess(&recognizer, 31, 1.00, true, true);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, 31, 1.05, true, true
    ) == MPZControlScrollNone);

    assert(MPZControlScrollRecognizerProcess(
        &recognizer, 31, 1.40, true, true
    ) == MPZControlScrollShowAllWindows);
}

static void testInvalidInputResetsGesture(void) {
    MPZControlScrollRecognizer recognizer = {0};
    MPZControlScrollRecognizerProcess(&recognizer, 20, 1.00, true, true);
    MPZControlScrollRecognizerProcess(&recognizer, 20, 1.05, false, true);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, 11, 1.10, true, true
    ) == MPZControlScrollNone);

    MPZControlScrollRecognizerProcess(&recognizer, 20, 1.15, true, false);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, 11, 1.20, true, true
    ) == MPZControlScrollNone);
}

static void testDirectionReversalRestartsAccumulation(void) {
    MPZControlScrollRecognizer recognizer = {0};
    MPZControlScrollRecognizerProcess(&recognizer, 25, 1.00, true, true);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, -10, 1.05, true, true
    ) == MPZControlScrollNone);
    assert(MPZControlScrollRecognizerProcess(
        &recognizer, -20, 1.10, true, true
    ) == MPZControlScrollShowApplicationWindows);
}

int main(void) {
    testDirections();
    testOnlyOneActionPerGesture();
    testInvalidInputResetsGesture();
    testDirectionReversalRestartsAccumulation();
    puts("ControlScrollRecognizerTests: all tests passed");
    return 0;
}
