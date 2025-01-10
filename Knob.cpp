// Include necessary headers
#include "daisy_seed.h"
#include "daisysp.h"
#include "looper.h"
#include "../../DaisySP/Source/Effects/chorus.h"

using namespace daisy;
using namespace synthux;

// Create a DaisySeed hardware object
DaisySeed hw;

// Use the Looper class
Looper looper_left;
Looper looper_right;

// Create Chorus objects
daisysp::Chorus chorus_left;
daisysp::Chorus chorus_right;

// Define constants
static const size_t kSampleRate = 48000;  
static const size_t kBufferLengthSec = 15;
static const size_t kBufferLengthSamples = kBufferLengthSec * kSampleRate;

// Allocate buffers in SDRAM
float DSY_SDRAM_BSS looper_buffer_left[kBufferLengthSamples];
float DSY_SDRAM_BSS looper_buffer_right[kBufferLengthSamples];

// Declare hardware peripherals
AnalogControl loop_start_knob, loop_length_knob;
AnalogControl pitch_shift_knob; // Pitch shift knob on pin 17
AnalogControl wet_dry_knob;     // Wet/dry mix knob on pin 19
Switch record_button;

// Audio callback function
void AudioCallback(const float *const *in, float **out, size_t size)
{
    float looper_out_left;
    float looper_out_right;
    float chorus_out_left;
    float chorus_out_right;
    float mixed_out_left;
    float mixed_out_right;

    for (size_t i = 0; i < size; i++)
    {
        // Process the looper with the input from left and right channels
        looper_out_left = looper_left.Process(in[0][i]);
        looper_out_right = looper_right.Process(in[1][i]);

        // Process the looper outputs through the chorus effects
        chorus_out_left = chorus_left.Process(looper_out_left);
        chorus_out_right = chorus_right.Process(looper_out_right);

        // Mix the dry (looper output) and wet (chorus output) signals at 50% each
        mixed_out_left = 0.5f * looper_out_left + 0.5f * chorus_out_left;
        mixed_out_right = 0.5f * looper_out_right + 0.5f * chorus_out_right;

        // Output the mixed signal to the left and right channels
        out[0][i] = mixed_out_left;
        out[1][i] = mixed_out_right;
    }
}

int main(void)
{
    // Initialize the hardware
    hw.Init();
    hw.SetAudioBlockSize(4); // Adjust block size if needed
    float sample_rate = hw.AudioSampleRate();

    // Initialize the loopers
    looper_left.Init(looper_buffer_left, kBufferLengthSamples);
    looper_right.Init(looper_buffer_right, kBufferLengthSamples);

    // Initialize the chorus effects
    chorus_left.Init(sample_rate);
    chorus_right.Init(sample_rate);

    // Set default delay times for the chorus effects
    // This will be updated dynamically, so you can set initial values
    chorus_left.SetDelayMs(10.0f);
    chorus_right.SetDelayMs(10.0f);

    // Set default LFO frequency and depth
    chorus_left.SetLfoFreq(0.5f);
    chorus_right.SetLfoFreq(0.5f * 1.02f); // Slight variation

    chorus_left.SetLfoDepth(0.5f);
    chorus_right.SetLfoDepth(0.5f * 0.95f); // Slight variation

    // Configure ADC pins for analog inputs (loop start, loop length, pitch shift, wet/dry)
    AdcChannelConfig adc_config[4];

    // Loop Start Knob on Pin 15
    adc_config[0].InitSingle(hw.GetPin(15));

    // Loop Length Knob on Pin 16
    adc_config[1].InitSingle(hw.GetPin(16));

    // Pitch Shift Knob on Pin 17
    adc_config[2].InitSingle(hw.GetPin(17));

    // Wet/Dry Mix Knob on Pin 19
    adc_config[3].InitSingle(hw.GetPin(19));

    // Initialize the ADC with the configurations
    hw.adc.Init(adc_config, 4);
    hw.adc.Start();

    // Initialize the analog controls
    loop_start_knob.Init(hw.adc.GetPtr(0), sample_rate);
    loop_length_knob.Init(hw.adc.GetPtr(1), sample_rate);
    pitch_shift_knob.Init(hw.adc.GetPtr(2), sample_rate);
    wet_dry_knob.Init(hw.adc.GetPtr(3), sample_rate);

    // Initialize the recording button on D18 (Pin 18)
    record_button.Init(hw.GetPin(18), sample_rate / 48.0f);

    // Start the audio callback
    hw.StartAudio(AudioCallback);

    // Main loop
    while (1)
    {
        // Debounce the record button
        record_button.Debounce();

        // Process the analog controls
        loop_start_knob.Process();
        loop_length_knob.Process();
        pitch_shift_knob.Process();
        wet_dry_knob.Process();

        // Get normalized values from 0.0 to 1.0
        float loop_start_val = loop_start_knob.Value();
        float loop_length_val = loop_length_knob.Value();
        float pitch_shift_val = pitch_shift_knob.Value();
        float wet_dry_val = wet_dry_knob.Value();

        // Set the loop parameters for both loopers
        looper_left.SetLoop(loop_start_val, loop_length_val);
        looper_right.SetLoop(loop_start_val, loop_length_val);

        // Set recording state based on the record button
        bool is_recording = record_button.Pressed();
        looper_left.SetRecording(is_recording);
        looper_right.SetRecording(is_recording);

        // Map pitch shift knob to playback speed and direction
        float max_speed = 2.0f;
        float playback_speed;
        if (pitch_shift_val < 0.5f)
        {
            // Reverse playback, speed from -2.0x to 0.0x
            playback_speed = - (1.0f - 2.0f * pitch_shift_val) * max_speed;
        }
        else
        {
            // Forward playback, speed from 0.0x to +2.0x
            playback_speed = (2.0f * pitch_shift_val - 1.0f) * max_speed;
        }

        // Set the playback speed for both loopers
        looper_left.SetPlaybackSpeed(playback_speed);
        looper_right.SetPlaybackSpeed(playback_speed);

        // Set the wet/dry mix in the loopers
        looper_left.SetWetDryMix(wet_dry_val);
        looper_right.SetWetDryMix(wet_dry_val);

        // Map knobs to Chorus parameters

        // Pin 15 (loop_start_knob) controls Chorus delay time (5 ms to 30 ms)
        float min_delay_ms = 5.0f;
        float max_delay_ms = 30.0f;
        float chorus_delay_ms_base = min_delay_ms + loop_start_val * (max_delay_ms - min_delay_ms);

        float chorus_delay_ms_left = chorus_delay_ms_base;
        float chorus_delay_ms_right = chorus_delay_ms_base * 1.02f; // Slight variation

        chorus_left.SetDelayMs(chorus_delay_ms_left);
        chorus_right.SetDelayMs(chorus_delay_ms_right);

        // Pin 16 (loop_length_knob) controls Chorus pan
        float pan_left = 0.5f - (loop_length_val * 0.5f);
        float pan_right = 0.5f + (loop_length_val * 0.5f);

        // Ensure pan values are within 0.0 to 1.0
        pan_left = fmaxf(0.0f, fminf(1.0f, pan_left));
        pan_right = fmaxf(0.0f, fminf(1.0f, pan_right));

        chorus_left.SetPan(pan_left);
        chorus_right.SetPan(pan_right);

        // Pin 17 (pitch_shift_knob) controls Chorus feedback (0.0 to 0.9)
        float chorus_feedback_base = pitch_shift_val * 0.9f;
        float chorus_feedback_left = chorus_feedback_base;
        float chorus_feedback_right = chorus_feedback_base * 1.05f; // Slight variation

        // Ensure feedback doesn't exceed maximum value
        chorus_feedback_right = fminf(chorus_feedback_right, 0.9f);

        chorus_left.SetFeedback(chorus_feedback_left);
        chorus_right.SetFeedback(chorus_feedback_right);

        // Add a small delay to prevent CPU overuse
        daisy::System::Delay(1);
    }
}
