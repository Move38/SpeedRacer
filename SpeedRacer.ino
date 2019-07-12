/*
    Speed Racer
    by Move38, Inc. 2019
    Lead development by Dan King
    original game by Dan King, Jonathan Bobrow

    Rules: https://github.com/Move38/SpeedRacer/blob/master/README.md

    --------------------
    Blinks by Move38
    Brought to life via Kickstarter 2018

    @madewithblinks
    www.move38.com
    --------------------
*/

enum faceRoadStates {LOOSE, ROAD, SIDEWALK, CRASH};
byte faceRoadInfo[6];

enum handshakeStates {NOCAR, HAVECAR, READY, CARSENT};
byte handshakeState[6];
Timer datagramTimeout;
#define DATAGRAM_TIMEOUT_LIMIT 150

bool isLoose = true;

bool hasDirection = false;
byte entranceFace = 0;
byte exitFace = 0;

bool haveCar = false;
word carProgress = 0;//from 0-100 is the regular progress

bool isCarPassed[6];
uint32_t timeCarPassed[6];
byte carBrightnessOnFace[6];

#define FADE_DURATION    2500
#define CRASH_DURATION   600

byte currentSpeed = 1;
#define SPEED_INCREMENTS 35
word currentTransitTime;
#define MIN_TRANSIT_TIME 800
#define MAX_TRANSIT_TIME 1200
Timer transitTimer;

word carTime[6];
byte carBri[6];

bool crashHere = false;
uint32_t timeOfCrash = 0;
Timer crashTimer;
#define CRASH_TIME 2500

Timer entranceBlinkTimer;
#define CAR_FADE_IN_DIST   200   // kind of like headlights

void setup() {
  randomize();
}

void loop() {
  //run loops
  if (isLoose) {
    looseLoop();
  } else if (crashHere) {
    crashLoop();
  } else if (haveCar) {
    roadLoopCar();
  } else {
    roadLoopNoCar();
  }

  //run graphics
  basicGraphics();

  //update communication
  FOREACH_FACE(f) {
    byte sendData = (faceRoadInfo[f] << 4) + (handshakeState[f] << 2);
    setValueSentOnFace(sendData, f);
  }

  //clear button presses
  buttonSingleClicked();
}

void looseLoop() {
  if (!isAlone()) {
    //so I look at all faces, see what my options are
    bool foundRoadNeighbor = false;
    byte currentChoice = random(5);
    FOREACH_FACE(f) {
      //should I still be looking?
      if (!foundRoadNeighbor) {//only look if I haven't found a road neighbor
        if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
          byte neighborData = getLastValueReceivedOnFace(f);
          if (getRoadState(neighborData) == ROAD) {//so this neighbor is a road. Sweet!
            foundRoadNeighbor = true;
            currentChoice = f;
          } else if (getRoadState(neighborData) == LOOSE) {
            currentChoice = f;
          }
        }
      }
      faceRoadInfo[f] = SIDEWALK;
    }

    //we can be confident that if there's a road neighbor, it's been chosen
    //failing that, a loose neighbor has been chosen
    //failing that, it's just random
    faceRoadInfo[currentChoice] = ROAD;
    completeRoad(currentChoice);
    isLoose = false;
  }
}

void completeRoad(byte startFace) {
  //so we've been fed a starting point
  //we need to assign an exit point based on some rules
  bool foundRoadExit = false;
  byte currentChoice = (startFace + 2 + random(1) + random(1)) % 6;//assigns a straightaway 50% of the time

  //now run through the legal exits and check for preferred exits
  FOREACH_FACE(f) {
    if (isValidExit(startFace, f)) {
      if (!foundRoadExit) {
        if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
          byte neighborData = getLastValueReceivedOnFace(f);
          if (getRoadState(neighborData) == ROAD) {
            foundRoadExit = true;
            currentChoice = f;
          } else if (getRoadState(neighborData) == LOOSE) {
            currentChoice = f;
          }
        }
      }
    }
  }//end face loop

  //so after this process, we can be confident that a ROAD has been chosen
  //or failing that, a LOOSE has been chosen
  //or failing that just a random face has been chosen
  faceRoadInfo[currentChoice] = ROAD;
}

bool isValidExit(byte startFace, byte exitFace) {
  if (exitFace == (startFace + 2) % 6) {
    return true;
  } else if (exitFace == (startFace + 3) % 6) {
    return true;
  } else if (exitFace == (startFace + 4) % 6) {
    return true;
  } else {
    return false;
  }
}

void roadLoopNoCar() {

  FOREACH_FACE(f) {
    if (faceRoadInfo[f] == ROAD) {//things can arrive here
      if (handshakeState[f] == NOCAR) {//I have no car here. Does my neighbor have a car?
        if (!isValueReceivedOnFaceExpired(f)) {//there's someone on this face
          byte neighborData = getLastValueReceivedOnFace(f);
          if (getHandshakeState(neighborData) == HAVECAR) {
            handshakeState[f] = READY;
          }
        }
      } else if (handshakeState[f] == READY) {//I am ready. Look for changes
        //did my neighbor disappear, change to CRASH or NOCAR?
        if (isValueReceivedOnFaceExpired(f)) {//my neighbor has left. Guess that's not happening
          handshakeState[f] = NOCAR;
        } else {//neighbor still there. have they changed in another way?
          byte neighborData = getLastValueReceivedOnFace(f);
          if (getRoadState(neighborData) != ROAD) { //huh, I guess they changed in a weird way
            handshakeState[f] = NOCAR;
          } else {//they're still a road. did their handshakeState change?
            if (getHandshakeState(neighborData) == NOCAR || getHandshakeState(neighborData) == READY) {//another weird failure
              handshakeState[f] = NOCAR;
            } else if (getHandshakeState(neighborData) == CARSENT) {
              //look for the speedDatagram
              if (isDatagramReadyOnFace(f)) {//is there a packet?
                if (getDatagramLengthOnFace(f) == 1) {//is it the right length?
                  byte *data = (byte *) getDatagramOnFace(entranceFace);//grab the data
                  currentSpeed = data[0];

                  //THEY HAVE SENT THE CAR. BECOME THE ACTIVE GUY
                  FOREACH_FACE(ff) {
                    handshakeState[ff] = NOCAR;
                  }
                  haveCar = true;
                  currentTransitTime = map(SPEED_INCREMENTS - currentSpeed, 0, SPEED_INCREMENTS, MIN_TRANSIT_TIME, MAX_TRANSIT_TIME);
                  transitTimer.set(currentTransitTime);

                  hasDirection = true;
                  entranceFace = f;
                  exitFace = findOtherSide(entranceFace);
                  handshakeState[entranceFace] = HAVECAR;
                  handshakeState[exitFace] = HAVECAR;
                }
              }
            }
          }
        }
      }
    }
  }


  //if you become alone, GO LOOSE
  if (isAlone()) {
    goLoose();
  }

  //if I'm clicked, I will attempt to spawn the car (only happens if there is a legitimate exit choice)
  if (buttonSingleClicked()) {
    spawnCar();
  }
}

void spawnCar() {
  FOREACH_FACE(f) {
    if (!hasDirection) {
      if (faceRoadInfo[f] == ROAD) {//this could be my exit
        if (!isValueReceivedOnFaceExpired(f)) {//there is someone there
          byte neighborData = getLastValueReceivedOnFace(f);
          if (getRoadState(neighborData) == ROAD) {//so this is a road I can send to. DO IT ALL
            //set direction
            hasDirection = true;
            exitFace = f;
            entranceFace = findOtherSide(exitFace);

            //set outgoing data
            FOREACH_FACE(ff) {
              handshakeState[ff] = NOCAR;
            }
            handshakeState[exitFace] = HAVECAR;

            // launch car
            haveCar = true;
            currentTransitTime = map(SPEED_INCREMENTS - currentSpeed, 0, SPEED_INCREMENTS, MIN_TRANSIT_TIME, MAX_TRANSIT_TIME);
            transitTimer.set(currentTransitTime);
          }
        }
      }
    }
  }
}

void goLoose() {
  isLoose = true;

  FOREACH_FACE(f) {
    faceRoadInfo[f] = LOOSE;
    isCarPassed[f] = false;
    timeCarPassed[f] = 0;
    carBrightnessOnFace[f] = 0;
    carTime[f] = 0;
    carBri[f] = 0;
  }

  loseCar();
}

void loseCar() {
  hasDirection = false;
  haveCar = false;
  carProgress = 0;//from 0-100 is the regular progress
  currentSpeed = 1;
  crashHere = false;
  timeOfCrash = 0;
  FOREACH_FACE(f) {
    handshakeState[f] = NOCAR;
  }
}

void resumeRoad() {
  FOREACH_FACE(f) {
    faceRoadInfo[f] = SIDEWALK;
  }
  faceRoadInfo[entranceFace] = ROAD;
  faceRoadInfo[exitFace] = ROAD;
}

byte findOtherSide(byte entrance) {
  FOREACH_FACE(f) {
    if (isValidExit(entrance, f)) {
      if (faceRoadInfo[f] == ROAD) {
        return f;
      }
    }
  }
}

void roadLoopCar() {

  if (handshakeState[exitFace] == HAVECAR) {
    //wait for the timer to expire and pass the car
    if (transitTimer.isExpired()) {
      //ok, so here is where shit gets tricky
      if (!isValueReceivedOnFaceExpired(exitFace)) {//there is someone on my exitFace
        byte neighborData = getLastValueReceivedOnFace(exitFace);
        if (getRoadState(neighborData) == ROAD) {
          if (getHandshakeState(neighborData) == READY) {
            handshakeState[exitFace] = CARSENT;

            byte speedDatagram[1];
            if ((entranceFace == (exitFace + 3) % 6) && currentSpeed + 2  <= SPEED_INCREMENTS) { //STRAIGHTAWAY
              speedDatagram[0] = currentSpeed + 2;
            } else if (currentSpeed + 1 <= SPEED_INCREMENTS) {
              speedDatagram[0] = currentSpeed + 1;
            } else {
              speedDatagram[0] = currentSpeed;
            }
            sendDatagramOnFace(&speedDatagram, sizeof(speedDatagram), exitFace);

            datagramTimeout.set(DATAGRAM_TIMEOUT_LIMIT);

          } else {
            //CRASH because not ready
            crashBlink();
          }
        } else {
          //CRASH crash because not road
          crashBlink();
        }
      } else {
        //CRASH because not there!
        crashBlink();
      }
    }
  } else if (handshakeState[exitFace] == CARSENT) {
    if (!isValueReceivedOnFaceExpired(exitFace)) {//there's someone on my exit face
      if (getHandshakeState(getLastValueReceivedOnFace(exitFace)) == HAVECAR) {//the car has been successfully passed
        handshakeState[exitFace] = NOCAR;
        loseCar();
      }
    }
    //if I'm still in CARSENT and my datagram timeout has expired, then we can assume the car is lost and we've crashed
    if (handshakeState[exitFace] == CARSENT) {
      if (datagramTimeout.isExpired()) {
        //CRASH because timeout
        crashBlink();
      }
    }
  }
}

void crashBlink() {
  isLoose = false;
  timeOfCrash = millis();
  crashHere = true;
  FOREACH_FACE(f) {
    faceRoadInfo[f] = CRASH;
  }
  crashTimer.set(CRASH_TIME);
}

void crashLoop() {
  if (crashTimer.isExpired()) {
    loseCar();
    resumeRoad();
  }
}

byte getRoadState(byte neighborData) {
  return (neighborData >> 4);//1st and 2nd bits
}

byte getHandshakeState(byte neighborData) {
  return ((neighborData >> 2) & 3);//3rd and 4th bits
}

void basicGraphics() {
  if (isLoose) {
    setColor(dim(CYAN, (millis() / 10) % 256));
  } else if (haveCar) {
    FOREACH_FACE(f) {
      if (faceRoadInfo[f] == ROAD) {
        if (f == entranceFace) {
          setColorOnFace(GREEN, f);
        } else if (f == exitFace) {
          setColorOnFace(ORANGE, f);
        }
      } else if (faceRoadInfo[f] == SIDEWALK) {
        setColorOnFace(BLUE, f);
      } else if (faceRoadInfo[f] == CRASH) {
        setColorOnFace(RED, f);
      }
    }
  } else {
    FOREACH_FACE(f) {
      if (faceRoadInfo[f] == ROAD) {
        setColorOnFace(YELLOW, f);
      } else if (faceRoadInfo[f] == SIDEWALK) {
        setColorOnFace(OFF, f);
      } else if (faceRoadInfo[f] == CRASH) {
        setColorOnFace(RED, f);
      }
    }
  }
}
