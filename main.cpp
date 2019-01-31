#include <iostream>
#include <thread>
#include "alsa_device.h"

int main()
{

    int i = 0;

	/*

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

	*/

	audio_devices::AlsaDevice recorder, player;

    audio_devices::audio_params_t player_params(false, { 44100, 16, 1 }, 441 * 6,true);
    audio_devices::audio_params_t recorder_params(true, { 44100, 16, 1 }, 441 * 6, false);

	player.Open("default", player_params);
	recorder.Open("default", recorder_params);

	while (true)
	{
		char buffer[441*2];

		auto ret = recorder.Read(buffer, sizeof(buffer));
		if (ret > 0)
		{
			player.Write(buffer, ret);
		}

		// std::this_thread::sleep_for(std::chrono::milliseconds(20));

	}

    return 0;
}
