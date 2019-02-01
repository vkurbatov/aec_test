//#include <webrtc/typedefs.h>
#include <webrtc/modules/audio_processing/include/audio_processing.h>

#include "aec_controller.h"

#include <vector>
#include <limits>

#ifndef LOG_END

#include <iostream>

#define LOG(a)	std::cout << "[" << #a << "] "
#define LOG_END << std::endl;

#endif

// https://github.com/pulseaudio/pulseaudio/blob/master/src/modules/echo-cancel/webrtc.cc

namespace audio_processing
{

namespace converters
{

template<typename Tval, Tval Tmax = std::numeric_limits<Tval>::max()>
void pcm_to_float(const void* pcm_frame, std::size_t size, void *float_frame)
{

    for( int i = 0; i < (size / sizeof(Tval)); i++ )
    {
        auto& sample = *(static_cast<const Tval*>(pcm_frame) + i);

        static_cast<float*>(float_frame)[i] = static_cast<float>(Tmax) / static_cast<float>(sample);
    }
}

void pcm_to_float(const void* pcm_frame, std::size_t size, void *float_frame, std::uint32_t bit_per_sample)
{
    switch(bit_per_sample)
    {
        case 8:
            pcm_to_float<std::int8_t>(pcm_frame, size, float_frame);
            break;
        case 16:
            pcm_to_float<std::int16_t>(pcm_frame, size, float_frame);
            break;
        case 32:
            pcm_to_float<std::int32_t>(pcm_frame, size, float_frame);
            break;
        default:
            throw("Error bit_per_sample parameter: ");
    }
}

template<typename Tval, Tval Tmax = std::numeric_limits<Tval>::max()>
void float_to_pcm(const void* float_frame, std::size_t size, void* pcm_frame)
{
    std::vector<Tval> result;

    for( int i = 0; i < (size / sizeof(float)); i++ )
    {
        auto& sample = *(static_cast<const float*>(float_frame) + i);

        static_cast<Tval*>(pcm_frame)[i] = static_cast<Tval>(sample * static_cast<float>(Tmax));
    }
}


void float_to_pcm(const float* float_frame, std::size_t size, void* pcm_frame, std::uint32_t bit_per_sample)
{
    switch(bit_per_sample)
    {
        case 8:
            float_to_pcm<std::int8_t>(float_frame, size, pcm_frame);
            break;
        case 16:
            float_to_pcm<std::int16_t>(float_frame, size, pcm_frame);
            break;
        case 32:
            float_to_pcm<std::int32_t>(float_frame, size, pcm_frame);
            break;
        default:
            throw("Error bit_per_sample parameter: ");
    }
}

} // converters

AecController::AecController(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels)
{
    init(sample_rate, bit_per_sample, channels);
}

webrtc::AudioProcessing* AecController::getAudioProcessor()
{
    if (m_audio_processing == nullptr)
    {
        if (m_stream_config != nullptr)
        {
            webrtc::ProcessingConfig config =
            {
                *m_stream_config,
                *m_stream_config,
                *m_stream_config,
                *m_stream_config
            };

            m_audio_processing.reset(webrtc::AudioProcessing::Create());

            if (m_audio_processing != nullptr)
            {
                if (m_audio_processing->Initialize(config) != webrtc::AudioProcessing::kNoError)
                {
                    m_audio_processing.reset(nullptr);
                }
                else
                {
                    m_audio_processing->echo_cancellation()->Enable(true);
                    m_audio_processing->echo_control_mobile()->Enable(false);
                }
            }
        }
    }
    return m_audio_processing.get();
}

bool AecController::init(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels)
{
    m_sample_rate = sample_rate;
    m_bit_per_sample = bit_per_sample;
    m_channels = channels;
    m_step_size = (sample_rate * channels * bit_per_sample) / (8 * 100);

    m_stream_config.reset(new webrtc::StreamConfig(m_sample_rate, m_channels, false));
    return m_stream_config != nullptr;
}

bool AecController::internalPlayback(const void *speaker_data, std::size_t speaker_data_size)
{
    bool result = false;

    auto apm = getAudioProcessor();

    if (apm != nullptr)
    {

        auto data = static_cast<const std::uint8_t*>(speaker_data);

        std::vector<float> float_buffer((m_step_size * 8) / m_bit_per_sample);

        while(speaker_data_size >= m_step_size)
        {

            converters::pcm_to_float(data, m_step_size, float_buffer.data(), m_bit_per_sample);

            auto samples = float_buffer.data();

            auto webrtc_status = apm->ProcessReverseStream(&samples, *m_stream_config, *m_stream_config, &samples);

            result = webrtc_status == webrtc::AudioProcessing::kNoError;

            if (!result)
            {
                LOG(error) "Process reverse stream error = " << webrtc_status LOG_END;
                break;
            }

            speaker_data_size -= m_step_size;
            data += m_step_size;
        }

        if (speaker_data_size > 0)
        {
            LOG(error) "Remaing " << speaker_data_size << " unprocessed bytes in playback buffer" LOG_END;
        }
    }

    return result;
}

bool AecController::internalCapture(void *capture_data, std::size_t capture_data_size)
{
    bool result = false;

    auto apm = getAudioProcessor();

    if (apm != nullptr)
    {

        auto data = static_cast<std::uint8_t*>(capture_data);

        std::vector<float> float_buffer((m_step_size * 8) / m_bit_per_sample);

        while(capture_data_size >= m_step_size)
        {
            converters::pcm_to_float(data, m_step_size, float_buffer.data(), m_bit_per_sample);

            auto samples = float_buffer.data();

            auto webrtc_status = apm->ProcessStream(&samples, *m_stream_config, *m_stream_config, &samples);

            result = webrtc_status == webrtc::AudioProcessing::kNoError;

            if (!result)
            {
                LOG(error) "Process stream error = " << webrtc_status LOG_END;
                break;
            }
            else
            {
                converters::float_to_pcm(float_buffer.data(), m_step_size, data, m_bit_per_sample);
            }

            capture_data_size -= m_step_size;
            data += m_step_size;
        }

        if (capture_data_size > 0)
        {
            LOG(error) "Remaing " << capture_data_size << " unprocessed bytes in capture buffer" LOG_END;
        }
    }

    return result;
}

}
