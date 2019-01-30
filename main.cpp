#include <iostream>
#include "alsa_device.h"

int main()
{

    int i = 0;

	audio_devices::AlsaDevice audio_device;

	auto device_playback_list = audio_device.GetPlaybackDeviceInfo();
	auto device_recorder_list = audio_device.GetRecorderDeviceInfo();

	std::cout << "ALSA playback list " << device_playback_list.size() << ":" << std::endl;

	for (const auto& info : device_playback_list)
    {
		std::cout << "#" << i << ": " << info.name << ", " << info.description << std::endl;
        i++;
    }

	i = 0;

	std::cout << "ALSA recorder list " << device_recorder_list.size() << ":" << std::endl;

	for (const auto& info : device_recorder_list)
	{
		std::cout << "#" << i << ": " << info.name << ", " << info.description << std::endl;;
		i++;
	}

    return 0;
}
