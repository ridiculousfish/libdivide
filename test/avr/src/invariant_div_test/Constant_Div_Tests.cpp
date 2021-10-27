#include <Arduino.h>

template<class T1, class T2> struct pair
{
  T1 first;
  T2 second;
};

typedef int32_t (*pCheckSumFunc)();
pair<int32_t, unsigned long> testCheckSum(pCheckSumFunc func)
{
  uint8_t iterations = 2;
  auto startTime = micros();

  int32_t checkSum = 0;
  for (uint8_t loop=0; loop<iterations; ++loop)
  {
    checkSum += func();
  }

  auto stopTime = micros();
   
  return { checkSum, stopTime-startTime };
}

#include "constant_macros.h"
#include "test_declares.g.hpp"

void print_result(const pair<int32_t, unsigned long> &result) {
  Serial.print(result.first); 
  Serial.print(" ");
  Serial.print(result.second);
}

void test_both(test_t Denom, pCheckSumFunc native, pCheckSumFunc libdivide) {
  Serial.print(OP_NAME);
  Serial.print(Denom);
  Serial.print(" ");
  auto nativeResult = testCheckSum(native);
  print_result(nativeResult);
  Serial.print(" ");
  auto libdivResult = testCheckSum(libdivide);
  print_result(libdivResult);
  Serial.println("");
  if (nativeResult.first!=libdivResult.first) { Serial.println("ERROR - Checksum mismatch"); }
}

void run_constant_test() {
  #include "test.g.hpp"
}