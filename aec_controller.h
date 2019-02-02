#ifndef AEC_CONTROLLER_H
#define AEC_CONTROLLER_H

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_INCLUDE_AUDIO_PROCESSING_H_
namespace webrtc
{
class AudioProcessing;
class StreamConfig;
// class ProcessingConfig;
}
#endif

#include <memory>

namespace audio_processing
{

class AecController
{
    typedef std::unique_ptr<webrtc::AudioProcessing, void(*)(webrtc::AudioProcessing*)> webrtc_amp_ptr;
    typedef std::unique_ptr<webrtc::StreamConfig, void(*)(webrtc::StreamConfig*)> webrtc_cfg_ptr;

    webrtc_amp_ptr                              m_audio_processing;
    webrtc_cfg_ptr                              m_stream_config;

    std::uint32_t                               m_sample_rate;
    std::uint32_t                               m_bit_per_sample;
    std::uint32_t                               m_channels;
    std::uint32_t                               m_step_size;

public:
    AecController(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels);
    bool Playback(const void* speaker_data, std::size_t speaker_data_size);
    bool Capture(void* capture_data, std::size_t capture_data_size, void* output_data = nullptr);
    bool Reset();



private:
    webrtc::AudioProcessing* getAudioProcessor();
    bool init(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels);
    bool internalReset();
    bool internalPlayback(const void* speaker_data, std::size_t speaker_data_size);
    bool internalCapture(void* capture_data, std::size_t capture_data_size, void* output_data);
};

}

#endif // AEC_CONTROLLER_H
