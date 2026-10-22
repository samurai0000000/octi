/*
 * EloCalculator.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "EloCalculator.hxx"

#include <algorithm>
#include <cmath>

namespace octi {

namespace {

double scoreOf(EloOutcome outcome)
{
    switch (outcome) {
    case EloOutcome::FIRST_WINS:
        return 1.0;
    case EloOutcome::SECOND_WINS:
        return 0.0;
    case EloOutcome::DRAW:
        break;
    }
    return 0.5;
}

int clampK(int k)
{
    return std::max(0, std::min(k, EloCalculator::kMaxK));
}

/* Rounded K * (S - E) for the first side. */
int rawDelta(double rating1, double rating2, EloOutcome outcome, int k)
{
    double expected = EloCalculator::expectedScore(rating1, rating2);
    double raw = static_cast<double>(clampK(k)) * (scoreOf(outcome) - expected);
    return static_cast<int>(std::lround(raw));
}

} // namespace

int EloCalculator::clampRating(int rating)
{
    return std::max(kMinRating, std::min(rating, kMaxRating));
}

double EloCalculator::expectedScore(double ratingA, double ratingB)
{
    return 1.0 / (1.0 + std::pow(10.0, (ratingB - ratingA) / 400.0));
}

EloCalculator::DuelUpdate EloCalculator::updateDuel(int rating1, int rating2,
                                                    EloOutcome outcome, int k)
{
    int r1 = clampRating(rating1);
    int r2 = clampRating(rating2);
    int d = rawDelta(r1, r2, outcome, k);

    /* Keep both results inside the legal range while staying zero-sum. */
    int lo = std::max(kMinRating - r1, r2 - kMaxRating);
    int hi = std::min(kMaxRating - r1, r2 - kMinRating);
    d = std::max(lo, std::min(d, hi));

    DuelUpdate u;
    u.delta1 = d;
    u.delta2 = -d;
    u.newRating1 = r1 + d;
    u.newRating2 = r2 - d;
    return u;
}

EloCalculator::TeamUpdate
EloCalculator::updateTeams(const std::array<int, 4> &ratings,
                           EloOutcome outcome, int k)
{
    std::array<int, 4> r;
    for (size_t i = 0; i < r.size(); ++i) {
        r[i] = clampRating(ratings[i]);
    }
    double team1 = (static_cast<double>(r[0]) + static_cast<double>(r[2])) / 2.0;
    double team2 = (static_cast<double>(r[1]) + static_cast<double>(r[3])) / 2.0;
    int d = rawDelta(team1, team2, outcome, k);

    int lo = std::max(std::max(kMinRating - r[0], kMinRating - r[2]),
                      std::max(r[1] - kMaxRating, r[3] - kMaxRating));
    int hi = std::min(std::min(kMaxRating - r[0], kMaxRating - r[2]),
                      std::min(r[1] - kMinRating, r[3] - kMinRating));
    d = std::max(lo, std::min(d, hi));

    TeamUpdate u;
    u.delta[0] = d;
    u.delta[2] = d;
    u.delta[1] = -d;
    u.delta[3] = -d;
    for (size_t i = 0; i < r.size(); ++i) {
        u.newRating[i] = r[i] + u.delta[i];
    }
    return u;
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
