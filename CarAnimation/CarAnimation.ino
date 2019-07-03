/*
   Fade out the car based on a trail length or timing fade away
*/

bool isCarPassed[6];
uint32_t timeCarPassed[6];
byte carBrightnessOnFace[6];

byte roadIn = 0;
byte roadOut = 3;

byte progress;
Timer carTimer;

#define FADE_DURATION    2500

#define MIN_TRANSIT_TIME  800
#define MAX_TRANSIT_TIME 1200

word transitTime = MAX_TRANSIT_TIME;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  if (buttonSingleClicked()) {
    // animate one of 3 ways
    roadIn = 0;
    roadOut = 2 + random(2);
    carTimer.set(transitTime);
    resetIsCarPassed();
  }
  else if (buttonDoubleClicked()) {
    // change speed of car
    transitTime -= 100;
    if(transitTime < MIN_TRANSIT_TIME) {
      transitTime = MAX_TRANSIT_TIME;
    }
  }

  FOREACH_FACE(f) {
    // did the car just pass us
    if(!isCarPassed[f]) {
      byte progress = 100 - map(carTimer.getRemaining(),0,1200,0,100);  
      if(didCarPassFace(f, progress, roadIn, roadOut)) {
        timeCarPassed[f] = millis();
        isCarPassed[f] = true;
      }
    }
  }
  

  FOREACH_FACE(f) {
    if (millis() - timeCarPassed[f] > FADE_DURATION) {
      carBrightnessOnFace[f] = 0;
    }
    else {
      carBrightnessOnFace[f] = 255 - map(millis() - timeCarPassed[f], 0, FADE_DURATION, 0, 255);
    }

    setColorOnFace(dim(WHITE, carBrightnessOnFace[f]), f);
  }
}

void resetIsCarPassed() {
  FOREACH_FACE(f) {
    isCarPassed[f] = false;
  }
}

bool didCarPassFace(byte face, byte pos, byte from, byte to) {

  // are we going straight, turning left, or turning right
  byte center;
  byte faceRotated = (6 + face - from) % 6;

  if ( (from + 6 - to) % 6 == 3 ) {
    switch ( faceRotated ) { //... rotate to the correct direction
      case 0: center = 0;  break;
      case 1: center = 33; break;
      case 2: center = 67; break;
      case 3: center = 99;  break;
      case 4: center = 67; break;
      case 5: center = 33; break;
    }
  }
  else if ( (from + 6 - to) % 6 == 2 ) {
    // we are turning right
    switch ( faceRotated ) { //... rotate to the correct direction
      case 0: center = 0;  break;
      case 1: center = 25; break;
      case 2: center = 50;  break;
      case 3: center = 75; break;
      case 4: center = 99;  break;
      case 5: center = 25;  break;
    }
  }

  else if ( (from + 6 - to) % 6 == 4 ) {
    // we are turning left
    switch ( faceRotated ) { //... rotate to the correct direction
      case 0: center = 0;  break;
      case 1: center = 25;  break;
      case 2: center = 99;  break;
      case 3: center = 75; break;
      case 4: center = 50;  break;
      case 5: center = 25; break;
    }
  }

  // if our car position is past our center,
  // great, we have had the car pass us
  return pos > center;
}
