#include <Arduino.h>
#include <unity.h>
// #include <avr/power.h>
// #include <avr/wdt.h>
// #include <avr/interrupt.h>
#include <avr/sleep.h>
#include "../../DivideTest.h"

static void test_int16(void) {
    run_test<int16_t>();
}
static void test_uint16(void) {
    run_test<uint16_t>();
}
static void test_int32(void) {
    run_test<int32_t>();
}
static void test_uint32(void) {
    run_test<uint32_t>();
}
static void test_int64(void) {
    run_test<int64_t>();
}
static void test_uint64(void) {
    run_test<uint64_t>();
}

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(MONITOR_SPEED);

#if !defined(SIMULATOR)
    delay(2000);
#endif    

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_int16);
    RUN_TEST(test_uint16);
    RUN_TEST(test_int32);
    RUN_TEST(test_uint32);
    RUN_TEST(test_int64);
    RUN_TEST(test_uint64);

    UNITY_END(); // stop unit testing

    // Tell SimAVR we are done
    cli();
    sleep_enable();
    sleep_cpu();
}

void loop()
{
    // Blink to indicate end of test
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
}