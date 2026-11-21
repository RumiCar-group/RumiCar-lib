#include <RumiCar.h>

int ipower = 128;       // steering strength 0-255 (start at half)
int lastTurn = CENTER;  // the side we steered to last time
int lastGap = 0;        // left/right difference last time [mm]

void setup()
{
  RC_setup();
  RC_setReadLimit(200); // the distance RC_read trusts [mm]. Beyond it RC_read returns -3 (far)
}

void loop()
{

  int iband = 30;       // if left and right differ by this much or less, go straight [mm]
  int s0, s1, s2;
  s0=RC_read(LEFT);
  s1=RC_read(CENTER);
  s2=RC_read(RIGHT);
  if(s0==-3) s0=250;    // -3: farther than the limit = open road (-3 is the smallest number, so replace it first)
  if(s1==-3) s1=250;
  if(s2==-3) s2=250;


  Serial.print("Sensor0:");
  Serial.print(s0);
  Serial.print("  Sensor1:");
  Serial.print(s1);
  Serial.print("  Sensor2:");
  Serial.print(s2);
  Serial.print("  Power:");
  Serial.println(ipower);

  if(s0==-1 || s1==-1 || s2==-1){
    return;             // -1: a sensor did not answer, keep driving as before
  }

  if(s1<100){           // keep these thresholds at or below the limit in setup()
    RC_drive(REVERSE,150);
  }else if (s1<150){
    RC_drive(FORWARD,150);
  }else if (s1<200){
    RC_drive(FORWARD,200);
  }else{
    RC_drive(FORWARD,255);
  }

  int gap = abs(s0-s2);
  if(gap<=iband){
    RC_steer(CENTER);
  }else{
    int side;
    if(s0>s2){
      side=LEFT;
    }else{
      side=RIGHT;
    }
    if(lastTurn!=CENTER && side!=lastTurn){
      ipower=ipower/2;        // went past the middle: too strong, use half
    }else if(side==lastTurn && gap>=lastGap){
      ipower=ipower*13/10;    // not getting closer to the middle: too weak, use 1.3 times
    }
    ipower=constrain(ipower,40,255);  // keep at least 40 so that 1.3 times can grow again
    RC_steer(side,ipower);
    lastTurn=side;
  }
  lastGap=gap;
}
