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
bool carPassed = false;
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
            //get speedPacket
            if (isPacketReadyOnFace(f)) {//is there a packet?
              if (getPacketLengthOnFace(f) == 1) {//is it the right length?
                byte *data = (byte *) getPacketDataOnFace(f);//grab the data
                currentSpeed = data[0];

                //go into full pathfinding mode
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
      //send the speed packet
      byte speedPacket = currentSpeed;
      if (exitFace == exitFace % 3) {//a straightaway!
        speedPacket++;
      }
      sendPacketOnFace(exitFace, (byte *) speedPacket, 1);
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
          if (isPacketReadyOnFace(f)) {//is there a packet?
            if (getPacketLengthOnFace(f) == 1) {//is it the right length?
              byte *data = (byte *) getPacketDataOnFace(f);//grab the data
              currentSpeed = data[0];//the data is currentSpeed
              //become a road piece
              playState = ENDPOINT;
              entranceFace = f;
            }
          }
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
    exitFace = entranceFace + 2 + random(2);
    faceRoadInfo[exitFace] = EXIT;
  }
}

void gameLoopRoad() {
  if (playState == ENDPOINT) {
    //search for a FREEAGENT on your exit face
    //if you find one, send a speed packet
    if (!isValueReceivedOnFaceExpired(exitFace)) { //there is someone on my exit face
      byte neighborData = getLastValueReceivedOnFace(exitFace);
      if (getGameState(neighborData) == PLAY) {//this neighbor is able to accept a packet
        byte speedPacket = currentSpeed;
        if (exitFace == exitFace % 3) {//a straightaway!
          speedPacket++;
        }
        sendPacketOnFace(exitFace, (byte *) speedPacket, 1);
        playState = THROUGH;
      }
    }
  }

  if (haveCar) {
    //do car progress
    //crash?
  } else {
    if (!carPassed) {//these checks only happen if you still need to receive the car
      //check your entrance face for... things happening
      if (!isValueReceivedOnFaceExpired(entranceFace)) { //oh, they're gone! Go LOOSE!
        looseReset();
      } else {//so someone is still there. Are they still a road piece?
        byte neighborData = getLastValueReceivedOnFace(entranceFace);
        if (getRoadState(neighborData) == FREEAGENT) {//uh oh, it's a loose one. Best become loose as well
          looseReset();
        }
      }
    }//end carPassed check

    //under any circumstances, you should go loose if you're alone
    if (isAlone()) {
      looseReset();
    }

  }//end haveCar checks


}

void looseReset() {

}

void crashReset() {

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

