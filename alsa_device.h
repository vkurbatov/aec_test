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

	audio_format_t(std::uint32_t sr = 0, std::uint32_t bps = 0, std::uint32_t c = 0)
        : sample_rate(sr)
        , bit_per_sample(bps)
        , channels(c)
    {}

	inline bool is_init() const { return sample_rate >= 8000 && bit_per_sample > 7 && channels > 0; }
	inline std::uint32_t frames_bytes() const { return (bit_per_sample * channels) / 8; }
};

static const audio_format_t default_audio_format = { 44100, 16, 1 };
static const audio_format_t null_audio_format = { 0, 0, 0 };

struct audio_params_t
{
	bool			recorder;
	audio_format_t	audio_format;
	std::uint32_t	buffer_size;
	bool			nonblock_mode;

	audio_params_t(bool rec = false, const audio_format_t& afmt = null_audio_format, std::uint32_t bsz = 0, bool nonblock = false)
		: recorder(rec)
		, audio_format(afmt)
		, buffer_size(bsz)
		, nonblock_mode(nonblock)
	{}

	inline bool is_init() const { return audio_format.is_init(); }
};


static const audio_params_t default_audio_params = { false, default_audio_format, 0, false };
static const audio_params_t null_audio_params = { false, null_audio_format, 0, false };

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

    using device_names_list_t = std::vector<audio_device_info>;

private:

    std::string                     m_hw_profile;
    std::string                     m_device_name;

    snd_pcm_t*                      m_handle;

    device_names_list_t             m_dictionary_devices;

	audio_params_t					m_audio_params;

public:

    AlsaDevice(const std::string& hw_profile = "");
    ~AlsaDevice();

    static const device_names_list_t GetDeviceInfo(const std::string& hw_profile = "");

	bool Open(const std::string& device_name, const audio_params_t& audio_params = null_audio_params);
    bool Close();

    inline bool IsOpen() const;
	inline bool IsRecorder() const;

	inline const audio_params_t& GetParams() const;
	bool SetParams(const audio_params_t& audio_params);

	std::int32_t Read(void* capture_data, std::size_t size);
	std::int32_t Write(const void* playback_data, std::size_t size);

    const device_names_list_t GetPlaybackDeviceInfo();
    const device_names_list_t GetRecorderDeviceInfo();

private:

	std::int32_t setHardwareParams(const audio_params_t& audio_params);

    const device_names_list_t getDeviceInfoByDirection(bool input);

    std::uint32_t updateDictionary(const std::string& hw_profile = "");

	std::int32_t internalRead(void* capture_data, std::size_t size);
	std::int32_t internalWrite(const void* playback_data, std::size_t size);

};

}

#endif // ALSA_DEVICE_H
