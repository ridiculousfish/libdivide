#include <Arduino.h>

template<class T1, class T2> struct pair
{
  T1 first;
  T2 second;
};

typedef unsigned long (*pCheckSumFunc)(unsigned long);
pair<unsigned long, unsigned long> testCheckSum(pCheckSumFunc func)
{
  unsigned long iterations = 2;
  auto startTime = micros();

  unsigned long checkSum = 0;
  for (unsigned long loop=0; loop<iterations; ++loop)
  {
    checkSum += func(checkSum);
  }

  auto stopTime = micros();
   
  return { checkSum, stopTime-startTime };
}

#include "constant_macros.h"
#include "test_declares.g.hpp"

void print_result(const pair<unsigned long, unsigned long> &result) {
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