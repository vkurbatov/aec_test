#include <iostream>
#include <thread>
#include <cstring>

#include "alsa_device.h"
#include "aec_controller.h"

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

    audio_devices::audio_params_t player_params(false, { 44100, 16, 1 }, 441 * 6, true);
    audio_devices::audio_params_t recorder_params(true, { 44100, 16, 1 }, 441 * 6, false);

    audio_processing::AecController aec_controller(44100, 16, 1);

	player.Open("default", player_params);
	recorder.Open("default", recorder_params);


    if (aec_controller.Reset())
    {
        while (true)
        {
            char buffer[882];
            std::int16_t buffer2[sizeof(buffer) / sizeof(std::int16_t)];

            for (auto& c : buffer2)
            {
                c = 0;// -32768;
            }

            // std::memset(buffer2, -32768, sizeof(buffer2));

            auto ret = recorder.Read(buffer, sizeof(buffer));

            //aec_controller.Playback(buffer2, sizeof(buffer));
             aec_controller.Capture(buffer, sizeof(buffer));

            if (ret > 0)
            {
                player.Write(buffer, ret);
            }

            // std::this_thread::sleep_for(std::chrono::milliseconds(20));

        }
    }

    return 0;
}
