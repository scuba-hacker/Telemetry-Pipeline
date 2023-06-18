#define M5_STICK_C_PLUS

#ifdef M5_STICK_C_PLUS
  #include <M5StickCPlus.h>
#endif

#include "Test_TelemetryPipeline.h"

void setup() 
{
#ifdef M5_STICK_C_PLUS
  M5.begin();
#endif

  Test_TelemetryPipeline pipeTest;

  pipeTest.test1();
  pipeTest.test2();
  pipeTest.test3();
  pipeTest.test4();
  pipeTest.test5();
  pipeTest.test6();
}

void loop() {
  // put your main code here, to run repeatedly:
}
