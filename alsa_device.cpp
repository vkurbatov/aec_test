#include "alsa_device.h"

#include <cstring>
#include <algorithm>


#include <alsa/asoundlib.h>

const char* device_info_fields[] = {"NAME", "DESC",  "IOID" };
const char default_hw_profile[] = "plughw:";

namespace audio_devices
{

static std::string get_field_from_hint(const void* hint, const char* field_name)
{
	std::string result;

	auto filed_value = snd_device_name_get_hint(hint, field_name);

	if (filed_value != nullptr)
	{

		result = filed_value;

		if (std::strcmp(filed_value, "null") != 0)
		{
			free(filed_value);
		}
	}

	return std::move(result);
}

snd_pcm_format_t bits_to_snd_format(std::uint32_t bits)
{
    snd_pcm_format_t result = SND_PCM_FORMAT_UNKNOWN;

    switch(bits)
    {
        case 8:
            result = SND_PCM_FORMAT_U8;
        break;
        case 16:
            result = SND_PCM_FORMAT_U16;
        break;
        case 24:
            result = SND_PCM_FORMAT_U24;
        break;
        case 32:
            result = SND_PCM_FORMAT_U32;
    }

    return result;
}

AlsaDevice::AlsaDevice(const std::string& hw_profile)
		: m_handle(nullptr)
		, m_hw_profile(hw_profile.empty() ? default_hw_profile : hw_profile)
        , m_is_recorder(false)
        , m_nonblock_mode(false)
        , m_device_name("default")
        , m_buffer_size(0)
{

}

AlsaDevice::~AlsaDevice()
{

}

const AlsaDevice::device_names_list_t AlsaDevice::GetDeviceInfo(const std::string &hw_profile)
{
	char ** hints = nullptr;

	device_names_list_t device_list;

    auto result = snd_device_name_hint(-1, "pcm", (void ***)&hints);

	if (result >=  0)
	{
		auto it = hints;

		while(*it != nullptr)
		{
			audio_device_info device_info = { "", "", "", false, false };

			enum fields_enum_t : std::uint32_t { name, desc, ioid };

			std::uint32_t i = 0;

			bool append = false;

			for (const auto f : device_info_fields)
			{
				auto field_value = get_field_from_hint(*it, f);

				if ( field_value != "null" )
				{
					auto field_id = static_cast<fields_enum_t>(i++);

					if (!field_value.empty() || field_id == fields_enum_t::ioid)
					{
						switch(field_id)
						{
							case fields_enum_t::name:

								append = field_value == "default"
										|| (hw_profile.empty())
										|| field_value.find(hw_profile) == 0;

								if (append == true)
								{
									device_info.name = field_value;
								}
								break;

							case fields_enum_t::desc:
								{
									auto delimeter_pos = field_value.find('\n');
									if (delimeter_pos != std::string::npos)
									{
										device_info.description = field_value.substr(0, delimeter_pos);
										device_info.hint = field_value.substr(delimeter_pos + 1);
									}
									else
									{
										device_info.description = device_info.name;
										device_info.hint = field_value;
									}

								}
								break;

							case fields_enum_t::ioid:

								device_info.input = field_value.empty() || field_value == "Input";
								device_info.output = field_value.empty() || field_value == "Output";
								append = device_info.input || device_info.output;
						}

					}

					if (append == false)
					{
						break;
					}
				}

			}//foreach fields
			it++;

			if (append)
			{
				device_list.emplace_back(device_info);
			}

		}

		snd_device_name_free_hint((void**)hints);
	}

    return std::move(device_list);
}

bool AlsaDevice::Open(const std::string &device_name, bool recorder, const audio_format_t &audio_format, std::uint32_t buffer_size)
{
    bool result = false;

    if ( IsOpen() )
    {
        Close();
    }

    auto err = snd_pcm_open(&m_handle
                                 , device_name.c_str()
                                 , recorder ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE
                                 , SND_PCM_NONBLOCK);

    if (err > 0)
    {
        m_device_name = device_name;
        m_audio_format = audio_format;
        m_is_recorder = recorder;

        // Set
    }

    return result;
}

bool AlsaDevice::Close()
{
    bool result = false;

    return result;
}

bool AlsaDevice::IsOpen() const
{
    return m_handle != nullptr;
}

bool AlsaDevice::IsRecorder() const
{
    return m_is_recorder;
}

const audio_format_t &AlsaDevice::GetAudioFormat() const
{
    return m_audio_format;
}

bool AlsaDevice::SetAudioFormat(const audio_format_t &audio_format)
{
    bool result = false;

    if (IsOpen())
    {

    }

    return true;
}

const uint32_t AlsaDevice::GetBufferSize() const
{
        return m_audio_format;
}

bool AlsaDevice::SetBufferSize(const uint32_t &buffer_size)
{

}

const AlsaDevice::device_names_list_t AlsaDevice::GetPlaybackDeviceInfo()
{
    return std::move(getDeviceInfoByDirection(false));
}

const AlsaDevice::device_names_list_t AlsaDevice::GetRecorderDeviceInfo()
{
    return std::move(getDeviceInfoByDirection(true));
}

bool AlsaDevice::setHardwareParams()
{
    bool result = false;

    snd_pcm_hw_params_t *hw_params = nullptr;
    snd_pcm_hw_params_alloca(&hw_params);

    if (hw_params != nullptr)
    {
        auto err = snd_pcm_hw_params_any(m_handle, hw_params);

        if (err >= 0)
        {
            err = snd_pcm_hw_params_set_access(m_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
            if (err >= 0)
            {
                err = snd_pcm_hw_params_set_format(m_handle, hw_params, bits_to_snd_format(m_audio_format.bit_per_sample));
                if (err >= 0)
                {
                    err = snd_pcm_hw_params_set_channels(m_handle, hw_params, bits_to_snd_format(m_audio_format.channels));
                    if (err >= 0)
                    {
                        err = snd_pcm_hw_params_set_rate_near(m_handle, hw_params, &m_audio_format.sample_rate, nullptr);
                        if(err >= 0)
                        {
                            snd_pcm_uframes_t desired_period_size = (m_buffer_size * 8) / m_audio_format.bit_per_sample;
                            err = snd_pcm_hw_params_set_period_size_near(m_handle, hw_params, &desired_period_size, nullptr);


                        }
                    }
                }
            }

        }



        snd_pcm_hw_params_free(hw_params);
    }

    return result;
}

const AlsaDevice::device_names_list_t AlsaDevice::getDeviceInfoByDirection(bool input)
{
	updateDictionary(m_hw_profile);

	device_names_list_t playback_device_list;

	std::copy_if(m_dictionary_devices.begin(), m_dictionary_devices.end(), std::back_inserter(playback_device_list), [input](audio_device_info& info) { return input ? info.input : info.output; } );

	return std::move(playback_device_list);
}

uint32_t AlsaDevice::updateDictionary(const std::string& hw_profile)
{
	m_dictionary_devices = AlsaDevice::GetDeviceInfo(hw_profile);
	return m_dictionary_devices.size();
}

}
