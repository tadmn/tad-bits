
#pragma once

#include <choc_SampleBuffers.h>
#include <numbers>

namespace tb {

// Creates a sine wave buffer with the specified parameters
inline choc::buffer::ChannelArrayBuffer<float> makeSineWave(float frequency, double sampleRate,
                                                            int numChannels, int numSamples,
                                                            float amplitude = 1.f) {
    auto buffer = choc::buffer::ChannelArrayBuffer<float>(numChannels, numSamples);

    // Fill all channels with the same sine wave
    choc::buffer::setAllSamples(buffer, [=](int /*channel*/, int frame) {
        return amplitude * std::sin(2.0 * std::numbers::pi * frequency * frame / sampleRate);
    });

    return buffer;
}

}