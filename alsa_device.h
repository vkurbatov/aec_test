#ifndef ALSA_DEVICE_H
#define ALSA_DEVICE_H

#include <string>
#include <vector>
#include <memory>

namespace audio_devices
{

#ifndef __ALSA_PCM_H
struct snd_pcm_t;
#endif

struct audio_format_t
{
    std::uint32_t   sample_rate;
    std::uint32_t   bit_per_sample;
    std::uint32_t   channels;
};

struct audio_device_info
{
    std::string name;
    std::string description;
    std::string hint;
    bool        input;
    bool        output;
};

class AlsaDevice
{
public:

    static constexpr audio_format_t default_audio_format = { 44100, 16, 1 };

    using device_names_list_t = std::vector<audio_device_info>;

private:

    std::string                     m_hw_profile;

    std::string                     m_device_name;

    snd_pcm_t*                      m_handle;

    audio_format_t                  m_audio_format;

    device_names_list_t             m_dictionary_devices;

    bool                            m_is_recorder;

public:

    AlsaDevice(const std::string& hw_profile = "");
    ~AlsaDevice();

    static const device_names_list_t GetDeviceInfo(const std::string& hw_profile = "");

    bool Open(const std::string& device_name, const audio_format_t& audio_format, bool recorder);
    bool Close();
    inline bool IsOpen() const;

    const device_names_list_t GetPlaybackDeviceInfo();
    const device_names_list_t GetRecorderDeviceInfo();

private:

    const device_names_list_t getDeviceInfoByDirection(bool input);
    std::uint32_t updateDictionary(const std::string& hw_profile = "");

};

}

#endif // ALSA_DEVICE_H
