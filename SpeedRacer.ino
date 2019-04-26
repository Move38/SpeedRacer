#include "Serial.h"
ServicePortSerial serial;

enum gameStates {SETUP, PATHFIND, PLAY, CRASH};
byte gameState = SETUP;

//SETUP DATA
bool connectedFaces[6];
bool isOrigin = false;

//PATHFIND DATA
bool isPathfinding = false;
bool pathFound = false;

enum faceRoadStates {FREEAGENT, ENTRANCE, EXIT, SIDEWALK};
byte faceRoadInfo[6];
byte entranceFace = 6;//these are an impossible number by default
byte exitFace = 6;//these are an impossible number by default

//PLAY DATA
enum playStates {LOOSE, THROUGH, ENDPOINT};
byte playState = LOOSE;

enum handshakeStates {NOCAR, HAVECAR, READY, CARSENT};
byte handshakeState = NOCAR;

bool haveCar = false;
bool carPassed = false;
byte carProgress = 0;//from 0-100 is the regular progress
byte currentSpeed = 1;
#define SPEED_INCREMENTS 100
int currentTransitTime;
#define MIN_TRANSIT_TIME 750
#define MAX_TRANSIT_TIME 1500
Timer transitTimer;

//CRASH DATA
bool crashHere = false;

void setup() {
  gameState = SETUP;
  serial.println("Starting in SETUP");
}

void loop() {

  //run loops
  switch (gameState) {
    case SETUP:
      setupLoop();
      break;
    case PATHFIND:
      pathfindLoop();
      break;
    case PLAY:
      gameLoop();
      break;
    case CRASH:
      crashLoop();
      break;
  }

  //run graphics
  switch (gameState) {
    case SETUP:
      setupGraphics();
      break;
    case PATHFIND:
      pathfindGraphics();
      break;
    case PLAY:
      playGraphics();
      break;
    case CRASH:
      crashGraphics();
      break;
  }

  //update communication
  switch (gameState) {
    case SETUP://this one is simple
      setValueSentOnAllFaces(SETUP << 4);
      break;
    case PATHFIND:
      FOREACH_FACE(f) {
        byte sendData = (PATHFIND << 4) + (faceRoadInfo[f] << 2);
        setValueSentOnFace(sendData, f);
      }
      break;
    case PLAY:
      FOREACH_FACE(f) {
        byte sendData = (PLAY << 4) + (faceRoadInfo[f] << 2) + handshakeState;
        setValueSentOnFace(sendData, f);
      }
      break;
    case CRASH:
      setValueSentOnAllFaces(CRASH << 4);
      break;
  }
}

void setupLoop() {

  //update connected faces array
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //something here
      connectedFaces[f] = true;
    } else {
      connectedFaces[f] = false;
    }
  }

  //listen for transition to PATHFIND
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //something here
      if (getGameState(getLastValueReceivedOnFace(f)) == PATHFIND) {//transition to PATHFIND
        gameState = PATHFIND;
        serial.println(F("PATHFIND"));
      } else if (getGameState(getLastValueReceivedOnFace(f)) == PLAY) {//transition to PLAY
        gameState = PLAY;
      }
    }
  }

  //listen for double click
  if (buttonDoubleClicked()) {
    serial.println(F("I'm starting the PATHFIND"));
    gameState = PATHFIND;
    isPathfinding = true;
    isOrigin = true;
    currentSpeed = 1;
    FOREACH_FACE(ff) {
      faceRoadInfo[ff] = SIDEWALK;
    }
    //set up a temporary entrance face, to be fixed later
    FOREACH_FACE(ff) {
      if (connectedFaces[ff] == true) {//there's something on this face
        entranceFace = (ff + 3) % 6;//just the opposite of an occupied face
      }
    }
  }
}

void pathfindLoop() {


  if (!isPathfinding && !pathFound) { //listen for pathfind command
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) { //something here
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getGameState(neighborData) == PATHFIND) {//a neighbor we should be listening to
          if (getRoadState(neighborData) == EXIT) {//this neighbor wants us to begin pathfinding
            isPathfinding = true;//begin pathfinding
            FOREACH_FACE(ff) {//set all my faces to sidewalk
              faceRoadInfo[ff] = SIDEWALK;
            }
            faceRoadInfo[f] = ENTRANCE;//this is our entrance
            entranceFace = f;
          }
        }
      }
    }
  }//end of listen for pathfind

  if (isPathfinding) { //do actual pathfinding
    //make an exit option array
    byte exitOptions[3] = {(entranceFace + 2) % 6, (entranceFace + 3) % 6, (entranceFace + 4) % 6};
    //shuffle that shit
    for (byte s = 0; s < 6; s++) {
      byte swapA = random(2);
      byte swapB = random(2);
      byte temp = exitOptions[swapA];
      exitOptions[swapA] = exitOptions[swapB];
      exitOptions[swapB] = temp;
    }

    //now, check each of them, and if you find a valid one, make it the exit face internally
    for (byte i = 0; i < 3; i++) {
      if (!isValueReceivedOnFaceExpired(exitOptions[i])) { //something here
        byte neighborData = getLastValueReceivedOnFace(exitOptions[i]);
        if (getGameState(neighborData) == PATHFIND) {//so this on is already in pathfind
          if (getRoadState(neighborData) == FREEAGENT) {//this neighbor is a legit entrance
            exitFace = exitOptions[i];
          }
        } else if (getGameState(neighborData) == SETUP) { //this neighbor is in some other mode, theoretically SETUP
          exitFace = exitOptions[i];
        }
      }
    }//end possible exit checks

    if (exitFace != 6) { //we actually found a legit exit
      faceRoadInfo[exitFace] = EXIT;
      //in the special case where we were the origin, we need to reorient the entrance face
      if (isOrigin) {
        entranceFace = (exitFace + 3) % 6;
        faceRoadInfo[entranceFace] = ENTRANCE;
      }
      pathFound = true;
      isPathfinding = false;
      playState = THROUGH;
    } else {//we didn't find an exit, therefore THE GAME SHALL BEGIN!
      gameState = PLAY;
      //serial.println("I'm starting the GAME");
      assignExit();
      playState = ENDPOINT;
    }
  }

  FOREACH_FACE(f) { //just straight up listen for the PLAY signal
    if (!isValueReceivedOnFaceExpired(f)) { //something here
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameState(neighborData) == PLAY) {
        gameState = PLAY;
        //serial.println("Neighbors have commanded me to GAME");
        if (isOrigin) {
          haveCar = true;
          handshakeState = HAVECAR;
          transitTimer.set(map(currentSpeed, 0, SPEED_INCREMENTS, MAX_TRANSIT_TIME, MIN_TRANSIT_TIME));
        }
      }
    }
  }
}

void gameLoop() {

  if (playState == LOOSE) {
    gameLoopLoose();
  } else {
    gameLoopRoad();
  }

  //check for crash signal regardless of state
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameState(neighborData) == CRASH) {
        crashReset();
      }
    }
  }
}

void gameLoopLoose() {
  //I need to look for neighbors that make me not alone no more
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameState(neighborData) == PLAY) {//he's playing the game
        if (getRoadState(neighborData) == EXIT) {//he wants me to become a road piece!
          //become a road piece
          playState = ENDPOINT;
          entranceFace = f;
        } else {
          //TODO: USE CONNECTED FACES ARRAY TO MAKE SOME OH NO SIGNALS
        }
      }
    }
  }
  //if I become a road piece, I need to get my info set up
  if (playState == ENDPOINT) {
    FOREACH_FACE(f) {
      faceRoadInfo[f] = SIDEWALK;
    }
    faceRoadInfo[entranceFace] = ENTRANCE;
    assignExit();
  }
}

void assignExit() {
  exitFace = entranceFace + 2 + random(2);
  faceRoadInfo[exitFace] = EXIT;
  serial.println("EXITED");
}

void gameLoopRoad() {

  if (playState == ENDPOINT) {
    //search for a FREEAGENT on your exit face
    //if you find one, send a speed packet
    if (!isValueReceivedOnFaceExpired(exitFace)) { //there is someone on my exit face
      byte neighborData = getLastValueReceivedOnFace(exitFace);
      if (getGameState(neighborData) == PLAY) {//this neighbor is able to accept a packet

        playState = THROUGH;

      }
    }
  }

  if (haveCar) {
    if (transitTimer.isExpired()) {
      //ok, so here is where shit gets tricky
      if (!isValueReceivedOnFaceExpired(exitFace)) {
        byte neighborData = getLastValueReceivedOnFace(exitFace);
        if (getRoadState(neighborData) == ENTRANCE) {
          if (getHandshakeState(neighborData) == READY) {
            handshakeState = CARSENT;
            carPassed = true;
            haveCar = false;
            //TODO: DATAGRAM
          } else {
            //CRASH because not ready
            //serial.println("CRASH here");
            gameState = CRASH;
            crashHere = true;
          }
        } else {
          //CRASH crash because not entrance
          //serial.println("CRASH here");
          gameState = CRASH;
          crashHere = true;
        }
      } else {
        //CRASH because not there!
        //serial.println("CRASH here");
        gameState = CRASH;
        crashHere = true;
      }
    }
  } else {//I don't have the car
    //under any circumstances, you should go loose if you're alone
    if (isAlone()) {
      looseReset();
    }

    if (!carPassed) {
      //check your entrance face for... things happening
      if (isValueReceivedOnFaceExpired(entranceFace)) { //oh, they're gone! Go LOOSE!
        looseReset();
      } else {//so someone is still there. Are they still a road piece?
        byte neighborData = getLastValueReceivedOnFace(entranceFace);
        if (getGameState(neighborData) == PLAY) { //this guy is in PLAY state, so I can trust that this isn't the transition period
          if (getRoadState(neighborData) == FREEAGENT) {//uh oh, it's a loose one. Best become loose as well
            looseReset();
          } else if (getRoadState(neighborData) == EXIT) {//ok, so it could send me a car. Is it?
            if (handshakeState == NOCAR) {//check and see if they are in HAVECAR
              if (getHandshakeState(neighborData) == HAVECAR) {
                handshakeState = READY;
              }
            } else if (handshakeState == READY) {
              if (getHandshakeState(neighborData) == CARSENT) {
                //THEY HAVE SENT THE CAR. BECOME THE ACTIVE GUY
                handshakeState = HAVECAR;
                haveCar = true;
                currentSpeed = 1;
                transitTimer.set(map(currentSpeed, 0, SPEED_INCREMENTS, MAX_TRANSIT_TIME, MIN_TRANSIT_TIME));
              }
            }
          }
        }
      }//end neighbor checks
    }//end carPassed check
  }//end haveCar checks

  //and no matter what, check to hear crashes elsewhere
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameState(neighborData) == CRASH) {
        gameState = CRASH;
        //serial.println("CRASH elsewhere");
      }
    }
  }
}

void looseReset() {

  playState = LOOSE;
  handshakeState = NOCAR;
  haveCar = false;
  carPassed = false;
  currentSpeed = 0;
  entranceFace = 6;//these are an impossible number by default
  exitFace = 6;//these are an impossible number by default

  FOREACH_FACE(f) {
    faceRoadInfo[f] = FREEAGENT;
  }
}

void crashReset() {
  looseReset();
  gameState = CRASH;
  isOrigin = false;
  isPathfinding = false;
  pathFound = false;
  crashHere = false;
}

void crashLoop() {
  //listen for transition to SETUP
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //something here
      if (getGameState(getLastValueReceivedOnFace(f)) == SETUP) {//transition to PATHFIND
        gameState = SETUP;
        //serial.println("Sent back to SETUP");
      } else if (getGameState(getLastValueReceivedOnFace(f)) == PATHFIND) {//transition to PLAY
        gameState = PATHFIND;
      }
    }
  }

  //listen for double click
  if (buttonDoubleClicked()) {
    gameState = SETUP;
    //serial.println("I'm sending us back to SETUP");
  }
}

byte getGameState(byte neighborData) {
  return (neighborData >> 4);//1st and 2nd bits
}

byte getRoadState(byte neighborData) {
  return ((neighborData >> 2) & 3);//3rd and 4th bits
}

byte getHandshakeState(byte neighborData) {
  return (neighborData & 3);//5th and 6th bits
}

void setupGraphics () {
  setColor(CYAN);
  FOREACH_FACE(f) {
    if (connectedFaces[f] == true) {
      setColorOnFace(YELLOW, f);
    }
  }
}

void pathfindGraphics() {
  FOREACH_FACE(f) {
    switch (faceRoadInfo[f]) {
      case FREEAGENT:
        setColorOnFace(WHITE, f);
        break;
      case ENTRANCE:
        setColorOnFace(RED, f);
        break;
      case EXIT:
        setColorOnFace(GREEN, f);
        break;
      case SIDEWALK:
        setColorOnFace(OFF, f);
        break;
    }
  }
}

void playGraphics() {
  FOREACH_FACE(f) {
    switch (faceRoadInfo[f]) {
      case FREEAGENT:
        setColorOnFace(MAGENTA, f);
        break;
      case ENTRANCE:
        setColorOnFace(YELLOW, f);
        break;
      case EXIT:
        setColorOnFace(YELLOW, f);
        break;
      case SIDEWALK:
        if (handshakeState == READY) {
          setColorOnFace(WHITE, f);
        } else {
          setColorOnFace(OFF, f);
        }
        break;
    }
  }

  if (haveCar) {
    if (entranceFace < 6 && exitFace < 6) {
      setColorOnFace(GREEN, entranceFace);
      setColorOnFace(GREEN, exitFace);
    }
  }
}

void crashGraphics() {
  if (crashHere) {
    setColor(RED);
  } else {
    setColor(ORANGE);
  }
}

