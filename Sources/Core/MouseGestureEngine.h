#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct MPZEngine MPZEngine;

MPZEngine *MPZEngineCreate(void);
void MPZEngineDestroy(MPZEngine *engine);

bool MPZEngineSetEnabled(MPZEngine *engine, bool enabled);
bool MPZEngineIsEnabled(MPZEngine *engine);

void MPZEngineSetSensitivity(MPZEngine *engine, double sensitivity);
void MPZEngineSetTapClickDelayMilliseconds(MPZEngine *engine, double milliseconds);
void MPZEngineSetPinchEnabled(MPZEngine *engine, bool enabled);
void MPZEngineSetPinchDirectionLocked(MPZEngine *engine, bool locked);
void MPZEngineSetTapClickEnabled(MPZEngine *engine, bool enabled);
void MPZEngineSetMiddleTapEnabled(MPZEngine *engine, bool enabled);
void MPZEngineSetControlScrollEnabled(MPZEngine *engine, bool enabled);
void MPZEngineRefreshDevices(MPZEngine *engine);
size_t MPZEngineGetDeviceCount(MPZEngine *engine);
