#pragma once

#include "tb_Core.h"
#include <samplerate.h>
#include <choc_SampleBuffers.h>
#include <memory>
#include <vector>

namespace tb {

/**
 * C++ wrapper for libsamplerate using choc audio buffers.
 * Works with non-interleaved (planar) audio data only.
 */
class SampleRateConverter {
public:
    /**
     * Converter quality types (maps to libsamplerate converter types)
     */
    enum class Quality {
        BestQuality   = SRC_SINC_BEST_QUALITY,     // Highest quality, slowest
        MediumQuality = SRC_SINC_MEDIUM_QUALITY,   // Good quality, moderate speed
        Fastest       = SRC_SINC_FASTEST,          // Lower quality, fastest
        ZeroOrderHold = SRC_ZERO_ORDER_HOLD,       // Very fast, lowest quality
        Linear        = SRC_LINEAR                 // Fast, low quality
    };

    /**
     * @param numChannels Number of audio channels (must be > 0)
     * @param quality Conversion quality
     */
    explicit SampleRateConverter(int numChannels, Quality quality) {
        tb_throwIf(numChannels <= 0);

        // Create one converter per channel for independent processing
        mConverters.reserve(numChannels);
        for (int i = 0; i < numChannels; ++i) {
            int error = 0;
            SRC_STATE* state = src_new(static_cast<int>(quality), 1, &error);
            if (!state || error != 0)
                tb_throw(std::string("Failed to create SRC state: ") + src_strerror(error));

            mConverters.emplace_back(state);
        }
    }

    ~SampleRateConverter() = default;

    // Prevent copying & moving
    SampleRateConverter(const SampleRateConverter&) = delete;
    SampleRateConverter& operator=(const SampleRateConverter&) = delete;

    /**
     * Process audio using choc::buffer::ChannelArrayView (non-interleaved/planar)
     * @param input Input buffer view (planar: separate channel buffers)
     * @param output Output buffer view (planar, must be pre-allocated)
     * @param inSampleRate Input sample rate in Hz
     * @param outSampleRate Output sample rate in Hz
     * @param endOfInput True if this is the last buffer
     * @return Number of frames actually written to output
     */
    int process(choc::buffer::ChannelArrayView<float> input,
                choc::buffer::ChannelArrayView<float> output,
                double inSampleRate,
                double outSampleRate,
                bool endOfInput = false) {
        tb_assert(!mConverters.empty() && inSampleRate > 0.0 && outSampleRate > 0.0);
        tb_assert(input.getNumChannels()  == getNumChannels() &&
                  output.getNumChannels() == getNumChannels());

        uint32_t framesGenerated = 0;

        // Process each channel independently
        for (uint32_t ch = 0; ch < input.getNumChannels(); ++ch) {
            SRC_DATA srcData;
            srcData.data_in = input.getChannel(ch).data.data;
            srcData.input_frames = static_cast<long>(input.getNumFrames());
            srcData.data_out = output.getChannel(ch).data.data;
            srcData.output_frames = static_cast<long>(output.getNumFrames());
            srcData.src_ratio = inSampleRate / outSampleRate;
            srcData.end_of_input = endOfInput ? 1 : 0;

            const int error = src_process(mConverters[ch].get(), &srcData);
            if (error != 0) {
                tb_throwMsgIf(error != 0, std::string("SRC processing error: ") + src_strerror(error));
            }

            if (ch == 0) {
                framesGenerated = srcData.output_frames_gen;
            } else if (srcData.output_frames_gen != framesGenerated) {
                tb_assert(srcData.output_frames_gen == framesGenerated);
            }
        }

        return static_cast<int>(framesGenerated);
    }

    /**
     * Reset the converter state for all channels
     */
    void reset() {
        for (auto& converter : mConverters) {
            const int error = src_reset(converter.get());
            tb_throwMsgIf(error != 0, std::string("Failed to reset SRC state: ") + src_strerror(error));
        }
    }

    /**
     * @return The number of channels set in the constructor
     */
    int getNumChannels() const noexcept { return static_cast<int>(mConverters.size()); }

    /**
     * Get libsamplerate version string
     */
    static const char* getVersion() { return src_get_version(); }

private:
    struct SRCStateDeleter {
        void operator()(SRC_STATE* state) const {
            if (state) {
                src_delete(state);
            }
        }
    };

    std::vector<std::unique_ptr<SRC_STATE, SRCStateDeleter>> mConverters;
};

}