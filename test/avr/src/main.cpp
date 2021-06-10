#include <avr/sleep.h>
#include "..\..\DivideTest.h"

void setup() {
  noInterrupts(); // Needed so simavr will exit on sleep
  Serial.begin(MONITOR_SPEED);

  run_test<int32_t>();
  run_test<uint32_t>();
  run_test<int64_t>();
  run_test<uint64_t>();

  PRINT_INFO(F("\nAll tests passed successfully!\n"));
  // Nothing more to do.
  // Also flags simavr to exit gracefully.
  sleep_cpu();
}

void loop() {
}