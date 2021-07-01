#include <avr/sleep.h>
#if defined(BENCHMARK)
#include "..\..\benchmark.h"
#else
#include "..\..\DivideTest.h"
#endif

void setup() {
  Serial.begin(MONITOR_SPEED);

#if defined(BENCHMARK)

#if defined(U16)
    test_many<uint16_t>();
#elif defined(S16)
    test_many<int16_t>();
#elif defined(U32)
    test_many<uint32_t>();
#elif defined(S32)
    test_many<int32_t>();
#elif defined(U64)
    test_many<uint64_t>();
#else
    test_many<int64_t>();
#endif

#else
  run_test<int16_t>();
  run_test<uint16_t>();
  run_test<int32_t>();
  run_test<uint32_t>();
  run_test<int64_t>();
  run_test<uint64_t>();

  PRINT_INFO(F("\nAll tests passed successfully!\n"));
#endif

  Serial.flush();
  // Nothing more to do.
  // Also flags simavr to exit gracefully.
  noInterrupts(); // Needed so simavr will exit on sleep
  sleep_cpu();
}

void loop() {
}