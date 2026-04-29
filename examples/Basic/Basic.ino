#include <RumiCar.h>

void setup() {
  RC_setup();
}

void loop() {
  RC_steer(CENTER);
  RC_drive(FORWARD, 120);
  delay(1000);

  RC_drive(BRAKE, 255);
  delay(200);

  RC_drive(FREE, 0);
  delay(1000);
}
