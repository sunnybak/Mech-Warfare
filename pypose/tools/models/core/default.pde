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

void setup(){   
    pinMode(0,OUTPUT);
    // setup IK    
    setupIK();    
    gaitSelect(RIPPLE);
    // setup serial for usage with the Commander
    Serial.begin(38400);

    // wait, then check the voltage (LiPO safety)
    delay (1000);
    float voltage = (ax12GetRegister (1, AX_PRESENT_VOLTAGE, 1)) / 10.0;
    Serial.print ("System Voltage: ");
    Serial.print (voltage);
    Serial.println (" volts.");
    if (voltage < 10.0)
        while(1);

    // stand up slowly
    bioloid.poseSize = @SERVO_COUNT;
    bioloid.readPose();        
    doIK();
    bioloid.interpolateSetup(1000);
    while(bioloid.interpolating > 0){
        bioloid.interpolateStep();
        delay(3);
    }
    multiplier = 1;
}

void loop(){
  // take commands 
  if(command.ReadMsgs() > 0){
    digitalWrite(0,HIGH-digitalRead(0));
    // select gaits
    if(command.buttons&BUT_R1){ gaitSelect(RIPPLE_SMOOTH); multiplier=1;}
    if(command.buttons&BUT_R2){ gaitSelect(RIPPLE); multiplier=1;}
    if(command.buttons&BUT_L4){ gaitSelect(AMBLE_SMOOTH); multiplier=2;}
    if(command.buttons&BUT_L5){ gaitSelect(AMBLE); multiplier=2;}
@IF legs 6
    if(command.buttons&BUT_L6){ gaitSelect(TRIPOD); multiplier=2;}
@END_IF
    // set speeds
    Xspeed = multiplier*command.walkV;
    if((command.buttons&BUT_LT) > 0)
      Yspeed = (multiplier*command.walkH)/2;
    else
      Rspeed = -(multiplier*command.walkH)/250.0;  
    bodyRotY = (((float)command.lookV))/250.0;
    if((command.buttons&BUT_RT) > 0)
      bodyRotX = ((float)command.lookH)/250.0; 
    else
      bodyRotZ = ((float)command.lookH)/250.0;
  }   

  // if our previous interpolation is complete, recompute the IK
  if(bioloid.interpolating == 0){
    doIK();
    bioloid.interpolateSetup(tranTime);
  }

  // update joints
  bioloid.interpolateStep();
}

