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

#define ROAD_INDICATOR_ON_DURATION_MS   50   // flashes the road indicator
#define ROAD_INDICATOR_OFF_DURATION_MS  500   // flashes the road indicator

Timer updateCarTimer;
Timer roadIndicatorTimer;

bool indicatorOn = false;

float carPosition = -CAR_FADE_IN_DIST;
float carSpeed = 0.03;   // travels this far every update delay ms
float carAccel = 0.005;  // acceleration each Blink traveled

float carFadeOutDistance = 40 * carSpeed; // the tail should have a relationship with the speed being travelled
float carFadeInDistance = 0.2;

byte rotate = 0;

byte face_in;
byte face_out;

bool hasCar = false;
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
  face_in = 0;
  face_out = getFaceOutBasedOnFaceIn( face_in );
}

void loop() {
  // put your main code here, to run repeatedly:

  if( buttonSingleClicked() ) {
    // spawn car
    hasCar = true;
    carPosition = -CAR_FADE_IN_DIST;
  }

  if( buttonDoubleClicked() ) {
    face_in++;
    if(face_in >5) {
      face_in = 0;
    }
    face_out = getFaceOutBasedOnFaceIn( face_in );
  }

  // update the car's position
  if( updateCarTimer.isExpired() ) {
    updateCarTimer.set( CAR_UPDATE_DELAY_MS );
    carPosition += carSpeed;

    if(carPosition == 1.0) {
      // the car is at the edge, should pass to the next Blink
      if( !isValueReceivedOnFaceExpired( face_out ) ) {
        // Blink is available to receive
        // send the Blink a pass message
        setValueSentOnFace( 2, face_out );    
      }
      else {
        // no Blink to receive, then we crash and explode 
      }
    }
    else if(carPosition >= 1.0 + CAR_FADE_OUT_DIST) {
      // car is now completely passed us
      
      // for testing purposes, just loop back to where we started
//      if(carPosition >= 1.0 + 2 * CAR_FADE_OUT_DIST) {
//        carPosition = -CAR_FADE_IN_DIST;
//      }
    }
  }

  // if we were alone 
  // and attached to the correct attachment point
  // choose a direction to go
  // if we attached to the incorrect attachment point
  // display an alert... i.e. flashing immediacy with yellow or orange

  // display the car
  FOREACH_FACE( f ) {
    setFaceColor( f, getFaceColorBasedOnCarPossition( f, carPosition, face_in, face_out) );
  }

  // if there is a car on the track, signal that there is a car on the track by sharing its score
  //
  
  // if we don't have a Blink on our face_out, show the face_out indicator...
  if ( roadIndicatorTimer.isExpired() ) {
    indicatorOn = !indicatorOn;
    if( indicatorOn ) {
      roadIndicatorTimer.set( ROAD_INDICATOR_ON_DURATION_MS );
    }
    else {
      roadIndicatorTimer.set( ROAD_INDICATOR_OFF_DURATION_MS );      
    }
  }

  if( indicatorOn ) {    
    setFaceColor( face_in, GREEN);
    setFaceColor( face_out, RED );
  }

  if( isAlone() ){
   // make it clear that we are a piece in transit... i.e. road building, maybe rotate yellow patern of 3 lights 
  }

}

/*
 * 
 */

byte getFaceOutBasedOnFaceIn( byte faceIn ) {
  byte faceOut;

  faceOut = ( faceIn + 2 + rand(2) ) % 6;
  
  return faceOut;
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
    byte faceRotated = (6 + face - from) % 6; 
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
  
  else if ( (from + 6 - to) % 6 == 2 ) {
    // we are turning right
    float center;
    byte faceRotated = (6 + face - from) % 6; 
    switch( faceRotated ) { //... rotate to the correct direction
      case 0: center = 0.0;  break;
      case 1: center = 0.25; break;
      case 2: center = 0.5;  break;
      case 3: center = 0.75; break;
      case 4: center = 1.0;  break;
      case 5: center = 0.5;  break;
    }

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
  
  else if ( (from + 6 - to) % 6 == 4 ) {
    // we are turning left
    float center;
    byte faceRotated = (6 + face - from) % 6; 
    switch( faceRotated ) { //... rotate to the correct direction
      case 0: center = 0.0;  break;
      case 1: center = 0.5;  break;
      case 2: center = 1.0;  break;
      case 3: center = 0.75; break;
      case 4: center = 0.5;  break;
      case 5: center = 0.25; break;
    }

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

  return makeColorHSB(0, 0, brightness);
}

