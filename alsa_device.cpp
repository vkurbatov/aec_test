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

AlsaDevice::AlsaDevice(const std::string& hw_profile)
		: m_handle(nullptr)
		, m_hw_profile(hw_profile.empty() ? default_hw_profile : hw_profile)
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

bool AlsaDevice::IsOpen() const
{
	return m_handle != nullptr;
}

const AlsaDevice::device_names_list_t AlsaDevice::GetPlaybackDeviceInfo()
{
	return getDeviceInfoByDirection(false);
}

const AlsaDevice::device_names_list_t AlsaDevice::GetRecorderDeviceInfo()
{
	return getDeviceInfoByDirection(true);
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
