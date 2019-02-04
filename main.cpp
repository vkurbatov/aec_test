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

    const std::uint32_t sample_rate = 48000;
    const std::uint32_t frame_size = sample_rate / 100;
    const std::uint32_t buffers_count = 2;


	audio_devices::AlsaDevice recorder, player;

    audio_devices::audio_params_t player_params(false, { sample_rate, 16, 1 }, frame_size * 6, true);
    audio_devices::audio_params_t recorder_params(true, { sample_rate, 16, 1 }, frame_size * 1 + 40, false);

    audio_processing::AecController aec_controller(sample_rate, 16, 1);

	player.Open("default", player_params);
	recorder.Open("default", recorder_params);


    auto begin = std::chrono::high_resolution_clock::now();

    if (aec_controller.Reset())
    {
        int i = 0;

        char buffers [buffers_count][frame_size * 2];

        char empty_buffer[frame_size * 2];

        std::memset(empty_buffer, 0, sizeof(empty_buffer));

        std::memset(buffers, 0, sizeof(buffers));

        while (true)
        {
            // std::memset(buffer2, -32768, sizeof(buffer2));

            auto t_1 = std::chrono::high_resolution_clock::now();

            auto r_idx = (i + 0) % buffers_count;
            auto w_idx = (i + 0) % buffers_count;

            auto& read_buffer = buffers[r_idx];
            auto& write_buffer = buffers[w_idx];

            auto ret = recorder.Read(read_buffer, sizeof(read_buffer));

            /*if (i < 1)
            {
                // aec_controller.Playback(buffer2, sizeof(buffer));
                // aec_controller.Playback(buffer2, sizeof(buffer));
                aec_controller.Playback(write_buffer, sizeof(write_buffer));
            }*/

            auto aec_t_1 = std::chrono::high_resolution_clock::now();

            aec_controller.Playback(write_buffer, sizeof(write_buffer));

            aec_controller.Capture(read_buffer, sizeof(read_buffer));

            auto aec_1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - aec_t_1).count();
            /*if (aec_1 > 0)
            {
                std::cout << "AEC delay = " << aec_1 << std::endl;
            }*/

            auto t_2 = std::chrono::high_resolution_clock::now();

            if (ret > 0)
            {
                player.Write(write_buffer, ret);
            }

            begin += std::chrono::milliseconds(10);

            std::this_thread::sleep_for(begin - std::chrono::high_resolution_clock::now());

            auto dl_1 = std::chrono::duration_cast<std::chrono::milliseconds>(t_2 - t_1).count();
            auto dl_2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_2).count();

            // std::cout << "Read time = " << dl_1 << ", write time = " << dl_2 << std::endl;
            i++;
        }
    }

    return 0;
}
