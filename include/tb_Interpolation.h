#pragma once

#include "tb_Core.h"
#include "tb_Space.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace tb::catmullRom {

enum class Type {
    Uniform,
    Centripetal,
    Chordal
};

/**
 * @brief Calculates the size needed for the output line when calling `spline`
 *
 * @param inLineSize The number of points in the input line (must be >= 4)
 * @param interpolationSteps The number of points to insert between each pair of original points
 * @return The required size for the `outLine` parameter of the `spline` function
 *
 * @note The first and last control points are only used for calculating tangents
 *       and are not included in the output curve
 */
inline int outLineSize(int inLineSize, int interpolationSteps) {
    tb_assert(inLineSize >= 4);
    tb_assert(interpolationSteps > 0);
    inLineSize -= 2; // The control points on either end will not be used
    return inLineSize + (inLineSize - 1) * interpolationSteps;
}

namespace detail {

inline double alphaForType(Type type) {
    switch (type) {
        case Type::Centripetal: return 0.5;
        case Type::Chordal:     return 1.0;
        case Type::Uniform:
        default:                return 0.0;
    }
}

// Barry-Goldman recursive formulation of Catmull-Rom, parameterized by knot values
// derived from the distance between consecutive control points (raised to `alpha`).
// This generalizes the standard uniform Catmull-Rom (alpha = 0, all knot intervals
// equal to 1) to centripetal (alpha = 0.5) and chordal (alpha = 1) variants.
//
// Uniform parameterization implicitly assumes control points are evenly spaced. When
// they aren't -- e.g. a curve with tightly-clustered points next to widely-spaced ones
// -- it can produce cusps, overshoot, or even self-intersecting loops. Centripetal
// parameterization is specifically designed to avoid this and is generally the safest
// default for unevenly-spaced data; chordal sits between the two.
inline Point interpolate(const Point& p0, const Point& p1, const Point& p2, const Point& p3,
                          double alpha, double t) {
    auto knotDelta = [alpha](const Point& a, const Point& b) -> double {
        if (alpha == 0.0)
            return 1.0;

        const auto dx = static_cast<double>(b.x) - static_cast<double>(a.x);
        const auto dy = static_cast<double>(b.y) - static_cast<double>(a.y);

        // Guard against coincident (or near-coincident) points, which would otherwise
        // produce a zero-length knot interval and a singular parameterization below.
        constexpr double min_dist = 1e-6;
        return std::pow(std::max(std::sqrt(dx * dx + dy * dy), min_dist), alpha);
    };

    const double t0 = 0.0;
    const double t1 = t0 + knotDelta(p0, p1);
    const double t2 = t1 + knotDelta(p1, p2);
    const double t3 = t2 + knotDelta(p2, p3);

    // Map the incoming normalized [0, 1] parameter (between p1 and p2) onto the real
    // knot interval [t1, t2]
    const double tt = t1 + t * (t2 - t1);

    auto lerp = [](const Point& a, const Point& b, double denom, double num) -> Point {
        const auto f = denom != 0.0 ? num / denom : 0.0;
        return Point(static_cast<float>(a.x + (b.x - a.x) * f),
                     static_cast<float>(a.y + (b.y - a.y) * f));
    };

    const auto A1 = lerp(p0, p1, t1 - t0, tt - t0);
    const auto A2 = lerp(p1, p2, t2 - t1, tt - t1);
    const auto A3 = lerp(p2, p3, t3 - t2, tt - t2);

    const auto B1 = lerp(A1, A2, t2 - t0, tt - t0);
    const auto B2 = lerp(A2, A3, t3 - t1, tt - t1);

    return lerp(B1, B2, t2 - t1, tt - t1);
}

} // namespace detail

/**
 * @brief Generates a Catmull-Rom spline through the provided control points
 *
 * @param outLine Output vector to store the resulting spline points. This must have the size
 *                calculated by calling outLineSize()
 * @param inLine Input vector containing the control points (must have >= 4 points)
 * @param interpolationSteps Number of points to insert between each pair of original points
 * @param type The sub-variant of Catmull-Rom to use
 *
 * @details This function creates a smooth curve that passes through all control points
 *          except the first and last ones, which are only used to determine the tangents
 *          at the endpoints. The algorithm inserts interpolationSteps new points between
 *          each pair of original points, using the Catmull-Rom formula to maintain
 *          smoothness across the entire curve.
 *
 * @note The size of outLine must match the value returned by outLineSize() for the same inputs
 */
inline void spline(std::vector<Point>& outLine, const std::vector<Point>& inLine,
                   int interpolationSteps, Type type) {
    tb_assert(inLine.size() >= 4);
    tb_assert(outLine.size() == outLineSize(inLine.size(), interpolationSteps));
    tb_assert(interpolationSteps > 0);

    if (type == Type::Uniform) {
        int outIdx = 0;

        // Iterate through points to create interpolated segments
        for (int i = 1; i + 2 < inLine.size(); i++) {
            // Determine control points
            const auto p0 = inLine[i - 1];
            const auto p1 = inLine[i];
            const auto p2 = inLine[i + 1];
            const auto p3 = inLine[i + 2];

            outLine[outIdx] = p1; // Add existing point
            outIdx++;

            // Interpolate between p1 and p2
            for (int j = 1; j <= interpolationSteps && outIdx < outLine.size(); ++j) {
                const auto t = static_cast<double>(j) / (interpolationSteps + 1);

                // Catmull-Rom spline calculation
                const auto t2 = t * t;
                const auto t3 = t2 * t;

                const auto x = 0.5 * (
                    (2.0 * p1.x) +
                    (-p0.x + p2.x) * t +
                    (2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x) * t2 +
                    (-p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x) * t3
                );

                const auto y = 0.5 * (
                    (2.0 * p1.y) +
                    (-p0.y + p2.y) * t +
                    (2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y) * t2 +
                    (-p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y) * t3
                );

                outLine[outIdx] = Point(x, y);
                outIdx++;
            }
        }

        tb_assert(outIdx == outLine.size() - 1);
        outLine[outIdx] = inLine[inLine.size() - 2]; // Add last existing point
    } else if (type == Type::Centripetal || type == Type::Chordal) {
        const double alpha = detail::alphaForType(type);
        int outIdx = 0;

        for (int i = 1; i + 2 < inLine.size(); i++) {
            const auto p0 = inLine[i - 1];
            const auto p1 = inLine[i];
            const auto p2 = inLine[i + 1];
            const auto p3 = inLine[i + 2];

            outLine[outIdx] = p1; // Add existing point
            outIdx++;

            // Interpolate between p1 and p2
            for (int j = 1; j <= interpolationSteps && outIdx < outLine.size(); ++j) {
                const auto t = static_cast<double>(j) / (interpolationSteps + 1);
                outLine[outIdx] = detail::interpolate(p0, p1, p2, p3, alpha, t);
                outIdx++;
            }
        }

        tb_assert(outIdx == outLine.size() - 1);
        outLine[outIdx] = inLine[inLine.size() - 2]; // Add last existing point
    } else
        tb_assert(false); // Not implemented/supported
}

}