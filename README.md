##Stereo Looper with Chorus Effect
A stereo looper implementation for the Daisy Seed platform with integrated chorus effects.

#Features

Stereo input/output processing
15-second loop buffer per channel
Variable playback speed (-2x to +2x) with reverse capability
Wet/dry mix control
Stereo chorus effect with:

Adjustable delay time (5-30ms)
Dynamic panning
Variable feedback (0-0.9)



#Hardware Requirements

Daisy Seed board
4 potentiometers
1 momentary switch
Audio input/output connections

#Pin Configuration

Pin 15: Loop Start / Chorus Delay Control
Pin 16: Loop Length / Chorus Pan Control
Pin 17: Pitch Shift / Chorus Feedback Control
Pin 18: Record Button (Digital)
Pin 19: Wet/Dry Mix Control


Requires DaisyToolchain and DaisySP library. 



#Adjust loop parameters using knobs:
  Loop Start: Sets loop starting point
  Loop Length: Controls loop duration
  Pitch Shift: Adjusts playback speed and direction
  Wet/Dry: Balances effect mix


Press record button to start/stop recording

#Technical Specifications

Sample Rate: 48kHz
Buffer Length: 15 seconds
Audio Block Size: 4 samples
Chorus Delay Range: 5-30ms
Playback Speed Range: -2x to +2x

#Dependencies

daisy_seed.h
daisysp.h
looper.h
chorus.h

#Acknowledgments
Special thanks to:

Synthux for the looper file and logic
The DaisyToolchain development team for providing the robust embedded platform
The DaisySP library contributors for the audio processing foundation


License
This project uses code from the DaisySP library, which is licensed under MIT License.
Contributing
Bug reports and pull requests are welcome. Please follow the existing code style and add unit tests for any new features.
