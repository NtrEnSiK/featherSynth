/*
  This is a dual oscillator polyphonic synthesizer, each with
  independent filters and envelopes. The frequency of each
  oscillator is constrained to a key/scale based on user input.
  There are also user inputs for the the probability that the note
  will play, the tempo, and the oscillator types.

  This project was designed by Jesse Simpson in collaboration
  with Bantam Tools.
  [PROJECT LINK]
*/

// Audio.h refers to the Adafruit fork of the PJRC Audio library.
// The forked version is required for use with the Adafruit
// Feather M4 Express.
#include <Audio.h>
#include "scales.h"

// middle A is the reference frequency for an
// equal-tempered scale. Set its frequency and note value.
// Thanks to Tom Igoe for this technique. Check out his
// Arduino sound examples at https://github.com/tigoe/SoundExamples.

#define NOTE_A4 69         // MIDI note value for middle A
#define FREQ_A4 440        // frequency for middle A

//a 2D array to store the notes of our scales in.
//There are 7 scales, with 18 notes each
int rootScaled[7][18] = {};

int arpAdvanceCounter = 0;
//used for setting the tempo
int delayTime = 0;
//
int probabilityOfPlaying = 50;
// the cutoff frequency for our Low Pass Filters
int freqCutoff;
//the key we want our scale in
int rootNote = 60;

//for button handling
int buttonPin = 5;
int buttonPushCounter = 0;
int buttonState = 0;
int lastButtonState = 0;
float frequency;

//These are the synth definitions generated by the
//PJRC Audio System Design Tool: https://www.pjrc.com/teensy/gui/

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform2;      //xy=109,246
AudioSynthWaveform       waveform1;      //xy=115,152
AudioFilterBiquad        biquad2;        //xy=255,245
AudioFilterBiquad        biquad1;        //xy=256,152
AudioEffectEnvelope      envelope2;      //xy=421,249
AudioEffectEnvelope      envelope1;      //xy=444,153
AudioMixer4              mixer1;         //xy=571,246
AudioOutputAnalogStereo  Audio_Output;          //xy=774,156
AudioConnection          patchCord1(waveform2, biquad2);
AudioConnection          patchCord2(waveform1, biquad1);
AudioConnection          patchCord3(biquad2, envelope2);
AudioConnection          patchCord4(biquad1, envelope1);
AudioConnection          patchCord5(envelope2, 0, mixer1, 1);
AudioConnection          patchCord6(envelope1, 0, mixer1, 0);
AudioConnection          patchCord7(mixer1, 0, Audio_Output, 0);
// GUItool: end automatically generated code

void setup() {
  //Set default audio memory
  AudioMemory(10);
  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
  //begin serial communication
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  //default settings for both oscillators
  oscillatorSetup();
  delay(500);
}


void loop() {
  //A function for button handling to change the instrument
  buttonHandler();
  //read our input sensors and store them in variables
  int bpmReading = analogRead(A2);
  int rootReading = analogRead(A3);
  int scaleReading = analogRead(A4);
  int probabilityReading = analogRead(A5);


  //result is the BPM we want in 4:4 (quarter notes)
  int masterTempo = map(bpmReading, 0, 1023, 60, 800);

  //Divide 60,000 (ms) by the desired BPM to get the duration of a single beat – a quarter note.
  //Quarter note duration = 60,000 / bpm = x ms.
  //example: Time between beats = 60,000/100 = 600ms
  delayTime = 60000 / masterTempo;

  // convert sensor reading to 21 - 108 range
  // which is the range of MIDI notes on an 88-key keyboard
  // (from A0 to C8):
  int rootNote = map(rootReading, 0, 1023, 21, 108);


  //map the scaleReading to the length of the array
  int scaleArrayPosition = map(scaleReading, 0, 1023, 0, scalesArrayLength - 1); // can also use scalesArrayLength

  //map the probability knob reading from 0-100 so that we can think about it in terms of %
  probabilityOfPlaying = map(probabilityReading, 0, 1023, 0, 100); // can also use diatonicArrayLength

  //populate the rootScaled array with all of the notes based on our chosen scale and root note
  for (int i = 0; i < scalesArrayLength; i++) {
    for (int j = 0; j < diatonicArrayLength; j++) {
      //climb up the scale
      //rootScaled[i][j] = rootNote + (scales[scaleArrayPosition][arpAdvanceCounter]);
      //randomize the note order
      rootScaled[i][j] = rootNote + (scales[scaleArrayPosition][random(arpAdvanceCounter)]);
    }
  }

  //read the position of the knob, pull the associated MIDI value from the array, and then convert it to a frequency
  frequency =  FREQ_A4 * pow(2, ((rootScaled[scaleArrayPosition][arpAdvanceCounter] - NOTE_A4) / 12.0));

  //randomly assign a frequency cutoff for the Low Pass Filter
  int r = random(100);
  if (r > 50) {
    freqCutoff = 7000;
  } else {
    freqCutoff = 2000;
  }

  //Calculate a random number
  int ranNum = random(100);
  //compare that number to the value of the probability knob
  if (ranNum <= probabilityOfPlaying) {
    arpAdvanceCounter++;
    if (arpAdvanceCounter >= diatonicArrayLength) {
      arpAdvanceCounter = 0;
    }
    playOsc1(frequency);
    playOsc2(frequency);
    waveform1.amplitude(.5);
    waveform2.amplitude(.5);
  } else {
    arpAdvanceCounter++;
    if (arpAdvanceCounter >= diatonicArrayLength) {
      arpAdvanceCounter = 0;
    }
    playOsc1(0);
    playOsc2(0);
    waveform1.amplitude(0);
    waveform2.amplitude(0);
  }

}

int playOsc1(int freq) {
  waveform1.frequency(freq);
  envelope1.noteOn();
  envelope1.attack(5);
  envelope1.hold(0);
  envelope1.decay(1000);
  envelope1.sustain(.5);
  biquad1.setLowpass(1, freqCutoff, 0.71);
  envelope1.noteOff();
  delay(delayTime);
}

int playOsc2(int freq) {
  //play oscillator2 at half of the indicated frequency (1 octave down)
  waveform2.frequency(freq / 2);
  envelope2.noteOn();
  envelope2.attack(5);
  envelope2.hold(0);
  envelope2.decay(1000);
  envelope2.sustain(.5);
  biquad2.setLowpass(0, freqCutoff, 0.71);
  envelope2.noteOff();
  delay(delayTime);

}

void buttonHandler() {
  // read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      buttonPushCounter++;
    } else {
      //if the button isn't pressed, don't do anything
    }
    delay(60);
  }
  lastButtonState = buttonState;

  switch (buttonPushCounter) {
    case 0:
      waveform1.begin(WAVEFORM_SINE);
      waveform2.begin(WAVEFORM_SINE);
      break;
    case 1:
      waveform1.begin(WAVEFORM_TRIANGLE);
      waveform2.begin(WAVEFORM_TRIANGLE);
      break;
    case 2:
      waveform1.begin(WAVEFORM_SAWTOOTH);
      waveform2.begin(WAVEFORM_SAWTOOTH);
      break;
    case 3:
      waveform1.begin(WAVEFORM_TRIANGLE);
      waveform2.begin(WAVEFORM_SAWTOOTH);
      break;
    case 4:
      waveform1.begin(WAVEFORM_SAWTOOTH_REVERSE);
      waveform2.begin(WAVEFORM_SAWTOOTH_REVERSE);
      break;
    case 5:
      waveform1.begin(WAVEFORM_SQUARE);
      waveform2.begin(WAVEFORM_SQUARE);
      break;
    case 6:
      waveform1.begin(WAVEFORM_SINE);
      waveform2.begin(WAVEFORM_SQUARE);
      break;
  }
  if (buttonPushCounter > 6) {
    buttonPushCounter = 0;
  }
}

void oscillatorSetup() {
  //Default settings for oscillator 1
  waveform1.begin(WAVEFORM_TRIANGLE);
  //Default envelope1
  envelope1.attack(5);
  envelope1.hold(0);
  envelope1.decay(1);
  envelope1.sustain(.5);
  //default filter1 set to lowpass
  biquad1.setLowpass(0, 14000, .71);
  waveform1.pulseWidth(.85);

  //Default settings for oscillator 2
  waveform2.begin(WAVEFORM_SAWTOOTH);
  //Default envelope2
  envelope2.attack(5);
  envelope2.hold(0);
  envelope2.decay(1);
  envelope2.sustain(.5);
  //default filter2 set to lowpass
  biquad2.setLowpass(0, 14000, .71);
  waveform2.pulseWidth(.45);

}
