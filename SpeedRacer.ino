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

enum handshakeStates {INERT, SENDING, READY};
byte handshakeState = INERT;

bool haveCar = false;
bool packetSent;
byte carProgress = 0;//from 0-100 is the regular progress
byte currentSpeed = 1;
int currentTransitTime;
#define MIN_TRANSIT_TIME 750
#define MAX_TRANSIT_TIME 1500
Timer transitTimer;

//CRASH DATA
bool crashHere = false;

void setup() {

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
        byte sendData = (PLAY << 4) + (faceRoadInfo[f] << 2);
        setValueSentOnFace(sendData, f);
      }
      break;
    case CRASH:
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
      } else if (getGameState(getLastValueReceivedOnFace(f)) == PLAY) {//transition to PLAY
        gameState = PLAY;
      }
    }
  }

  //listen for double click
  if (buttonDoubleClicked()) {
    gameState = PATHFIND;
    isPathfinding = true;
    isOrigin = true;
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
  }

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
      playState = ENDPOINT;
    }
  }

  FOREACH_FACE(f) { //just straight up listen for the PLAY signal
    if (!isValueReceivedOnFaceExpired(f)) { //something here
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getGameState(neighborData) == PLAY) {
        gameState = PLAY;
      }
    }
  }
}

void gameLoop() {
  //so here's the new architecture
  //we set a bunch of bools to do a certain set of functions
  bool doCarProgress = false;
  bool doPacketSend = false;
  bool 

  if (haveCar) {
    doCarProgress = true;
  } else {

  }

  if (playState == LOOSE) {

  } else if (playState == ENDPOINT) {

  } else if (playState == THROUGH) {

  }
  /////////////OOOOOLD
  //first we check if we're alone
  if (haveCar) {//once you have a car, the only thing you do is tick car progress and try to establish a connection
    if (!packetSent) {//you have an exit, but haven't made a connection. Just check that face for a READY signal
      if (!isValueReceivedOnFaceExpired(exitFace)) { //there is a neighbor here
        byte neighborData = getLastValueReceivedOnFace(exitFace);
        if (getHandshake(neighborData) == READY) {//ooh, this neighbor wants my shit!
          //throw a packet with speed at them
          ////DO IT
          packetSent = true;
        }
      }

    } else {//the packet has been sent, and you have an exit
      if (isValueReceivedOnFaceExpired(exitFace)) { //woah, my neighbor is gone
        packetSent = false;//so we can resend it once a connection is re-established
      }
    }

    //regardless of exit status, we must tick up the car progress and check for handshakes
    carProgress = (transitTimer.getRemaining() * 100) / (currentTransitTime); //counts from 100-0
    //time for a pass!
    if (packetSent) {//we have a functional partner to pass to
      handshakeState = INERT;
      haveCar = false;
    } else {//we don't have an exit to pass to, we crash
      gameState = CRASH;
      crashHere = true;
    }
    ////END OF HAVE CAR
  } else {//we do not have the car
    if (isAlone()) {//we are alone. Make sure we're set to all defaults
      playState = LOOSE;
      FOREACH_FACE(ff) {
        faceRoadInfo[ff] = FREEAGENT;
      }
      entranceFace = 6;
      exitFace = 6;
      haveCar = false;
      packetSent = 0;
      handshakeState = INERT;
      carProgress = 0;
    } else {// we are connected to someone
      if (playState == LOOSE) { //we're still looking to become part of the road
        FOREACH_FACE(f) {//run through the faces, look for someone screaming EXIT at us
          if (!isValueReceivedOnFaceExpired(f)) { //found someone
            byte neighborData = getLastValueReceivedOnFace(f);
            if (getRoadState(neighborData) == EXIT) {//nice, we found an exit
              entranceFace = f;
              playState = ENDPOINT;
              exitFace = entranceFace + (2 + random(2));
              faceRoadInfo[exitFace] = EXIT;
            }
          }
        }
      } else {//we are either and ENDPOINT or a THROUGH

      }

    }

  }//end of do not have car
}

void crashLoop() {

}

byte getGameState(byte neighborData) {
  return (neighborData >> 4);//1st and 2nd bits
}

byte getRoadState(byte neighborData) {
  return ((neighborData >> 2) & 3);//3rd and 4th bits
}

byte getHandshake(byte neighborData) {
  return (neighborData & 3);//5th and 6th bits
}

void setupGraphics () {
  setColor(BLUE);
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
        setColorOnFace(BLUE, f);
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
        setColorOnFace(BLUE, f);
        break;
      case ENTRANCE:
        setColorOnFace(YELLOW, f);
        break;
      case EXIT:
        setColorOnFace(YELLOW, f);
        break;
      case SIDEWALK:
        setColorOnFace(OFF, f);
        break;
    }
  }
}

