# Console-Opus-1.1

Arduino code for the original 10-button console for Opus 1.

## Hardware Configuration

The original console contains an Arduino MEGA 2560 controller board. Three Sparkfun MIDI breakout boards (BOB-09598) are connected to Serial1, Serial2, and Serial3. Ten locking pushbuttons with integral LED lights are connected to individual pins on the Mega.

The inputs Serial1 and Serial2 are to be connected, through MIDI breakout boards, to two standard MIDI keyboards. The input of Serial3 may be connected, through another MIDI breakout board, to an external source of MIDI commands. The output of Serial1 is connected through a long umbilical cable to the windchest, which contains two daisy-chained MIDI-to-Parallel converter boards, MTP-8 from J-Omega Electronics, which are in turn connected to the air valves from Peterson Electro-Musical Products, Inc.

## Console Functional Description

The console's job is to combine the two MIDI streams from the two keyboards (manuals) into a single MIDI stream for the MTP8 converters. It limits the number of concurrent notes played, because the MTP8 boards have a limited current sinking capability. The MTP8 boards are protected by external fuses, but in order to avoid blowing fuses, the console software must limit the number of notes the organ tries to play simultaneously.

A big pipe organ will have lots of stops (which connect ranks of pipes to manuals) and couplers (which either cross-connect ranks or connect ranks with octave offsets). Organ Donor Opus 1 has just two ranks of pipes (8' and 4') and two manuals (called Great and Swell), and its stops and couplers are implemented using eight of the ten pushbuttons. They are labeled as follows:

* 8' GREAT
* 8' SWELL
* 4' GREAT
* 4' SWELL
* 8' SUB
* 8' OCTAVE
* 4' SUB
* 4' OCTAVE

Each of the first four buttons maps the named manual onto the named rank of pipes. Any combination of the four possible mappings is allowed. The other four buttons enable octave couplers and suboctave couplers associated with each rank of pipes. An octave coupler activates the pipe one octave higher than played, in addition to the played pipe. A suboctave coupler does the same thing, only an octave lower. Each of these four buttons enables an octave or suboctave coupler associated with a named rank of pipes. Any activated coupler is in effect for notes played on either manual, as long as that manual is mapped onto that rank by one of the first four buttons.

The ninth pushbutton, labeled MIDI, enables MIDI commands from the external MIDI input to be sent to the MTP8 converters. The tenth pushbutton, labeled BLOWER, was originally intended to control the pipe organ's air source, but this function was never implemented (and shouldn't be; we've learned that users, especially children, push all the buttons willy nilly).

## Dependencies

We use the standard MIDI library for Arduino, https://github.com/FortySevenEffects/arduino_midi_library/
