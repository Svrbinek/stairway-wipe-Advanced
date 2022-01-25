#include "wled.h"

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This is Stairway-Wipe as a v2 usermod.
 * 
 * Using this usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "stairway-wipe-usermod-v2.h" in the top and registerUsermod(new StairwayWipeUsermod()) in the bottom of usermods_list.cpp
 */

class StairwayWipeUsermod : public Usermod {
  private:
    //Private class members. You can declare variables and functions only accessible to your usermod here
    unsigned long lastTime = 0;
    byte wipeState = 0; //0: inactive 1: wiping 2: solid
    unsigned long timeStaticStart = 0;
    uint16_t previousUserVar0 = 0;
//    uint16_t uv0 = 0;
//    uint16_t puv0 = 0;
//    byte wipest = 0;
//    byte prvni_vypis = 1;

//comment this out if you want the turn off effect to be just fading out instead of reverse wipe
#define STAIRCASE_WIPE_OFF
  public:

    void loop() {
  //userVar0 (U0 in HTTP API):
  //has to be set to 1 if movement is detected on the PIR that is the same side of the staircase as the ESP8266
  //has to be set to 2 if movement is detected on the PIR that is the opposite side
  //can be set to 0 if no movement is detected. Otherwise LEDs will turn off after a configurable timeout (userVar1 seconds)
//  if (prvni_vypis) {
//    Serial.println((String)"Start:");
//    Serial.println((String)"userVar0:"+userVar0+" previousUserVar0:"+previousUserVar0+" wipeState:"+wipeState);
//    prvni_vypis = 0;
//  }
//  if ((previousUserVar0 != puv0) || (userVar0 != uv0) || (wipeState != wipest)) {
//    Serial.println();
//    if (userVar0 != uv0) Serial.println((String)"Zmena> userVar0 z:"+uv0+" na:"+userVar0);
//    Serial.println((String)"D> userVar0:"+userVar0+" previousUserVar0:"+previousUserVar0+" wipeState:"+wipeState);
//    puv0 = previousUserVar0;
//    uv0 = userVar0;
//    wipest = wipeState;
//  }

  if (userVar0 > 0)
  {
    if ((previousUserVar0 == 1 && userVar0 == 2) || (previousUserVar0 == 2 && userVar0 == 1)) wipeState = 3; //turn off if other PIR triggered
    previousUserVar0 = userVar0;
  } else {
    resetuj();
  }    
  switch(wipeState) {
    case 0: {
      if (userVar0 > 0) {
        startWipe();
        wipeState = 1;
        break;
      }
    }
    case 1: {
      if (userVar0 > 0) {
        //wiping
 //       Serial.print((String)"1");
        uint32_t cycleTime = 360 + (255 - effectSpeed)*75; //this is how long one wipe takes (minus 25 ms to make sure we switch in time)
        if (millis() + strip.timebase > (cycleTime - 25)) { //wipe complete
          effectCurrent = FX_MODE_STATIC;
          timeStaticStart = millis();
          colorUpdated(CALL_MODE_NOTIFICATION);
          wipeState = 2;
        }
        break;
      }
    }
    case 2: {
      if (userVar0 > 0) {
        //static
 //       Serial.print((String)"2");
        if (userVar1 > 0) //if U1 is not set, the light will stay on until second PIR or external command is triggered
        {
          if (millis() - timeStaticStart > userVar1*1000) wipeState = 3;
        }
        break;
      }
    }
    case 3: {
      if (userVar0 > 0) {
        //switch to wipe off
 //       Serial.print((String)"----------------------------------------------------------------------------------------------------------------------------------------------------");
        // #ifdef STAIRCASE_WIPE_OFF
        effectCurrent = FX_MODE_COLOR_WIPE;
        strip.timebase = 360 + (255 - effectSpeed)*75 - millis(); //make sure wipe starts fully lit
        colorUpdated(CALL_MODE_NOTIFICATION);
        wipeState = 4;
        // #else
        // turnOff();
        // #endif
        break;
      }
    }
    default: {
      if (userVar0 > 0) {
        
        
 //       Serial.print((String)"x");
        if (millis() + strip.timebase > (725 + (255 - effectSpeed)*150)) turnOff(); //wipe complete
        if (userVar0 != 6) { userVar0 = previousUserVar0; wipeState = 0; }
        break;
      }
    }
  }
}

    void readFromJsonState(JsonObject& root)
    {
      userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }

    uint16_t getId()
    {
      return USERMOD_ID_EXAMPLE;
    }

    void resetuj()
    {
      wipeState = 0; //reset for next time
      if (previousUserVar0) {
        #ifdef STAIRCASE_WIPE_OFF
 //       Serial.println((String)"R+start:");
 //       Serial.println((String)"userVar0:"+userVar0+" previousUserVar0:"+previousUserVar0+" wipeState:"+wipeState);
        // userVar0 = previousUserVar0;
        userVar0 = 6; //neco jineho ve vyznamu 0, testuji v case 4: (default), pokud bude neco jineho menim wipestate
        wipeState = 3;
        #else
        turnOff();
        #endif
//       Serial.println((String)"R+end:");
//       Serial.println((String)"userVar0:"+userVar0+" previousUserVar0:"+previousUserVar0+" wipeState:"+wipeState);
        // Serial.print((String)"r+");
      }
      previousUserVar0 = 0;
//     Serial.print((String)"r-");
    }

    void startWipe()
    {
    applyPreset(2, true);
    bri = briLast; //turn on
    transitionDelayTemp = 0; //no transition
    effectCurrent = FX_MODE_COLOR_WIPE;
    effectSpeed = 240;
    resetTimebase(); //make sure wipe starts from beginning

    //set wipe direction
    WS2812FX::Segment& seg = strip.getSegment(0);
    bool doReverse = (userVar0 == 2);
    seg.setOption(1, doReverse);

    colorUpdated(CALL_MODE_NOTIFICATION);
    }

    void turnOff()
    {
    #ifdef STAIRCASE_WIPE_OFF
    transitionDelayTemp = 0; //turn off immediately after wipe completed
    #else
    transitionDelayTemp = 4000; //fade out slowly
    #endif
    bri = 0;
    applyPreset(3, true);
    colorUpdated(CALL_MODE_NOTIFICATION);
    wipeState = 0;
    userVar0 = 0;
    previousUserVar0 = 0;
    }



   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};