/*
 *  Blinks – Speed Racer 
 *  
 *  Start a race by pressing on a single Blink
 *  Make sure the racer has road ahead, otherwise the racer will crash and explode
 *  
 *  When a new blink is added to the end of the line, it chooses the next direction to turn
 *  Place the next available Blink at the end of the line to turn that direction
 *  
 *  Pass the score from Blink to Blink on each transition
 */

#define CAR_FADE_IN_DIST   0.20   // kind of like headlights
#define CAR_FADE_OUT_DIST  1.25   // kind of like a taillight trail

#define MAX_CAR_SPEED      0.25   

// TODO: make fade in/out dist variable based on speed...

#define CAR_UPDATE_DELAY_MS  30   // updates the car position every n milliseconds

Timer updateCarTimer;

float carPosition = -CAR_FADE_IN_DIST;
float carSpeed = 0.03;  // travels this far every update delay ms

byte rotate = 0;

/*
   This map() functuion is now in Arduino.h in /dev
   It is replicated here so this skect can compile on older API commits
*/

float map_f(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
 
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  if( buttonSingleClicked() ) {
    // change car speed
    carSpeed += 0.01;

    if( carSpeed > MAX_CAR_SPEED ) {
      carSpeed = 0.01;
    }
  }

  if( buttonDoubleClicked() ) {
    rotate++;
    if(rotate >5) {
      rotate = 0;
    }
  }

  if( updateCarTimer.isExpired() ) {
    updateCarTimer.set( CAR_UPDATE_DELAY_MS );
    carPosition += carSpeed;

    if(carPosition == 1.0) {
      // the car is at the edge, should pass to the next Blink
      // if there is a Blink to receive success
      // if no Blink to receive, then we crash and explode
    }
    else if(carPosition >= 1.0 + CAR_FADE_OUT_DIST) {
      // car is now completely passed us
      
      // for testing purposes, just loop back to where we started
      if(carPosition >= 1.0 + 2 * CAR_FADE_OUT_DIST) {
        carPosition = -CAR_FADE_IN_DIST;
      }
    }
  }

  // display the car
  FOREACH_FACE( f ) {
    setFaceColor( f, getFaceColorBasedOnCarPossition( f, carPosition, (0 + rotate)%6, (3 + rotate)%6) );
  }
}

/*
 * fade from the first side to the opposite side
 * front of the fade should be faster than the fall off
 * 
 */
Color getFaceColorBasedOnCarPossition(byte face, float pos, byte from, byte to) {
  byte hue, saturation, brightness;
   
  // are we going straight, turning left, or turning right
  if ( (from + 6 - to) % 6 == 3 ) {

    float center;
    byte faceRotated = (face + from) % 6; 
    switch( faceRotated ) { //... rotate to the correct direction
      case 0: center = 0.0;  break;
      case 1: center = 0.25; break;
      case 2: center = 0.75; break;
      case 3: center = 1.0;  break;
      case 4: center = 0.75; break;
      case 5: center = 0.25; break;
    }

    // we are traveling straight
    if( pos < -CAR_FADE_IN_DIST + center || pos > CAR_FADE_OUT_DIST + center ) {
      // out of range for us...
      brightness = 0;
    }
    
    else if( pos < center ) {
        // fade in
        brightness = (byte) map_f(pos, -CAR_FADE_IN_DIST + center, center, 0, 255);
    }

    else if( pos == center ) {
        brightness = 255;
    }
    
    else if( pos > center ) {
      // fade out
      brightness = (byte) map_f(pos, center, CAR_FADE_OUT_DIST + center, 255, 0);
    }
    
  }
  
  else if ( (from + 6 - to) % 6 == 2 ) { // if ( (from + 6 - to) % 6 == 2 )
    // we are turning right
    
  }
  
  else if ( (from + 6 - to) % 6 == 4 ) {
    // we are turning left
  }

  return makeColorHSB(0, 0, brightness);
}

