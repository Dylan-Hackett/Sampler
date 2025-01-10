#pragma once
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <cmath>

namespace synthux {

class Looper {
  public:
    void Init(float *buf, size_t length) {
      _buffer = buf;
      _buffer_length = length;
      // Reset buffer contents to zero
      memset(_buffer, 0, sizeof(float) * _buffer_length);
    }

    void SetRecording(bool is_rec_on) {
        // Initialize recording head position on start
        if (_rec_env_pos_inc <= 0 && is_rec_on) {
            _rec_head = fmodf(_loop_start + _play_head_pos, static_cast<float>(_buffer_length));
            _is_empty = false;
        }
        // Set ramp for fade in/out
        _rec_env_pos_inc = is_rec_on ? 1 : -1;
    }

    void SetLoop(const float loop_start, const float loop_length) {
      // Set the start of the next loop
      _pending_loop_start = loop_start * (static_cast<float>(_buffer_length) - 1);

      // If the current loop start is not set yet, set it too
      if (!_is_loop_set) _loop_start = _pending_loop_start;

      // Set the length of the next loop
      _pending_loop_length = std::max(kMinLoopLength, loop_length * static_cast<float>(_buffer_length));

      // If the current loop length is not set yet, set it too
      if (!_is_loop_set) _loop_length = _pending_loop_length;
      _is_loop_set = true;
    }

    void SetPlaybackSpeed(float speed) {
        // Limit playback speed to a reasonable range (-2.0x to -0.5x and 0.5x to 2.0x)
        if (speed < 0.0f) {
            _playback_speed = clamp(speed, -2.0f, 2.0f);
        } else {
            _playback_speed = clamp(speed, 0.0f, 2.0f);
        }
    }

    void SetWetDryMix(float wet_dry) {
        _wet_dry = clamp(wet_dry, 0.0f, 1.0f);
    }

    float Process(float in) {
        // Playback logic
        float attenuation = 1.0f;
        float output_sample = 0.0f;

        // Calculate fade in/out
        if (_play_head_pos < kFadeLength) {
            attenuation = _play_head_pos / kFadeLength;
        } else if (_play_head_pos >= _loop_length - kFadeLength) {
            attenuation = (_loop_length - _play_head_pos) / kFadeLength;
        }

        // Calculate play position
        float play_pos = _loop_start + _play_head_pos;

        // Wrap around buffer boundaries
        while (play_pos >= static_cast<float>(_buffer_length)) {
            play_pos -= static_cast<float>(_buffer_length);
        }
        while (play_pos < 0) {
            play_pos += static_cast<float>(_buffer_length);
        }

        // Interpolate between samples for playback
        size_t idx0 = static_cast<size_t>(play_pos) % _buffer_length;
        size_t idx1 = (idx0 + 1) % _buffer_length;
        float frac = play_pos - static_cast<float>(idx0);
        float playback_sample = _buffer[idx0] * (1.0f - frac) + _buffer[idx1] * frac;

        playback_sample *= attenuation;

        // Mix the input and playback samples according to wet/dry mix for output
        output_sample = in * (1.0f - _wet_dry) + playback_sample * _wet_dry;

        // Recording logic
        if ((_rec_env_pos_inc > 0 && _rec_env_pos < kFadeLength)
            || (_rec_env_pos_inc < 0 && _rec_env_pos > 0)) {
            _rec_env_pos += _rec_env_pos_inc;
        }

        if (_rec_env_pos > 0 && _wet_dry > 0.0f && _wet_dry < 1.0f) {
            // Only record when wet/dry mix is between 0.0 and 1.0

            // Calculate fade in/out for recording
            float rec_attenuation = _rec_env_pos / kFadeLength;

            // Mix input and playback sample for recording
            float mixed_signal = in * (1.0f - _wet_dry) + playback_sample * _wet_dry;

            // Prevent clipping
            mixed_signal = clamp(mixed_signal, -1.0f, 1.0f);

            size_t rec_idx = static_cast<size_t>(_rec_head) % _buffer_length;
            _buffer[rec_idx] = mixed_signal * rec_attenuation + _buffer[rec_idx] * (1.f - rec_attenuation);

            _rec_head += 1.0f;
            if (_rec_head >= static_cast<float>(_buffer_length)) {
                _rec_head -= static_cast<float>(_buffer_length);
            }
            _is_empty = false;
        }

        // Advance playhead with variable speed
        _play_head_pos += _playback_speed;
        if (_play_head_pos >= _loop_length || _play_head_pos < 0) {
            _loop_start = _pending_loop_start;
            _loop_length = _pending_loop_length;
            _play_head_pos = fmodf(_play_head_pos, _loop_length);
            if (_play_head_pos < 0) {
                _play_head_pos += _loop_length;
            }
        }

        return output_sample;
    }

  private:
    // Clamp function since std::clamp is not available in C++14
    template<typename T>
    T clamp(const T& val, const T& min_val, const T& max_val)
    {
        return (val < min_val) ? min_val : (val > max_val) ? max_val : val;
    }

    static constexpr float kFadeLength = 600.0f;
    static constexpr float kMinLoopLength = 2.0f * kFadeLength;

    float* _buffer = nullptr;

    size_t _buffer_length = 0;
    float _loop_length = 0.0f;
    float _pending_loop_length = 0.0f;
    float _loop_start = 0.0f;
    float _pending_loop_start = 0.0f;

    float _play_head_pos = 0.0f;
    float _rec_head = 0.0f;

    float _rec_env_pos = 0.0f;
    int32_t _rec_env_pos_inc = 0;
    bool _is_empty = true;
    bool _is_loop_set = false;

    float _playback_speed = 1.0f; // Normal speed (positive for forward, negative for reverse)

    float _wet_dry = 0.5f; // Wet/dry mix (0.0 to 1.0)
};

} // namespace synthux
