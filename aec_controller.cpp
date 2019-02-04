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
void pcm_to_float(const void* pcm_frame, std::size_t sample_count, float* float_frame)
{

    auto pcm_data =  static_cast<const Tval*>(pcm_frame);

    for( int i = 0; i < sample_count; i++ )
    {
        float_frame[i] = static_cast<float>(pcm_data[i]) / static_cast<float>(Tmax);
    }
}

void pcm_to_float(const void* pcm_frame, std::size_t sample_count, float* float_frame, std::uint32_t bit_per_sample)
{
    switch(bit_per_sample)
    {
        case 8:
            pcm_to_float<std::int8_t>(pcm_frame, sample_count, float_frame);
            break;
        case 16:
            pcm_to_float<std::int16_t>(pcm_frame, sample_count, float_frame);
            break;
        case 32:
            pcm_to_float<std::int32_t>(pcm_frame, sample_count, float_frame);
            break;
        default:
            throw("Error bit_per_sample parameter");
    }
}

template<typename Tval, Tval Tmax = std::numeric_limits<Tval>::max()>
void float_to_pcm(const float* float_frame, std::size_t sample_count, void* pcm_frame)
{
    auto pcm_data =  static_cast<Tval*>(pcm_frame);

    for( int i = 0; i < sample_count; i++ )
    {
        pcm_data[i] = static_cast<Tval>(float_frame[i] * static_cast<float>(Tmax));
    }
}


void float_to_pcm(const float* float_frame, std::size_t sample_count, void* pcm_frame, std::uint32_t bit_per_sample)
{
    switch(bit_per_sample)
    {
        case 8:
            float_to_pcm<std::int8_t>(float_frame, sample_count, pcm_frame);
            break;
        case 16:
            float_to_pcm<std::int16_t>(float_frame, sample_count, pcm_frame);
            break;
        case 32:
            float_to_pcm<std::int32_t>(float_frame, sample_count, pcm_frame);
            break;
        default:
            throw("Error bit_per_sample parameter");
    }
}

} // converters

template<typename T>
void webrtc_deletor(T* webrtc_obj)
{
    if (webrtc_obj != nullptr)
    {
        delete webrtc_obj;
    }
}

AudioProcessor::AudioProcessor(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels)
    : m_audio_processing(nullptr, webrtc_deletor<webrtc::AudioProcessing> )
    , m_stream_config(nullptr, webrtc_deletor<webrtc::StreamConfig> )
{
    channels = 1; // temporarily

    init(sample_rate, bit_per_sample, channels);
}


bool AudioProcessor::Playback(const void *speaker_data, std::size_t speaker_data_size)
{
    return internalPlayback(speaker_data, speaker_data_size);
}

bool AudioProcessor::Capture(void *capture_data, std::size_t capture_data_size, void *output_data)
{
    if (output_data == nullptr)
    {
        output_data = capture_data;
    }

    return internalCapture(capture_data, capture_data_size, output_data);
}

bool AudioProcessor::Reset()
{
    return internalReset();
}

void AudioProcessor::SetEchoCancellation(bool enabled, int32_t suppression_level)
{

    if (m_audio_processing != nullptr)
    {
        m_audio_processing->echo_cancellation()->Enable(enabled);

        if (suppression_level >= static_cast<std::int32_t>(webrtc::EchoCancellation::SuppressionLevel::kLowSuppression)
                && suppression_level <= static_cast<std::int32_t>(webrtc::EchoCancellation::SuppressionLevel::kHighSuppression))
        {
            m_audio_processing->echo_cancellation()->set_suppression_level(static_cast<webrtc::EchoCancellation::SuppressionLevel>(suppression_level));
        }
    }
}

bool AudioProcessor::IsEchoCancellationEnabled() const
{
    return m_audio_processing != nullptr && m_audio_processing->echo_cancellation()->is_enabled();
}

std::uint32_t AudioProcessor::GetEchoSuppressionLevel() const
{

    if (m_audio_processing != nullptr)
    {
        return static_cast<std::int32_t>(m_audio_processing->echo_cancellation()->suppression_level());
    }

    return -1;
}

void AudioProcessor::SetNoiseSuppression(bool enabled, int32_t suppression_level)
{
    if (m_audio_processing != nullptr)
    {
        m_audio_processing->noise_suppression()->Enable(enabled);

        if (suppression_level >= static_cast<std::int32_t>(webrtc::NoiseSuppression::Level::kLow)
            && suppression_level <= static_cast<std::int32_t>(webrtc::NoiseSuppression::Level::kVeryHigh))

        {
            m_audio_processing->noise_suppression()->set_level(static_cast<webrtc::NoiseSuppression::Level>(suppression_level));
        }
    }
}

bool AudioProcessor::IsNoiseSuppressionEnabled() const
{
    return m_audio_processing != nullptr && m_audio_processing->noise_suppression()->is_enabled();
}

uint32_t AudioProcessor::GetNoiseSuppressionLevel() const
{
    if (m_audio_processing != nullptr)
    {
        return static_cast<std::int32_t>(m_audio_processing->noise_suppression()->level());
    }

    return -1;
}

void AudioProcessor::SetHighPassFilter(bool enabled)
{
    if (m_audio_processing != nullptr)
    {
        m_audio_processing->high_pass_filter()->Enable(enabled);
    }
}

bool AudioProcessor::IsHighPassFilterEnabled() const
{
    return m_audio_processing != nullptr && m_audio_processing->high_pass_filter()->is_enabled();
}

void AudioProcessor::SetVoiceDetection(bool enabled, std::int32_t likelihood)
{
    if (m_audio_processing != nullptr)
    {
        m_audio_processing->voice_detection()->Enable(enabled);

        if (likelihood >= static_cast<std::int32_t>(webrtc::VoiceDetection::Likelihood::kVeryLowLikelihood)
            && likelihood <= static_cast<std::int32_t>(webrtc::VoiceDetection::Likelihood::kHighLikelihood))
        {
            m_audio_processing->voice_detection()->set_likelihood(static_cast<webrtc::VoiceDetection::Likelihood>(likelihood));
        }
    }
}

bool AudioProcessor::IsVoiceDetectionEnabled() const
{
    return m_audio_processing != nullptr && m_audio_processing->voice_detection()->is_enabled();
}

bool AudioProcessor::HasVoice() const
{
    return m_audio_processing != nullptr &&m_audio_processing->voice_detection()->stream_has_voice();
}

void AudioProcessor::SetGainControl(bool enabled, int32_t mode)
{
    if (m_audio_processing != nullptr)
    {
        m_audio_processing->gain_control()->Enable(enabled);

        if (mode >= static_cast<std::int32_t>(webrtc::GainControl::Mode::kAdaptiveAnalog)
            && mode <= static_cast<std::int32_t>(webrtc::GainControl::Mode::kFixedDigital))
        {
            m_audio_processing->gain_control()->set_mode(static_cast<webrtc::GainControl::Mode>(mode));

            if (mode == static_cast<std::int32_t>(webrtc::GainControl::Mode::kAdaptiveAnalog))
            {
                m_audio_processing->gain_control()->set_analog_level_limits(0, 255);
            }
        }
    }
}

bool AudioProcessor::IsGainControlEnabled() const
{
    return m_audio_processing != nullptr && m_audio_processing->gain_control()->is_enabled();
}

int32_t AudioProcessor::GetGainMode() const
{
    if (m_audio_processing != nullptr)
    {
        return static_cast<std::int32_t>(m_audio_processing->gain_control()->mode());
    }

    return -1;
}



webrtc::AudioProcessing* AudioProcessor::getAudioProcessor()
{
    if (m_audio_processing == nullptr)
    {
        internalReset();
    }
    return m_audio_processing.get();
}

bool AudioProcessor::init(std::uint32_t sample_rate, std::uint32_t bit_per_sample, std::uint32_t channels)
{
    m_sample_rate = sample_rate;
    m_bit_per_sample = bit_per_sample;
    m_channels = channels;
    m_step_size = (sample_rate * channels * bit_per_sample) / (8 * 100);

    m_stream_config.reset(new webrtc::StreamConfig(m_sample_rate, m_channels, false));
    auto sr = m_stream_config->sample_rate_hz();

    return getAudioProcessor() != nullptr;
}

bool AudioProcessor::internalReset()
{
    bool result = false;

    if (m_audio_processing == nullptr)
    {
        m_audio_processing.reset(webrtc::AudioProcessing::Create());

        LOG(info) << "Webrtc audio processor create success " LOG_END;
    }

    if(m_audio_processing != nullptr)
    {
        webrtc::ProcessingConfig config =
        {
            *m_stream_config,
            *m_stream_config,
            *m_stream_config,
            *m_stream_config
        };

        auto webrtc_err = m_audio_processing->Initialize(config);
        result = webrtc_err == webrtc::AudioProcessing::kNoError;

        if (!result)
        {
            m_audio_processing.reset(nullptr);
            LOG(error) << "Error config webrtc audio processing object, error =  " << webrtc_err LOG_END;
        }
        else
        {

            /*m_audio_processing->echo_control_mobile()->Enable(false);

            m_audio_processing->noise_suppression()->Enable(true);

            m_audio_processing->high_pass_filter()->Enable(true);

            m_audio_processing->gain_control()->Enable(true);

            m_audio_processing->voice_detection()->Enable(false);

            m_audio_processing->echo_cancellation()->enable_drift_compensation(false);
            m_audio_processing->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::SuppressionLevel::kHighSuppression);

            m_audio_processing->echo_cancellation()->Enable(true);*/


            LOG(info) << "Webrtc audio processor initialize success " LOG_END;
        }
    }

    return result;
}

bool AudioProcessor::internalPlayback(const void *speaker_data, std::size_t speaker_data_size)
{
    bool result = false;

    auto apm = getAudioProcessor();

    if (apm != nullptr)
    {

        auto speaker_ptr = static_cast<const std::uint8_t*>(speaker_data);

        auto sample_count = (m_step_size * 8) / m_bit_per_sample;

        std::vector<float> float_buffer(sample_count);

        while(speaker_data_size >= m_step_size)
        {

            converters::pcm_to_float(speaker_ptr, sample_count , float_buffer.data(), m_bit_per_sample);

            auto samples = float_buffer.data();

            auto webrtc_status = apm->ProcessReverseStream(&samples, *m_stream_config, *m_stream_config, &samples);

            result = webrtc_status == webrtc::AudioProcessing::kNoError;

            if (!result)
            {
                LOG(error) "Process reverse stream error = " << webrtc_status LOG_END;
                break;
            }

            speaker_data_size -= m_step_size;
            speaker_ptr += m_step_size;
        }

        if (speaker_data_size > 0)
        {
            LOG(error) "Remaing " << speaker_data_size << " unprocessed bytes in playback buffer" LOG_END;
        }
    }

    return result;
}

bool AudioProcessor::internalCapture(void *capture_data, std::size_t capture_data_size, void * output_data)
{
    bool result = false;

    auto apm = getAudioProcessor();

    if (apm != nullptr)
    {

        auto capturt_ptr = static_cast<std::uint8_t*>(capture_data);
        auto output_ptr = static_cast<std::uint8_t*>(output_data);

        auto sample_count = (m_step_size * 8) / m_bit_per_sample;

        std::vector<float> float_buffer(sample_count);

        while(capture_data_size >= m_step_size)
        {
            converters::pcm_to_float(capturt_ptr, sample_count, float_buffer.data(), m_bit_per_sample);

            auto samples = float_buffer.data();

            apm->set_stream_delay_ms(0);

            auto webrtc_status = apm->ProcessStream(&samples, *m_stream_config, *m_stream_config, &samples);

            result = webrtc_status == webrtc::AudioProcessing::kNoError;

            if (!result)
            {
                LOG(error) "Process stream error = " << webrtc_status LOG_END;
                break;
            }
            else
            {
                converters::float_to_pcm(float_buffer.data(), sample_count, output_ptr, m_bit_per_sample);
            }

            capture_data_size -= m_step_size;
            capturt_ptr += m_step_size;
            output_ptr += m_step_size;
        }

        if (capture_data_size > 0)
        {
            LOG(error) "Remaing " << capture_data_size << " unprocessed bytes in capture buffer" LOG_END;
        }
    }

    return result;
}


}
