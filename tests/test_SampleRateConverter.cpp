#include "tb_SampleRateConverter.h"
#include "tb_DspUtilities.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <choc_SampleBuffers.h>
#include <cmath>

using namespace tb;
using Catch::Approx;

TEST_CASE("SampleRateConverter - Construction", "[SampleRateConverter]") {
    SECTION("Valid construction with different channel counts") {
        REQUIRE_NOTHROW(SampleRateConverter(1, SampleRateConverter::Quality::BestQuality));
        REQUIRE_NOTHROW(SampleRateConverter(2, SampleRateConverter::Quality::MediumQuality));
        REQUIRE_NOTHROW(SampleRateConverter(8, SampleRateConverter::Quality::Fastest));
    }

    SECTION("Construction with all quality types") {
        REQUIRE_NOTHROW(SampleRateConverter(1, SampleRateConverter::Quality::BestQuality));
        REQUIRE_NOTHROW(SampleRateConverter(1, SampleRateConverter::Quality::MediumQuality));
        REQUIRE_NOTHROW(SampleRateConverter(1, SampleRateConverter::Quality::Fastest));
        REQUIRE_NOTHROW(SampleRateConverter(1, SampleRateConverter::Quality::ZeroOrderHold));
        REQUIRE_NOTHROW(SampleRateConverter(1, SampleRateConverter::Quality::Linear));
    }

    SECTION("Invalid construction with zero or negative channels") {
        REQUIRE_THROWS(SampleRateConverter(0, SampleRateConverter::Quality::BestQuality));
        REQUIRE_THROWS(SampleRateConverter(-1, SampleRateConverter::Quality::BestQuality));
    }

    SECTION("getNumChannels returns correct value") {
        SampleRateConverter src1(1, SampleRateConverter::Quality::Fastest);
        REQUIRE(src1.getNumChannels() == 1);

        SampleRateConverter src2(2, SampleRateConverter::Quality::Fastest);
        REQUIRE(src2.getNumChannels() == 2);

        SampleRateConverter src8(8, SampleRateConverter::Quality::Fastest);
        REQUIRE(src8.getNumChannels() == 8);
    }
}

TEST_CASE("SampleRateConverter - Version", "[SampleRateConverter]") {
    SECTION("getVersion returns non-null string") {
        const char* version = SampleRateConverter::getVersion();
        REQUIRE(version != nullptr);
        REQUIRE(std::strlen(version) > 0);
    }
}

TEST_CASE("SampleRateConverter - Processing mono signal", "[SampleRateConverter]") {
    SECTION("Upsampling 44.1kHz to 48kHz") {
        SampleRateConverter converter(1, SampleRateConverter::Quality::BestQuality);

        // Create input buffer with 100 samples of 440Hz sine wave at 44100 Hz
        const int inputFrames = 100;
        const double inputSampleRate = 44100.0;
        const double outputSampleRate = 48000.0;
        const double frequency = 440.0;

        auto inputBuffer = makeSineWave(frequency, inputSampleRate, 1, inputFrames);

        // Calculate expected output size
        const int expectedOutputFrames = static_cast<int>(std::ceil(inputFrames * outputSampleRate / inputSampleRate));
        choc::buffer::ChannelArrayBuffer<float> outputBuffer(1, expectedOutputFrames);

        // Process
        auto [in, out] = converter.process(
            inputBuffer,
            outputBuffer,
            inputSampleRate,
            outputSampleRate,
            true
        );

        REQUIRE(out.getNumFrames() > 0);
        REQUIRE(out.getNumFrames() <= expectedOutputFrames);
    }

    SECTION("Downsampling 48kHz to 44.1kHz") {
        SampleRateConverter converter(1, SampleRateConverter::Quality::BestQuality);

        const int inputFrames = 100;
        const double inputSampleRate = 48000.0;
        const double outputSampleRate = 44100.0;

        auto inputBuffer = makeSineWave(440.0, inputSampleRate, 1, inputFrames);

        const int expectedOutputFrames = static_cast<int>(std::ceil(inputFrames * outputSampleRate / inputSampleRate));
        choc::buffer::ChannelArrayBuffer<float> outputBuffer(1, expectedOutputFrames);

        auto [in, out] = converter.process(
            inputBuffer.getView(),
            outputBuffer.getView(),
            inputSampleRate,
            outputSampleRate,
            true
        );

        REQUIRE(out.getNumFrames() > 0);
        REQUIRE(out.getNumFrames() <= expectedOutputFrames);
    }

    SECTION("Processing DC signal preserves value") {
        SampleRateConverter converter(1, SampleRateConverter::Quality::BestQuality);

        const int inputFrames = 100;
        const double dcValue = 0.5f;

        choc::buffer::ChannelArrayBuffer<float> inputBuffer(1, inputFrames);
        for (int i = 0; i < inputFrames; ++i) {
            inputBuffer.getSample(0, i) = dcValue;
        }

        const int outputFrames = 109; // ~100 * 48000/44100
        choc::buffer::ChannelArrayBuffer<float> outputBuffer(1, outputFrames);

        auto [in, out] = converter.process(
            inputBuffer.getView(),
            outputBuffer.getView(),
            44100.0,
            48000.0,
            false
        );

        // Check that DC value is preserved (with small tolerance)
        for (int i = 0; i < out.getNumFrames(); ++i) {
            REQUIRE(outputBuffer.getSample(0, i) == Approx(dcValue).margin(0.01f));
        }
    }
}

TEST_CASE("SampleRateConverter - Processing stereo signal", "[SampleRateConverter]") {
    SECTION("Stereo upsampling maintains channel independence") {
        SampleRateConverter converter(2, SampleRateConverter::Quality::BestQuality);

        const int inputFrames = 100;
        const double inputSampleRate = 44100.0;
        const double outputSampleRate = 48000.0;

        choc::buffer::ChannelArrayBuffer<float> inputBuffer(2, inputFrames);

        // Fill left channel with 440Hz sine
        for (int i = 0; i < inputFrames; ++i) {
            inputBuffer.getSample(0, i) = std::sin(2.0 * M_PI * 440.0 * i / inputSampleRate);
        }

        // Fill right channel with 880Hz sine
        for (int i = 0; i < inputFrames; ++i) {
            inputBuffer.getSample(1, i) = std::sin(2.0 * M_PI * 880.0 * i / inputSampleRate);
        }

        const int outputFrames = 109;
        choc::buffer::ChannelArrayBuffer<float> outputBuffer(2, outputFrames);

        auto [in, out] = converter.process(
            inputBuffer.getView(),
            outputBuffer.getView(),
            inputSampleRate,
            outputSampleRate,
            true
        );

        REQUIRE(out.getNumFrames() > 0);
        REQUIRE(out.getNumFrames() <= outputFrames);

        // Verify channels are different (not copied)
        bool channelsDifferent = false;
        for (int i = 0; i < out.getNumFrames(); ++i) {
            if (std::abs(outputBuffer.getSample(0, i) - outputBuffer.getSample(1, i)) > 0.01f) {
                channelsDifferent = true;
                break;
            }
        }
        REQUIRE(channelsDifferent);
    }
}

TEST_CASE("SampleRateConverter - Reset", "[SampleRateConverter]") {
    SECTION("Reset doesn't throw") {
        SampleRateConverter converter(2, SampleRateConverter::Quality::Fastest);
        REQUIRE_NOTHROW(converter.reset());
    }

    SECTION("Reset clears internal state") {
        SampleRateConverter converter(1, SampleRateConverter::Quality::BestQuality);

        const int inputFrames = 100;
        auto inputBuffer = makeSineWave(440.0, 44100.0, 1, inputFrames);

        const int outputFrames = 109;
        choc::buffer::ChannelArrayBuffer<float> outputBuffer1(1, outputFrames);
        choc::buffer::ChannelArrayBuffer<float> outputBuffer2(1, outputFrames);

        // Process once
        auto [in1, out1] = converter.process(
            inputBuffer.getView(),
            outputBuffer1.getView(),
            44100.0,
            48000.0,
            false
        );

        // Reset and process again with same input
        converter.reset();

        auto [in2, out2] = converter.process(
            inputBuffer.getView(),
            outputBuffer2.getView(),
            44100.0,
            48000.0,
            false
        );

        // Should generate same number of frames
        REQUIRE(out1.getNumFrames() == out2.getNumFrames());

        // Outputs should be similar (since we reset the state)
        for (int i = 0; i < std::min(out1.getNumFrames(), out2.getNumFrames()); ++i) {
            REQUIRE(outputBuffer1.getSample(0, i) == Approx(outputBuffer2.getSample(0, i)).margin(0.0001f));
        }
    }
}

TEST_CASE("SampleRateConverter - Edge cases", "[SampleRateConverter]") {
    SECTION("Processing small buffer") {
        SampleRateConverter converter(1, SampleRateConverter::Quality::Fastest);

        const int inputFrames = 1;
        choc::buffer::ChannelArrayBuffer<float> inputBuffer(1, inputFrames);
        inputBuffer.getSample(0, 0) = 1.0f;

        const int outputFrames = 10;
        choc::buffer::ChannelArrayBuffer<float> outputBuffer(1, outputFrames);

        auto [in, out] = converter.process(
            inputBuffer.getView(),
            outputBuffer.getView(),
            44100.0,
            48000.0,
            false
        );

        REQUIRE(out.getNumFrames() >= 0);
    }

    SECTION("Multiple consecutive processing calls") {
        SampleRateConverter converter(1, SampleRateConverter::Quality::Fastest);

        const int inputFrames = 50;
        const int outputFrames = 60;

        for (int iteration = 0; iteration < 3; ++iteration) {
            auto inputBuffer = makeSineWave(440.0, 44100.0, 1, inputFrames);
            choc::buffer::ChannelArrayBuffer<float> outputBuffer(1, outputFrames);

            auto [in, out] = converter.process(
                inputBuffer.getView(),
                outputBuffer.getView(),
                44100.0,
                48000.0,
                false
            );

            REQUIRE(out.getNumFrames() > 0);
            REQUIRE(out.getNumFrames() <= outputFrames);
        }
    }
}

TEST_CASE("SampleRateConverter - Different quality settings produce output", "[SampleRateConverter]") {
    const int inputFrames = 100;
    const int outputFrames = 109;

    auto inputBuffer = makeSineWave(440.0, 44100.0, 1, inputFrames);

    auto testQuality = [&](SampleRateConverter::Quality quality) {
        SampleRateConverter converter(1, quality);
        choc::buffer::ChannelArrayBuffer<float> outputBuffer(1, outputFrames);

        auto [in, out] = converter.process(
            inputBuffer.getView(),
            outputBuffer.getView(),
            44100.0,
            48000.0,
            true
        );

        REQUIRE(out.getNumFrames() > 0);
    };

    SECTION("BestQuality") { testQuality(SampleRateConverter::Quality::BestQuality); }
    SECTION("MediumQuality") { testQuality(SampleRateConverter::Quality::MediumQuality); }
    SECTION("Fastest") { testQuality(SampleRateConverter::Quality::Fastest); }
    SECTION("ZeroOrderHold") { testQuality(SampleRateConverter::Quality::ZeroOrderHold); }
    SECTION("Linear") { testQuality(SampleRateConverter::Quality::Linear); }
}

TEST_CASE("SampleRateConverter - getLatencyInSamples", "[SampleRateConverter]") {
    SECTION("BestQuality - various sample rates") {
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 44100.0, 48000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 48000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 44100.0, 96000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 96000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 44100.0, 44100.0) >= 0);
    }

    SECTION("MediumQuality - various sample rates") {
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::MediumQuality, 44100.0, 48000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::MediumQuality, 48000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::MediumQuality, 44100.0, 96000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::MediumQuality, 96000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::MediumQuality, 44100.0, 44100.0) >= 0);
    }

    SECTION("Fastest - various sample rates") {
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Fastest, 44100.0, 48000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Fastest, 48000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Fastest, 44100.0, 96000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Fastest, 96000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Fastest, 44100.0, 44100.0) >= 0);
    }

    SECTION("ZeroOrderHold - various sample rates") {
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::ZeroOrderHold, 44100.0, 48000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::ZeroOrderHold, 48000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::ZeroOrderHold, 44100.0, 96000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::ZeroOrderHold, 96000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::ZeroOrderHold, 44100.0, 44100.0) >= 0);
    }

    SECTION("Linear - various sample rates") {
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Linear, 44100.0, 48000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Linear, 48000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Linear, 44100.0, 96000.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Linear, 96000.0, 44100.0) >= 0);
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Linear, 44100.0, 44100.0) >= 0);
    }

    SECTION("Additional sample rate conversions") {
        // 22.05kHz to 48kHz (more than 2x upsampling)
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 22050.0, 48000.0) >= 0);

        // 192kHz to 44.1kHz (more than 4x downsampling)
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::BestQuality, 192000.0, 44100.0) >= 0);

        // 48kHz to 96kHz (2x upsampling)
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::MediumQuality, 48000.0, 96000.0) >= 0);

        // 88.2kHz to 44.1kHz (2x downsampling)
        REQUIRE(SampleRateConverter::getLatencyInSamples(
            SampleRateConverter::Quality::Fastest, 88200.0, 44100.0) >= 0);
    }
}
