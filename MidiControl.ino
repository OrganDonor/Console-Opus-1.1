#include <MIDI.h>
// MIDI Controller for Organ Donor Opus 1
// Two manuals and two ranks, initial version control panel
// Requires Arduino MEGA 2560 board and Arduino 1.5.8 (or at least not 1.0.6)
// 2014-10-07 ptw created.
// 2014-10-14 ptw updated to match as-built hardware.

// Pin assignments for the control console:
#define PIN_8GREAT      41
#define PIN_8SWELL      39
#define PIN_4SWELL      37
#define PIN_4GREAT      35
#define PIN_8SUB        33
#define PIN_8SUPER      31
#define PIN_4SUB        29
#define PIN_4SUPER      27
#define PIN_MIDI        25
#define PIN_BLOWER      23

#define PIN_8GREAT_LED  40
#define PIN_8SWELL_LED  38
#define PIN_4SWELL_LED  36
#define PIN_4GREAT_LED  34
#define PIN_8SUB_LED    32
#define PIN_8SUPER_LED  30
#define PIN_4SUB_LED    28
#define PIN_4SUPER_LED  26
#define PIN_MIDI_LED    24
#define PIN_BLOWER_LED  22

// Control console is a bunch of switches, which are read into these flags.
boolean flag8Great, flag8Swell, flag4Swell, flag4Great;      // enable each rank on each manual
boolean flag8Sub, flag8Super, flag4Sub, flag4Super;          // octave couplers on each rank
boolean flagMidi;                                            // enable remote MIDI passthru
boolean flagBlower;                                          // enable the blower (future)


// There are two or three hardware MIDI shields connected, to Serial1 and Serial2 and maybe Serial3.
// Each has all three MIDI connectors: IN, OUT, and THRU.
// The IN connectors are wired up to the two manuals (keyboards), traditionally called Great and Swell,
// and to an external source of MIDI commands (probably a computer).
// The OUT connector on Serial1 is the output to the Organ Donor windchest (two J-Omega MTPs chained).
// The OUT connector on Serial2 is available for future expansion.
// The THRU connectors are not currently used.
#define  midiGreat  midi1
#define  midiSwell  midi2
#define  midiExt    midi3
#define  midiOrgan  midi1
#define  midiAux    midi2
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midi1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, midi2);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, midi3);

// Each rank supports exactly 61 notes.
#define  RANKS            2
#define  RANK0  0      // 8' rank
#define  RANK1  1      // 4' rank

// #define  notesPerRank    61

// Ranks are 0-based (so we can use array indexing) but MIDI Channels start at 1.
#define  channelForRank(x)   (x+1)
#define  rankForChannel(x)   (x-1)

// But we limit how many are sounded simultaneously over all ranks
#define  MaxAllowedNotes  30

// Keep track of all the ways each note has been requested on,
// as a bitmap in a byte for each possible MIDI pitch.
#define  NOTE_PRIME       0x01
#define  NOTE_SUB         0x02
#define  NOTE_SUPER       0x04
#define  NOTE_RANK_PRIME  0x08
#define  NOTE_RANK_SUB    0x10
#define  NOTE_RANK_SUPER  0x20
// We'll use 128 bytes here even though only 61 are valid. Easier.
byte noteIsOn[RANKS][128];

// Count of how many notes are currently active on the organ.
byte noteCount = 0;

// Initialization or Panic: turn everything off right now.
void allOff(void)
{
  byte rank, pitch;
  
  for (pitch=0; pitch < 128; pitch++)
  {
    for (rank=0; rank < RANKS; rank++)
    {
      midiOrgan.sendNoteOff(pitch, 0, channelForRank(rank));
      noteIsOn[rank][pitch] = 0;
    }
  }
  
  noteCount = 0;
}

// startNote and stopNote are the bottom of the stack.
// They are responsible for limiting the number of simultaneous notes being sounded,
// and for sending the actual Note On and Note Off messages to MIDI.
void startNote(byte rank, byte pitch)
{
  if (noteCount >= MaxAllowedNotes)
  {  // too many notes already being played to allow this one!
    //!!! do something cute here
  }
  else
  {
    noteCount++;
    midiOrgan.sendNoteOn(pitch, 127, channelForRank(rank));
  }
}

void stopNote(byte rank, byte pitch)
{
  midiOrgan.sendNoteOff(pitch, 0, channelForRank(rank));
  if (noteCount > 0)
    noteCount--;
}

// requestOn and requestOff are the next layer up in the stack from startNote and stopNote.
// They are responsible for combining all the different possible ways a note can be requested
// (directly, through a sub-octave or super-octave coupler, or linked from the other manual).
//
// Notes already playing are not re-played. This is appropriate for the pipe organ but not for
// the general case.
void requestOn(byte flag, byte rank, byte pitch)
{
  byte oldReq, newReq;
  
  oldReq = noteIsOn[rank][pitch];
  newReq = oldReq | flag;
  
  if (oldReq == 0  && newReq != 0)
    startNote(rank, pitch);
    
  noteIsOn[rank][pitch] = newReq;
}

void requestOff(byte flag, byte rank, byte pitch)
{
  byte oldReq, newReq;

  oldReq = noteIsOn[rank][pitch];
  newReq = oldReq & ~flag;
  
  if (oldReq != 0  &&  newReq == 0)
    stopNote(rank, pitch);
    
  noteIsOn[rank][pitch] = newReq;
}

// These handlers are called by the MIDI library when the corresponding
// message arrives from one of the manuals. They are responsible for mapping
// the keypress into one or more note requests, depending on the current
// settings of the stops and couplers on the control panel.
void handleGreatNoteOn(byte channel, byte pitch, byte unused_velocity)
{
  // If we are playing the 8' rank with the Great manual,
  if (flag8Great)
  {
    // First, handle the normal note for this manual
    requestOn(NOTE_PRIME, RANK0, pitch);
    
    // Then check for sub-octave and super-octave couplers
    if (flag8Sub && pitch > 12)
      requestOn(NOTE_SUB, RANK0, pitch-12);
    if (flag8Super && pitch <= 115)
      requestOn(NOTE_SUPER, RANK0, pitch+12);
  }
  
  // And, if we are playing the 4' rank with the Great manual, (cross coupled)
  if (flag4Great)
  {
    requestOn(NOTE_RANK_PRIME, RANK1, pitch);
    
    if (flag4Sub && pitch > 12)
      requestOn(NOTE_RANK_SUB, RANK1, pitch-12);
    if (flag4Super && pitch <= 115)
      requestOn(NOTE_RANK_SUPER, RANK1, pitch+12);
  }
}

void handleGreatNoteOff(byte channel, byte pitch, byte unused_velocity)
{
  // First, handle the normal note for this manual
  requestOff(NOTE_PRIME, RANK0, pitch);
  
  // Turn off all optional couplers, disregarding flag states.
  // This is in case the flag state has changed: we don't want orphan notes left on!
  requestOff(NOTE_RANK_PRIME, RANK1, pitch);

  if (pitch > 12)
  {
    requestOff(NOTE_SUB, RANK0, pitch-12);
    requestOff(NOTE_RANK_SUB, RANK1, pitch-12);
  }
  
  if (pitch <= 115)
  {
    requestOff(NOTE_SUPER, RANK0, pitch+12);
    requestOff(NOTE_RANK_SUPER, RANK1, pitch+12);
  }

}

void handleSwellNoteOn(byte channel, byte pitch, byte unused_velocity)
{
  // If we are playing the 4' rank with the Swell manual,
  if (flag4Swell)
  {
    // First, handle the normal note for this manual
    requestOn(NOTE_PRIME, RANK1, pitch);
    
    // Then check for sub-octave and super-octave couplers
    if (flag4Sub && pitch > 12)
      requestOn(NOTE_SUB, RANK1, pitch-12);
    if (flag4Super && pitch <= 115)
      requestOn(NOTE_SUPER, RANK1, pitch+12);
  }
  
  // And, if we are playing the 8' rank with the Swell manual, (cross coupled)
  if (flag8Swell)
  {
    requestOn(NOTE_RANK_PRIME, RANK0, pitch);
    
    if (flag8Sub && pitch > 12)
      requestOn(NOTE_RANK_SUB, RANK0, pitch-12);
    if (flag8Super && pitch <= 115)
      requestOn(NOTE_RANK_SUPER, RANK0, pitch+12);
  }
}

void handleSwellNoteOff(byte channel, byte pitch, byte unused_velocity)
{
    // First, handle the normal note for this manual
  requestOff(NOTE_PRIME, RANK1, pitch);
  
  // Turn off all optional couplers, disregarding flag states.
  // This is in case the flag state has changed: we don't want orphan notes left on!
  requestOff(NOTE_RANK_PRIME, RANK0, pitch);

  if (pitch > 12)
  {
    requestOff(NOTE_SUB, RANK1, pitch-12);
    requestOff(NOTE_RANK_SUB, RANK0, pitch-12);
  }
  
  if (pitch <= 115)
  {
    requestOff(NOTE_SUPER, RANK1, pitch+12);
    requestOff(NOTE_RANK_SUPER, RANK0, pitch+12);
  }

}

void handleExtNoteOn(byte channel, byte pitch, byte unused_velocity)
{
  // External MIDI just passes thru, if enabled
  if (flagMidi)
    requestOn(NOTE_PRIME, rankForChannel(channel), pitch);
}

void handleExtNoteOff(byte channel, byte pitch, byte unused_velocity)
{
  // External MIDI just passes thru, if enabled
  if (flagMidi)
    requestOff(NOTE_PRIME, rankForChannel(channel), pitch);

}


// The control panel tells us what the mapping should be between manual keypresses
// and organ pipes sounded. It consists of ... !!!
// This function looks at the current state of all the controls and updates.
void pollControlPanel()
{
  // Read control console latching switches into these flags.
  flag8Great = !digitalRead(PIN_8GREAT);
  flag8Swell = !digitalRead(PIN_8SWELL);
  flag4Swell = !digitalRead(PIN_4SWELL);
  flag4Great = !digitalRead(PIN_4GREAT);
  flag8Sub   = !digitalRead(PIN_8SUB);
  flag8Super = !digitalRead(PIN_8SUPER);
  flag4Sub   = !digitalRead(PIN_4SUB);
  flag4Super = !digitalRead(PIN_4SUPER);
  flagMidi   = !digitalRead(PIN_MIDI);
  flagBlower = !digitalRead(PIN_BLOWER);
  
  // The switches are illuminated; echo the switch state to its LED
  digitalWrite(PIN_8GREAT_LED, flag8Great);
  digitalWrite(PIN_8SWELL_LED, flag8Swell);
  digitalWrite(PIN_4SWELL_LED, flag4Swell);
  digitalWrite(PIN_4GREAT_LED, flag4Great);
  digitalWrite(PIN_8SUB_LED,   flag8Sub);
  digitalWrite(PIN_8SUPER_LED, flag8Super);
  digitalWrite(PIN_4SUB_LED,   flag4Sub);
  digitalWrite(PIN_4SUPER_LED, flag4Super);
  digitalWrite(PIN_MIDI_LED, flagMidi);
  digitalWrite(PIN_BLOWER_LED, flagBlower);
}

void dumpControlPanel()
{
  Serial.print(flag8Great);
  Serial.print(flag8Swell);
  Serial.print(flag4Swell);
  Serial.print(flag4Great);
  Serial.print(flag8Sub);
  Serial.print(flag8Super);
  Serial.print(flag4Sub);
  Serial.print(flag4Super);
  Serial.print(flagMidi);
  Serial.print(flagBlower);
  Serial.println();
}

void setup()
{
  
  //Debug port
  Serial.begin(115200);
  Serial.println("Opus 1 MIDI Controller");
  
  pinMode(PIN_8GREAT, INPUT_PULLUP);
  pinMode(PIN_8SWELL, INPUT_PULLUP);
  pinMode(PIN_4SWELL, INPUT_PULLUP);
  pinMode(PIN_4GREAT, INPUT_PULLUP);
  pinMode(PIN_8SUB, INPUT_PULLUP);
  pinMode(PIN_8SUPER, INPUT_PULLUP);
  pinMode(PIN_4SUB, INPUT_PULLUP);
  pinMode(PIN_4SUPER, INPUT_PULLUP);
  pinMode(PIN_MIDI, INPUT_PULLUP);
  pinMode(PIN_BLOWER, INPUT_PULLUP);

  pinMode(PIN_8GREAT_LED, OUTPUT);
  pinMode(PIN_8SWELL_LED, OUTPUT);
  pinMode(PIN_4SWELL_LED, OUTPUT);
  pinMode(PIN_4GREAT_LED, OUTPUT);
  pinMode(PIN_8SUB_LED, OUTPUT);
  pinMode(PIN_8SUPER_LED, OUTPUT);
  pinMode(PIN_4SUB_LED, OUTPUT);
  pinMode(PIN_4SUPER_LED, OUTPUT);
  pinMode(PIN_MIDI_LED, OUTPUT);
  pinMode(PIN_BLOWER_LED, OUTPUT);
  
  // All processing is handled in the receive callbacks for NoteOn and NoteOff.
  midiGreat.setHandleNoteOn(handleGreatNoteOn);
  midiGreat.setHandleNoteOff(handleGreatNoteOff);
  midiSwell.setHandleNoteOn(handleSwellNoteOn);
  midiSwell.setHandleNoteOff(handleSwellNoteOff);
  midiExt.setHandleNoteOn(handleExtNoteOn);
  midiExt.setHandleNoteOff(handleExtNoteOff);
  
  // Begin receive processing. Listen to all channels so we don't care about keyboard configuration.
  midiGreat.begin(MIDI_CHANNEL_OMNI);
  midiSwell.begin(MIDI_CHANNEL_OMNI);
  midiExt.begin(MIDI_CHANNEL_OMNI);
  
  // We don't want automatic echoing of MIDI messages.
  midiGreat.turnThruOff();
  midiSwell.turnThruOff();
  midiExt.turnThruOff();
  
  // Cancel anything going on right now
  allOff();
  
  // debug
  pollControlPanel();
  dumpControlPanel();
}

void loop()
{
  pollControlPanel();
  midiGreat.read();
  midiSwell.read();
  midiExt.read();
}

