#ifndef AEC_CONTROLLER_H
#define AEC_CONTROLLER_H

#include <memory>

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_INCLUDE_AUDIO_PROCESSING_H_
namespace webrtc
{
class AudioProcessing;
class StreamConfig;
class ProcessingConfig;
}
#endif

namespace audio_processing
{

class AecController
{
    std::unique_ptr<webrtc::AudioProcessing>    m_audio_processing;
    std::unique_ptr<webrtc::ProcessingConfig>   m_processing_config;

    std::uint32_t                               m_sample_rate;
    std::uint32_t                               m_bit_per_sample;
    std::uint32_t                               m_channels;

public:
    AecController(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels);
    ~AecController();

private:
    void init(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels);
    void internalPlayback(const void* speaker_data, std::size_t speaker_size);
    void internalCapture(void* capture_data, std::size_t capture_size);
};

}

#endif // AEC_CONTROLLER_H
