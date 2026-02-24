#pragma once

#include <span>

namespace tb {

// Expects one-sided FFT power spectrum values [0, N/2] inclusive, where N is the FFT size.
// Values should be magnitude squared of raw fft complex output.
static float spectralCentroid(std::span<const float> fftPowerSpectrum, double sampleRate) {
    const double freqResolution = sampleRate / (2.0 * (fftPowerSpectrum.size() - 1));

    double weightedSum = 0.0;
    double magnitudeSum = 0.0;

    for (size_t i = 0; i < fftPowerSpectrum.size(); ++i) {
        double frequency = i * freqResolution;
        weightedSum += frequency * fftPowerSpectrum[i];
        magnitudeSum += fftPowerSpectrum[i];
    }

    double centroid = 0.0;
    if (magnitudeSum != 0.0)
        centroid = weightedSum / magnitudeSum;

    return static_cast<float>(centroid);
}

}
