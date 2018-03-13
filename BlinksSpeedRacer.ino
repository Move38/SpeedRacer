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
#define CAR_FADE_OUT_DIST  0.50   // kind of like a taillight trail

// TODO: make fade in/out dist variable based on speed...

#define CAR_UPDATE_DELAY_MS  30   // updates the car position every n milliseconds

Timer updateCarTimer;

float carPosition = -CAR_FADE_IN_DIST;
float carSpeed = 0.05;  // travels this far every update delay ms
 
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

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
      carPosition = -CAR_FADE_IN_DIST;
    }
  }

}

/*
 * fade from the first side to the opposite side
 * front of the fade should be faster than the fall off
 * 
 */
Color getFaceColorBasedOnCarPossition(byte face, float pos, byte from, byte to) {
   
  // are we going straight, turning left, or turning right
  if ( from == 0 && to == 3 ||    // if ( (from + 6 - to) % 6 == 3 )
       from == 1 && to == 4 ||
       from == 2 && to == 5 ||
       from == 3 && to == 0 ||
       from == 4 && to == 1 ||
       from == 5 && to == 2 ) {

        // we are traveling straight
  }
  else if ( from == 0 && to == 3) { 
  }
  else if ( ) {
    
  }
}

