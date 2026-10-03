#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <ArduinoJson.h>

void requestSharedAttributes();

void applySharedAttributes(
    JsonObject attrs
);

void publishClientAttributes();

#endif
