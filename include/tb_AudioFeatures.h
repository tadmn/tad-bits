#pragma once

#include <span>

namespace tb {

// Expects one-sided FFT magnitudes [0, N/2] inclusive, where N is the FFT size.
static float spectralCentroid(std::span<const float> fftMagnitudes, double sampleRate) {
    const double freqResolution = sampleRate / (2.0 * (fftMagnitudes.size() - 1));

    double weightedSum = 0.0;
    double magnitudeSum = 0.0;

    for (size_t i = 0; i < fftMagnitudes.size(); ++i) {
        double frequency = i * freqResolution;
        weightedSum += frequency * fftMagnitudes[i];
        magnitudeSum += fftMagnitudes[i];
    }

    double centroid = 0.0;
    if (magnitudeSum != 0.0)
        centroid = weightedSum / magnitudeSum;

    return static_cast<float>(centroid);
}

}
