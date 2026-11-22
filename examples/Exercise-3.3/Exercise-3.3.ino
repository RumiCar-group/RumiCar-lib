#include <RumiCar.h>

// Tentative values. When you change them on your car, change one at a time.
const int LIMIT_MM = 200;   // the distance RC_read trusts [mm]. Beyond it RC_read returns -3 (out of range).
                            // The RumiCar model 32 picks up the floor at about 30 cm; 200 mm leaves a margin.
const int BAND_MM  = 30;    // with walls on both sides, go straight if left and right differ by this much or less [mm]
                            // (larger than the sensor error of 1-2 cm)

int ipower = 128;           // steering strength 0-255 (start at half; it changes by itself while driving,
                            // 1.3 times or half at a time - rough guides)
int lastSide = CENTER;      // the side we steered to last time
int lastErr = 0;            // the error last time [mm]
int lastCase = -1;          // the situation last time (see below)

void setup()
{
  RC_setup();
  RC_setReadLimit(LIMIT_MM);  // beyond LIMIT_MM, RC_read returns -3
}

void loop()
{
  int s0 = RC_read(LEFT);
  int s1 = RC_read(CENTER);
  int s2 = RC_read(RIGHT);

  Serial.print("Sensor0:");   // print the values as RC_read returned them (-3 is out of range, not a distance)
  Serial.print(s0);
  Serial.print("  Sensor1:");
  Serial.print(s1);
  Serial.print("  Sensor2:");
  Serial.print(s2);
  Serial.print("  Power:");
  Serial.println(ipower);

  if (s0 < 0 || s1 < 0 || s2 < 0) {          // status: a negative value is not a distance
    if (s0 == -1 || s1 == -1 || s2 == -1) {  // -1: a sensor did not answer, so stop
      RC_drive(BRAKE, 0);
      return;
    }
    // -3 (out of range): nothing was measured before LIMIT_MM in that direction.
    // Below, it means "no wall is seen on that side".
  }

  // Speed: decided by the front (center) sensor
  if (s1 == -3) {             // nothing before LIMIT_MM
    RC_drive(FORWARD, 255);
  } else if (s1 < 100) {      // keep these thresholds at or below LIMIT_MM
    RC_drive(REVERSE, 150);
  } else if (s1 < 150) {
    RC_drive(FORWARD, 150);
  } else if (s1 < 200) {
    RC_drive(FORWARD, 200);
  } else {
    RC_drive(FORWARD, 255);
  }

  // Direction: for each situation, decide the side to steer to and the error
  bool seeLeft = (s0 >= 0);   // a wall is seen on the left (RC_read returned a distance)
  bool seeRight = (s2 >= 0);
  int theCase;                // the situation
  int side;                   // the side to steer to (CENTER = go straight)
  int err;                    // the error [mm]: how far we are from what we want
  if (seeLeft && seeRight) {         // walls on both sides: drive in the middle
    theCase = 0;
    err = abs(s0 - s2);
    if (err <= BAND_MM) {
      side = CENTER;
    } else if (s0 > s2) {
      side = LEFT;
    } else {
      side = RIGHT;
    }
  } else if (seeRight) {             // only the right wall: move away from it
    theCase = 1;
    err = LIMIT_MM - s2;      // the closer the wall, the larger the error
    side = LEFT;
  } else if (seeLeft) {              // only the left wall: move away from it
    theCase = 2;
    err = LIMIT_MM - s0;
    side = RIGHT;
  } else {                           // no wall on either side: go straight
    theCase = 3;
    err = 0;
    side = CENTER;
  }
  // We do not know how far an unseen wall is (-3), so we do not guess a distance for it:
  // with one wall, we only move away from the wall we can see.

  if (theCase != lastCase) {
    // The situation changed (a wall or a pole appeared or disappeared). That change was not
    // caused by our steering, so do not judge the strength by comparing with last time.
    lastSide = CENTER;
  }

  if (side == CENTER) {
    RC_steer(CENTER);
  } else {
    if (lastSide != CENTER && side != lastSide) {
      ipower = ipower / 2;          // went past the target: too strong, use half
    } else if (side == lastSide && err >= lastErr) {
      ipower = ipower * 13 / 10;    // the error is not getting smaller: too weak, use 1.3 times
    }
    ipower = constrain(ipower, 40, 255);  // keep at least 40 so that 1.3 times can grow again
    RC_steer(side, ipower);
    lastSide = side;
  }
  lastErr = err;
  lastCase = theCase;
}
