/*
  Tin Can Robot
 */
#include <Servo.h> 
#include "pitches.h"

// Declare the functions here so the compiler knows the parameter types
void makeAMove( int turn );
void changeEyes();
void playAnotherNote();
void playMusicAsync( const struct s_oneNote noteList[] );
void playMusic( const struct s_oneNote noteList[] );
void showTurn( int turn );
void showLight( int value1, int value2 );

// Name our servos
Servo LeftWheel;
Servo RightWheel;

// Motion variables
const int robotRight    = 1;
const int robotLeft     = 2;
const int robotForward  = 3;
const int robotBackward = 4;
const int robotStop     = 5;
const int debugMotion   = 0;   // Set to 1 to enable logging turns
int lastTurn            = 0;   // Remember the last turn direction

// Light sensors
const int light1        = 6;   // Use analog pin 0 for a CDS photo cell
const int light2        = 7;   // Use analog pin 1 for a CDS photo cell
const int thresholdTurn = 200; // Difference between left/right eyes to start turning
const int debugLight    = 0;   // Set to 1 to enable logging light readings
int thresholdMove       = 400; // Adjust to change light sensitivity for motion

// Use pin 8 for LeftWheel and ping 9 for RightWheel
const int leftWheel     = 8;
const int rightWheel    = 9;

// Use pins 2..5 to control two LED's named led1 and led2
const int led1C         = 2;
const int led1A         = 3;
const int led2C         = 4;
const int led2A         = 5;
int ledOn               = 0;   // Remember the state of the LEDs
// Basic timing for the eye blink - 1000ms or 1 second
const int eyeBlinkDuration = 1000;
int eyeBlinkCounter     = eyeBlinkDuration;

// Speaker on pin 10
const int speakerA      = 10;

// Loop Timing
const int dispatchWait  = 10;  // Basic time slice of 10ms

// Dynamic values used during the playing of music
//
// Defines a note as having a tone and duration
struct s_oneNote
  {
    int toneValue;
    int toneDuration;
  };
// Pointer to the current piece of music being played
const struct s_oneNote *noteMusic;

const int playNextNote  = 1;
const int playStop      = 2;
const int debugMusic    = 0;   // Set to 1 to enable music logging

int noteState           = 2;   // 1 = play next note, 2 = don't play
int noteIndex           = 0;   // index of the next note to play
int noteLength          = 0;   // number of notes in the tune
int noteTempo           = 1;   // speed to play adjustment (bigger == faster) [1..4?]
// Define a quarter note to be 250ms
const int qN            = 250; // 250ms
int noteCounter         = qN; // Basic not duration in milliseconds
// Music pieces
//
// Imperial March (Star Wars)
static struct s_oneNote imperialMarch[] = { { 9, 1 }, 
	{ NOTE_G4, 3*qN}, { NOTE_G4, 3*qN }, { NOTE_G4, 3*qN }, { NOTE_DS4, 2*qN }, { NOTE_AS4, qN }, { NOTE_G4, 3*qN },
        { NOTE_DS4, 2*qN }, { NOTE_AS4, qN }, { NOTE_G4, 6*qN }};
//
// Daisy Bell (HAL 9000)
static struct s_oneNote daisyBell[] = { { 10, 3 },
    { NOTE_D6, 9*qN }, { NOTE_B5, 9*qN }, { NOTE_G5, 9*qN }, { NOTE_D5, 9*qN }, 
    { NOTE_E5, 3*qN }, { NOTE_FS5, 3*qN }, { NOTE_G5, 3*qN }, { NOTE_E5, 6*qN }, { NOTE_G5, 3*qN }, { NOTE_D5, 15*qN } };
//
// --------------------------------------------------
// The setup() function runs once on power up or when
// when you press the reset button.
// --------------------------------------------------
void setup() {
  // Basic timeouts for the eye blink and note player
  eyeBlinkCounter = eyeBlinkDuration;
  noteCounter     = qN;
  
  // Initialize the servos
  LeftWheel.attach(leftWheel);
  RightWheel.attach(rightWheel);
  
  // Initialize the digital pins for the LEDs as outputs.
  // Set the cathode side of each LED to ground.  Setting the
  // anode side high will turn the LED on and low will turn it off.
  pinMode(led1A, OUTPUT);
  pinMode(led1C, OUTPUT);
  pinMode(led2A, OUTPUT);
  pinMode(led2C, OUTPUT);  
  digitalWrite(led1C, LOW);  // Set cathode to ground
  digitalWrite(led2C, LOW);
  // Initialize the digital pin for the speaker.
  pinMode(speakerA, OUTPUT);
  
  // Take a light reading to establish a baseline.
  thresholdMove = analogRead(light1)-150;
  
  // Initialize serial communication:
  Serial.begin(9600);
  // Setup is done, the 'loop()' function will now take over an run
  // everything.
}
// --------------------------------------------------
// The loop routine runs over and over again forever:
// --------------------------------------------------
void loop() {
  int turn = 0;
  // See if we are being asked to turn.
  turn = QueryMotion();
  // If so then make our move if it's different than what we are
  // already doing.
  if (turn != lastTurn) makeAMove( turn );
  // when < 0 we need to blink the LED's
  eyeBlinkCounter -= dispatchWait;
  if (eyeBlinkCounter<=0) changeEyes();
  // when < 0 we need to play the next note
  noteCounter -= dispatchWait;
  if (noteCounter<=0) playAnotherNote();
  // Delay 'dispatchWait' milliseconds and run the loop again.
  delay(dispatchWait);
}

// Returns whether we should turn and in what direction
// 0 = no turn, 1 = turn right,  2 = turn left, 3 = go forward, 4 = go backward, 5 = stop
// If the difference between the sensors is more than 'thresholdTurn' then turn 
//   left or right
// If the sensors see more that 'thresholdMove' then move forward
// Currently no move back - maybe add a sensor on the back?
int QueryMotion()
{
  int diffVal;
  int turn;
  int light1Val = analogRead(light1);
  int light2Val = analogRead(light2);
  diffVal = (light1Val - light2Val);
  if (abs(diffVal)>thresholdTurn) {
    if (light1Val > light2Val) { turn = 1; }
    else { turn = 2; }
  } else {
    if (light1Val < (thresholdMove-1) ) { turn = 3; }
    else { turn = 0; }
  }
  return (turn);
}

void makeAMove( int turn )
{
  lastTurn = turn;
  if ( debugMotion ) showTurn(turn);
  if (turn == robotRight) {
    LeftWheel.write(179);
    RightWheel.write(0);
    playMusicAsync(imperialMarch);
  }
  else if (turn == robotLeft) {
    LeftWheel.write(0);
    RightWheel.write(179);
    playMusicAsync(daisyBell);
  }
  else if (turn == robotForward) {
    LeftWheel.write(179);
    RightWheel.write(179);
  }
  else { // robotStop
    LeftWheel.writeMicroseconds(1500);
    RightWheel.writeMicroseconds(1500);
  }
  return;
}

void changeEyes()
{
  eyeBlinkCounter = eyeBlinkDuration;
  // Flash the leds
  ledOn = !ledOn;
  if (ledOn) {
    digitalWrite(led1A, HIGH);  // turn the LED on (HIGH is the voltage level)
    digitalWrite(led2A, LOW);
  } else {
    digitalWrite(led1A, LOW);   // turn the LED off by making the voltage LOW
    digitalWrite(led2A, HIGH);
  }
  return;
}

// Get the next node, look up the tone value and start playing the node
// If no more notes set the play state to playStop
void playAnotherNote()
{
  noteCounter=250;
  if (noteState = playNextNote)
  {
    if (noteIndex++<noteLength)
    {
      int ajustedDuration=noteMusic[noteIndex].toneDuration/noteTempo;
      tone(speakerA, noteMusic[noteIndex].toneValue, ajustedDuration);
      noteCounter=250+ajustedDuration;
    } else {
      noteState = playStop;
    }
  }
}

void playMusicAsync( const struct s_oneNote noteMusicRequest[] )
{
  noteMusic = noteMusicRequest;
  noteIndex = 0;
  noteState = 1;
  noteCounter = 250;
  noteLength = noteMusic[0].toneValue;
  noteTempo = noteMusic[0].toneDuration;
  if ( debugMusic ) Serial.println("Music on");
  return;
} 

void playMusic( const struct s_oneNote noteMusicRequest[] )
{
  int tempo=noteMusicRequest[0].toneDuration;
  for (int i=1; i<(noteMusicRequest[0].toneValue+1); i++)
  {
    int t=noteMusicRequest[i].toneDuration/tempo;
    tone(speakerA, noteMusicRequest[i].toneValue, t);
    delay(250+t);
  }
  return;
}
void showTurn( int turn )
{
  Serial.print("Turn: ");
  switch (turn)
  {
    case 1:  Serial.println("Right");
             break;
    case 2:  Serial.println("Left");
             break;
    case 3:  Serial.println("Forward");
             break;
    default: Serial.println("Stop");
             break;
  }
  return;
}

void showLight( int value1, int value2 )
{
  Serial.print("Light: ");
  Serial.print(value1);
  Serial.print(" , ");
  Serial.println(value2);
  return;
}
