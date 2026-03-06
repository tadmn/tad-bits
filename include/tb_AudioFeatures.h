#pragma once

#include <span>
#include <vector>
#include <array>
#include <cmath>
#include <cassert>

namespace tb {

// ─────────────────────────────────────────────────────────────────────────────
//  Mel filterbank
//
//  Builds a (nMelBins × nFftBins) triangular filterbank matrix on the mel
//  scale.  Call once at construction time; apply every frame with
//  applyMelFilterbank().
//
//  nFftBins  = N/2 + 1  (one-sided, inclusive)
//  fMin/fMax = frequency range to cover (Hz)
// ─────────────────────────────────────────────────────────────────────────────

inline std::vector<std::vector<float>> melFilterbank(
        std::size_t nMelBins,
        std::size_t nFftBins,
        double      sampleRate,
        double      fMin = 20.0,
        double      fMax = -1.0)   // -1 → nyquist
{
    if (fMax < 0.0)
        fMax = sampleRate / 2.0;

    // Hz → mel
    auto hzToMel = [](double hz) {
        return 2595.0 * std::log10(1.0 + hz / 700.0);
    };
    // mel → Hz
    auto melToHz = [](double mel) {
        return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0);
    };

    const double melMin = hzToMel(fMin);
    const double melMax = hzToMel(fMax);

    // nMelBins + 2 equally-spaced mel points (includes lower/upper edges)
    std::vector<double> melPoints(nMelBins + 2);
    for (std::size_t i = 0; i < melPoints.size(); ++i)
        melPoints[i] = melMin + i * (melMax - melMin) / (nMelBins + 1);

    // Convert to FFT bin indices
    const double freqResolution = (sampleRate / 2.0) / (nFftBins - 1);
    std::vector<double> binFreqs(nMelBins + 2);
    for (std::size_t i = 0; i < binFreqs.size(); ++i)
        binFreqs[i] = melToHz(melPoints[i]) / freqResolution;

    // Build triangular filters
    std::vector<std::vector<float>> fb(nMelBins, std::vector<float>(nFftBins, 0.f));

    for (std::size_t m = 0; m < nMelBins; ++m) {
        const double left   = binFreqs[m];
        const double center = binFreqs[m + 1];
        const double right  = binFreqs[m + 2];

        for (std::size_t k = 0; k < nFftBins; ++k) {
            const double f = static_cast<double>(k);
            if (f >= left && f <= center && center != left)
                fb[m][k] = static_cast<float>((f - left) / (center - left));
            else if (f > center && f <= right && right != center)
                fb[m][k] = static_cast<float>((right - f) / (right - center));
        }
    }

    return fb;
}


// Apply a pre-built mel filterbank to a one-sided FFT power spectrum.
// Writes log-compressed energy into dst (must have nMelBins elements).
// log(x + 1e-9) keeps -inf away from silent frames.
inline void applyMelFilterbank(
        std::span<const float>              fftPowerSpectrum,
        const std::vector<std::vector<float>>& fb,
        std::span<float>                    dst)
{
    assert(dst.size() == fb.size());
    for (std::size_t m = 0; m < fb.size(); ++m) {
        assert(fb[m].size() == fftPowerSpectrum.size());
        double energy = 0.0;
        for (std::size_t k = 0; k < fftPowerSpectrum.size(); ++k)
            energy += fb[m][k] * fftPowerSpectrum[k];
        dst[m] = static_cast<float>(std::log(energy + 1e-9));
    }
}


// ─────────────────────────────────────────────────────────────────────────────
//  Spectral flux
//
//  L1 norm of the positive difference between consecutive magnitude spectra
//  (half-wave rectified so only increases in energy are counted — this is
//  more robust to decay/fade than full L1 difference).
//
//  prev is the magnitude spectrum from the previous frame.
//  curr is the current magnitude spectrum.
//  After the call prev should be updated to curr by the caller.
// ─────────────────────────────────────────────────────────────────────────────

inline float spectralFlux(
        std::span<const float> prev,
        std::span<const float> curr)
{
    assert(prev.size() == curr.size());
    double flux = 0.0;
    for (std::size_t i = 0; i < curr.size(); ++i) {
        const double diff = curr[i] - prev[i];
        if (diff > 0.0)
            flux += diff;
    }
    return static_cast<float>(flux);
}


// ─────────────────────────────────────────────────────────────────────────────
//  RMS energy  (time-domain frame)
// ─────────────────────────────────────────────────────────────────────────────

inline float rmsEnergy(std::span<const float> samples)
{
    double sum = 0.0;
    for (float s : samples)
        sum += static_cast<double>(s) * s;
    return static_cast<float>(std::sqrt(sum / samples.size()));
}


// ─────────────────────────────────────────────────────────────────────────────
//  Spectral centroid  (kept for reference / offline analysis)
// ─────────────────────────────────────────────────────────────────────────────

static float spectralCentroid(std::span<const float> fftPowerSpectrum, double sampleRate) {
    const double freqResolution = sampleRate / (2.0 * (fftPowerSpectrum.size() - 1));

    double weightedSum  = 0.0;
    double magnitudeSum = 0.0;

    for (size_t i = 0; i < fftPowerSpectrum.size(); ++i) {
        double frequency = i * freqResolution;
        weightedSum  += frequency * fftPowerSpectrum[i];
        magnitudeSum += fftPowerSpectrum[i];
    }

    return magnitudeSum != 0.0
        ? static_cast<float>(weightedSum / magnitudeSum)
        : 0.f;
}

} // namespace tb