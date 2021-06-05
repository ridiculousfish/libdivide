#include "..\..\DivideTest.h"

void setup() {
  Serial.begin(MONITOR_SPEED);

  run_test<int32_t>();
  run_test<uint32_t>();
  run_test<int64_t>();
  run_test<uint64_t>();

  PRINT_INFO("\nAll tests passed successfully!\n");
}

void loop() {
  // put your main code here, to run repeatedly:
}