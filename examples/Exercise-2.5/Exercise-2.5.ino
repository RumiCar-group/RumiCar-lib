#include <RumiCar.h>

void setup()
{
  RC_setup();
}

void loop()
{
  RC_steer(RIGHT,255);
  delay(500);
  RC_steer(LEFT,255);
  delay(500);
  RC_steer(RIGHT,128);
  delay(500);
  RC_steer(LEFT,128);
  delay(500);
}
