// #include <webrtc/typedefs.h>
#include <webrtc/modules/audio_processing/include/audio_processing.h>

#include "aec_controller.h"

// https://github.com/pulseaudio/pulseaudio/blob/master/src/modules/echo-cancel/webrtc.cc

namespace audio_processing
{

AecController::AecController(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels)
{
    init(sample_rate, bit_per_sample, channels);
}

void AecController::init(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels)
{

}

void AecController::internalPlayback(const void *speaker_data, std::size_t speaker_size)
{

}

void AecController::internalCapture(void *capture_data, std::size_t capture_size)
{

}

}
