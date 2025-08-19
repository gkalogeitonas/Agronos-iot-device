#pragma once

#include <DHT.h>

// Return a DHT* shared per-pin. Caller does not own the pointer.
DHT* getSharedDht(int pin);
