/*
 * Speed Racer
 * by Move38
 * 
 * Racing game in which you need to build the road
 * faster than the car racing down the highway can go.
 * 
 * 04.01.2019
 * 
 */

#define NO_FACE 6

byte face_in = NO_FACE;
byte face_out = NO_FACE;

byte wasAlone = 0;

enum State {
  SKY,
  ATTRACT,
  CAR,
  PASS,
  HIGHWAY,
  CRASH
};

byte state = SKY;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  // [SKY LOOP]
  // if was alone and now has neighbor
  if (wasAlone && !isAlone()) {

    // if current neighbor is in game mode
    FOREACH_FACE(f) {
      if ( !isValueReceivedOnFaceExpired( f ) ) {
        byte data = getLastValueReceivedOnFace( f );
        

      }
    }
    // if I am on the exit path
    // become highway
    // else if I am attached somewhere else
    // signal orange hazard on the sides of connection
    // if current neighbor is in attract mode
    // become highway attract
    // if current neighbor is sky
    // become highway attract
  }
  // if I have 2 or more adjacent neighbors
  // signal orange hazard on the sides of connection

  // [ATTRACT LOOP]
  // synchronize flashing...
  // maybe send signal down the line
  // if I am on the end, when pressed, create a car
  face_in = (f + 3) % 6;
  face_out = f;


  // [HIGHWAY LOOP]
  // if my neighbor sends car
  // acknowledge and receive car
  // set in face and out face
  // if car reaches the end
  // crash

  // [CAR LOOP]
  // drive from the entrance to the exit
  // speed up when you attach to a piece of road again...

  // [CRASH LOOP]
  // spread crash
  // show fiery crash source
  // eminate to the rest of the road

}

void changeState(byte state) {
  state =
}
