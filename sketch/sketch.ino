/*
 * Auto-Generated by NUKE!
 *   http://arbotix.googlecode.com
 *
 * See http://code.google.com/p/arbotix/wiki/NukeIntro
 *   for details on usage.
 */

#include <ax12.h>
#include <BioloidController.h>
#include <Commander.h>
#include "nuke.h"

Commander command = Commander();
/* The Commander protocol has values of -100 to 100, x/y/z speed are in mm/s
 * To go faster than 100mm/s, we can use this multiplier
 */
int multiplier;
int countH = 0;
int countV = 0;
int timestamp = 0;
int mil;
int turret_up_limit;
int turret_down_limit;
int turret_h_limit;
int returnVel;
int deathband;
float voltage = 0;


void setup(){
    pinMode(0,OUTPUT); // LED
    pinMode(2, OUTPUT); // shooting control
    pinMode(13, OUTPUT); // pin to cycle servo power
    digitalWrite(13, LOW);

    // Sets motor positions
    SetPosition(13,512);
    SetPosition(14,512);


    // setup IK
    setupIK();
    gaitSelect(RIPPLE);
    // setup serial for usage with the Commander
    Serial.begin(38400);

    // wait, then check the voltage (LiPO safety)
    delay (1000);
    for(int i = 1; i < 15; i++){
      voltage += (ax12GetRegister (i, AX_PRESENT_VOLTAGE, 1));
    }
    voltage /= 140.0;
    Serial.print ("System Voltage: ");
    Serial.print (voltage);
    Serial.println (" volts.");
    if (voltage < 10.0)
        while(1);

    // stand up slowly
    bioloid.poseSize = 12;
    bioloid.readPose();
    doIK();
    bioloid.interpolateSetup(1000);
    while(bioloid.interpolating > 0){
        bioloid.interpolateStep();
        delay(3);
    }
    multiplier = 1.5;
}

void loop(){
  // take commands
  if(command.ReadMsgs() > 0){
    // select gaits
    if(command.buttons&BUT_R1){ gaitSelect(RIPPLE_SMOOTH); multiplier=1;}
    if(command.buttons&BUT_R2){ gaitSelect(RIPPLE); multiplier=1;}
    if(command.buttons&BUT_L4){ gaitSelect(AMBLE_SMOOTH); multiplier=2;}
    if(command.buttons&BUT_L5){ gaitSelect(AMBLE); multiplier=2;}

    // set speeds
    // Xspeed = multiplier*command.walkV;
    // if(command.leftTrigger == 0) {
    //       Rspeed = -(multiplier*command.walkH)/250.0;
    // } else {
    //       Yspeed = (multiplier*command.walkH)/2;
    // }
    // bodyRotX = 0;
    // bodyRotY = 0;
    // bodyRotZ = 0;
  // }
    Xspeed = multiplier*command.walkV;
    if((command.buttons&BUT_LT) > 0)
      Yspeed = (multiplier*command.walkH)/2;
    else
      Rspeed = -(multiplier*command.walkH)/250.0;

    // bodyRotY = (((float)command.lookV))/250.0;
    // if((command.buttons&BUT_RT) > 0)
    //   bodyRotX = ((float)command.lookH)/250.0;
    // else
    //   bodyRotZ = ((float)command.lookH)/250.0;
    }

  //
  mil = millis();
  if(command.rightTrigger == 1) {
    if((mil - timestamp) < 1100){
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
    } else {
      digitalWrite(0, LOW);
      digitalWrite(2, LOW);
    }
  }  else if(command.rightTrigger == 0){
    timestamp = mil;
    digitalWrite(0, LOW);
    digitalWrite(2, LOW);
  }


  // maximum values for countV and countH
  turret_h_limit = 511;
  turret_up_limit = 40;
  turret_down_limit = -125;

  bool logicV = ((countV > turret_down_limit && countV < turret_up_limit) ||
      (countV <= turret_down_limit && command.lookV > 0) ||
        (countV >= turret_up_limit && command.lookV < 0));

  bool logicH = ((abs(countH) < turret_h_limit) ||
        (countH <= -turret_h_limit && command.lookH > 0) ||
          (countH >= turret_h_limit && command.lookH < 0));


  // move gun
  if(command.leftBumper == 1){
    if(logicV)
      countV +=  (int) abs(command.lookV)/command.lookV;

    if(logicH)
      countH +=  (int) abs(command.lookH)/command.lookH;

  } else {
    if(logicV)
       countV +=  (int) command.lookV/18;

    if(logicH)
        countH +=  (int) command.lookH/18;
  }
  // if(logicH)
    SetPosition(13, 512 - countH);
  // if(logicV)
    SetPosition(14, 512 + countV);


  returnVel = 5;
  deathband = 2;
  if(command.squareButton == 1) {
    if(countH >= deathband) {
        countH -= returnVel;
        SetPosition(13, 512 - countH);
      } else if(countH < -deathband) {
        countH += returnVel;
        SetPosition(13, 512 - countH);
    }
    if(countV >= deathband) {
        countV -= returnVel;
        SetPosition(14, 512 + countV);
      } else if(countV < -deathband) {
        countV += returnVel;
        SetPosition(14, 512 + countV);
    }
  }


  if(command.triangleButton == 1){
    digitalWrite(13, HIGH);
    while(command.triangleButton == 1){;}
    digitalWrite(13, LOW);
  }



  // if our previous interpolation is complete, recompute the IK
  if(bioloid.interpolating == 0){
    doIK();
    bioloid.interpolateSetup(tranTime);
  }

  // update joints
  bioloid.interpolateStep();
}

