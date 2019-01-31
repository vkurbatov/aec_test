#define ALSA_PCM_NEW_HW_PARAMS_API 1

extern "C"
{
#include <alsa/asoundlib.h>
}

#include "alsa_device.h"

#include <cstring>
#include <algorithm>
#include <iostream>

#define LOG(a)	std::cout << "[" << #a << "] "

#define LOG_END << std::endl;

const char* device_info_fields[] = {"NAME", "DESC",  "IOID" };
const char default_hw_profile[] = "plughw:";
const char default_device_name[] = "default";

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
			result = SND_PCM_FORMAT_S16_LE;
        break;
        case 24:
			result = SND_PCM_FORMAT_S24_LE;
        break;
        case 32:
			result = SND_PCM_FORMAT_S32_LE;
    }

    return result;
}

AlsaDevice::AlsaDevice(const std::string& hw_profile)
		: m_handle(nullptr)
		, m_hw_profile(hw_profile.empty() ? default_hw_profile : hw_profile)
        , m_device_name("default")
{

}

AlsaDevice::~AlsaDevice()
{
	Close();
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

								append = (field_value == default_device_name)
										|| (hw_profile.empty())
										|| (field_value.find(hw_profile) == 0);

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

bool AlsaDevice::Open(const std::string &device_name, const audio_params_t& audio_params)
{
    bool result = false;
	if ( IsOpen() )
	{
		Close();
	}

	if ( audio_params.is_init() )
	{

		auto err = snd_pcm_open(&m_handle
									 , device_name.c_str()
									 , audio_params.recorder ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK
									 , SND_PCM_NONBLOCK);
		if (err >= 0)
		{
			// snd_pcm_nonblock(m_handle, 0);
			result = setHardwareParams(audio_params) >= 0;

			if (result == false)
			{
				Close();
				LOG(warning) << "Can't Open device [" << device_name << "]: error set hardware params" LOG_END;
			}
			else
			{
				m_audio_params = audio_params;
				LOG(info) "Open device [" << device_name << "]: success" LOG_END;
			}
		}
		else
		{
			LOG(warning) << "Can't Open device [" << device_name << "]: errno = " << errno LOG_END;
		}
	}
	else
	{
		LOG(warning) << "Can't Open device [" << device_name << "]: audio params not set" LOG_END;
	}

    return result;
}

bool AlsaDevice::Close()
{
    bool result = false;

	if (m_handle != nullptr)
	{
		snd_pcm_abort(m_handle);
		snd_pcm_close(m_handle);

		m_handle = nullptr;

		LOG(info) << "Device [" << m_device_name << "] closed" LOG_END;
	}

    return result;
}

bool AlsaDevice::IsOpen() const
{
    return m_handle != nullptr;
}

bool AlsaDevice::IsRecorder() const
{
	return m_audio_params.recorder;
}

const audio_params_t &AlsaDevice::GetParams() const
{
	return m_audio_params;
}

bool AlsaDevice::SetParams(const audio_params_t &audio_params)
{
	bool result = audio_params.is_init() && (!IsOpen() || setHardwareParams(audio_params) >= 0);

	if (result == true)
	{
		m_audio_params = audio_params;
	}
	else
	{
		LOG(info) << "Cant't set params for [" << m_device_name << "]" LOG_END;
	}

	return result;
}

std::int32_t AlsaDevice::Read(void *capture_data, std::size_t size)
{
	std::int32_t result = -EBADF;

	if ( IsOpen() )
	{
		result = internalRead(capture_data, size);
	}

	return result;
}

std::int32_t AlsaDevice::Write(const void *playback_data, std::size_t size)
{
	std::int32_t result = -EBADF;

	if ( IsOpen() )
	{
		result = internalWrite(playback_data, size);
	}

	return result;
}

const AlsaDevice::device_names_list_t AlsaDevice::GetPlaybackDeviceInfo()
{
    return std::move(getDeviceInfoByDirection(false));
}

const AlsaDevice::device_names_list_t AlsaDevice::GetRecorderDeviceInfo()
{
    return std::move(getDeviceInfoByDirection(true));
}

std::int32_t AlsaDevice::setHardwareParams(const audio_params_t& audio_params)
{
	std::int32_t result = -EINVAL;

	if ( audio_params.is_init() )
	{
		snd_pcm_hw_params_t* hw_params = nullptr;
		snd_pcm_hw_params_alloca(&hw_params);

		if (hw_params != nullptr)
		{
			// for braking seq
			do
			{
				result = snd_pcm_hw_params_any(m_handle, hw_params);
				if (result < 0)
				{
					LOG(error) << "Can't init hardware params, errno = " << result LOG_END;
					break;
				}

				result = snd_pcm_hw_params_set_access(m_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
				if (result < 0)
				{
					LOG(error) << "Can't set access hardware params, errno = " << result LOG_END;
					break;
				}

				result = snd_pcm_hw_params_set_format(m_handle, hw_params, bits_to_snd_format(audio_params.audio_format.bit_per_sample));
				if (result < 0)
				{
					LOG(error) << "Can't set format S" << audio_params.audio_format.bit_per_sample << " hardware params, errno = " << result LOG_END;
					break;
				}

				result = snd_pcm_hw_params_set_channels(m_handle, hw_params, audio_params.audio_format.channels);
				if (result < 0)
				{
					LOG(error) << "Can't set channels " << audio_params.audio_format.channels << " hardware params, errno = " << result LOG_END;
					break;
				}

				auto sample_rate = audio_params.audio_format.sample_rate;
				result = snd_pcm_hw_params_set_rate_near(m_handle, hw_params, &sample_rate, nullptr);
				if (result < 0)
				{
					LOG(error) << "Can't set sample rate " << sample_rate << " hardware params, errno = " << result LOG_END;
					break;
				}

				//default buffer_size
				if(audio_params.buffer_size == 0)
				{
					break;
				}

				snd_pcm_uframes_t period_size = (audio_params.buffer_size * audio_params.audio_format.bit_per_sample) / 8;
				result = snd_pcm_hw_params_set_buffer_size_near(m_handle, hw_params, &period_size);

				if (result < 0)
				{
					LOG(error) << "Can't set buffer size " << period_size << " hardware params, errno = " << result LOG_END;
					break;
				}

				period_size = audio_params.buffer_size;
				result = snd_pcm_hw_params_set_period_size_near(m_handle, hw_params, &period_size, nullptr);
				if (result < 0)
				{
					LOG(error) << "Can't set period size " << period_size << " hardware params, errno = " << result LOG_END;
					break;
				}

				result = snd_pcm_hw_params(m_handle, hw_params);
				if(result < 0)
				{
					LOG(error) << "Can't set hardware params, errno = " << errno LOG_END;
					break;
				}

				result = snd_pcm_nonblock(m_handle, static_cast<int>(audio_params.nonblock_mode));
				if(result < 0)
				{
					LOG(error) << "Can't set " << (audio_params.nonblock_mode ? "nonblock" : "block") << " mode, errno = " << result LOG_END;
					break;
				}

			}
			while(false);

		}

		if (result < 0)
		{
			result = -errno;
		}
		else
		{
			LOG(info) << "Set hardware params success " << errno LOG_END;
		}
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

std::uint32_t AlsaDevice::updateDictionary(const std::string& hw_profile)
{
	m_dictionary_devices = AlsaDevice::GetDeviceInfo(hw_profile);
	return m_dictionary_devices.size();
}

std::int32_t AlsaDevice::internalRead(void *capture_data, std::size_t size)
{
	std::int32_t result = -1, total = 0;

	auto data = static_cast<std::int8_t*>(capture_data);

	auto frame_bytes = m_audio_params.audio_format.frames_bytes();

	std::int32_t err;
	do
	{
		err = snd_pcm_readi(m_handle, data, size / frame_bytes);

		if (err > 0)
		{
			size -= err * frame_bytes;
			data += err * frame_bytes;
			total += err * frame_bytes;
		}
	}
	while((err == -EAGAIN || err > 0) && (size > 0));

	if (err >= 0)
	{
		result = total;
		// LOG(debug) << "Read " << total << " bytes from device success" LOG_END;
	}
	else
	{
		LOG(error) << "Faile read from device, errno = " << err LOG_END;
	}
	return result;
}

std::int32_t AlsaDevice::internalWrite(const void *playback_data, std::size_t size)
{
	std::int32_t result = 0, total = 0;

	auto data = static_cast<const std::int8_t*>(playback_data);

	auto frame_bytes = m_audio_params.audio_format.frames_bytes();

	std::int32_t err;
	bool work;

	do
	{
		work = false;
		err = snd_pcm_writei(m_handle, data, size / frame_bytes);

		if (err > 0)
		{
			size -= err * frame_bytes;
			data += err * frame_bytes;
			total += err * frame_bytes;

			work = size > 0;
		}
		else
		{
			switch(err)
			{
				case -EPIPE:
					snd_pcm_prepare(m_handle);
				case -EAGAIN:
					work = true;
					break;
				case -ESTRPIPE:
					while (snd_pcm_resume(m_handle) == -EAGAIN);
					break;
			}
		}
	}
	while(work);

	if (err >= 0)
	{
		result = total;
		// LOG(debug) << "Write " << total << " bytes to device success " LOG_END;
	}
	else
	{
		LOG(error) << "Failed write to device, errno = " << err LOG_END;
	}

	return result;
}

}
