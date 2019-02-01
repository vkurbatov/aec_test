#ifndef AEC_CONTROLLER_H
#define AEC_CONTROLLER_H

#include <memory>

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_INCLUDE_AUDIO_PROCESSING_H_
namespace webrtc
{
class AudioProcessing;
class StreamConfig;
// class ProcessingConfig;
}
#endif

namespace audio_processing
{

class AecController
{
    std::unique_ptr<webrtc::AudioProcessing>    m_audio_processing;
    std::unique_ptr<webrtc::StreamConfig>       m_stream_config;

    std::uint32_t                               m_sample_rate;
    std::uint32_t                               m_bit_per_sample;
    std::uint32_t                               m_channels;
    std::uint32_t                               m_step_size;

public:
    AecController(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels);
    ~AecController();


private:
    webrtc::AudioProcessing* getAudioProcessor();
    bool init(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels);
    bool internalPlayback(const void* speaker_data, std::size_t speaker_data_size);
    bool internalCapture(void* capture_data, std::size_t capture_data_size);
};

}

#endif // AEC_CONTROLLER_H
