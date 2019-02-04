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
    inline std::uint32_t frames_octets() const { return (bit_per_sample * channels) / 8; }
    inline std::uint32_t bytes_per_second() const { return (sample_rate * bit_per_sample * channels) / 8; }
    inline std::uint32_t duration_ms(std::size_t size) const { return (size * 1000) / bytes_per_second(); }
    inline std::size_t octets_count(std::uint32_t duration_ms) const { return (duration_ms * bytes_per_second()) / 1000; }
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
    using sample_buffer_t = std::vector<std::uint8_t>;

private:

    std::string                     m_device_name;

    snd_pcm_t*                      m_handle;

	audio_params_t					m_audio_params;

    std::uint32_t                   m_volume;

    sample_buffer_t                 m_sample_buffer;


public:

    AlsaDevice();
    ~AlsaDevice();

    static const device_names_list_t GetDeviceList(bool recorder, const std::string& hw_profile = "");

	bool Open(const std::string& device_name, const audio_params_t& audio_params = null_audio_params);
    bool Close();

    inline bool IsOpen() const;
	inline bool IsRecorder() const;

	inline const audio_params_t& GetParams() const;
	bool SetParams(const audio_params_t& audio_params);

	std::int32_t Read(void* capture_data, std::size_t size);
	std::int32_t Write(const void* playback_data, std::size_t size);

    inline void SetVolume(std::uint32_t volume) { m_volume = volume; }
    inline std::uint32_t GetVolume() const { return m_volume; }

private:

	std::int32_t setHardwareParams(const audio_params_t& audio_params);

	std::int32_t internalRead(void* capture_data, std::size_t size);
	std::int32_t internalWrite(const void* playback_data, std::size_t size);

};

}

#endif // ALSA_DEVICE_H
