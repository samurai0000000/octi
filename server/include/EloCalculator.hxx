/*
 * EloCalculator.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_ELO_CALCULATOR_HXX
#define OCTI_SERVER_ELO_CALCULATOR_HXX

#include <array>

namespace octi {

/*
 * Outcome of a 1v1 duel or a 2v2 team game from the point of view of the
 * first participant (player 1, or team 1 = seats 1 and 3).
 */
enum class EloOutcome {
    FIRST_WINS,
    SECOND_WINS,
    DRAW
};

/*
 * Integer Elo arithmetic. Every update is exactly zero-sum: the deltas of
 * all participants add up to 0, and no resulting rating leaves
 * [kMinRating, kMaxRating] (the range AccountStore accepts for newElo).
 * Ratings handed in outside that range are clamped into it first.
 */
class EloCalculator {
public:
    static constexpr int kMinRating = 0;
    static constexpr int kMaxRating = 10000;
    static constexpr int kDefaultK = 32;
    static constexpr int kMaxK = 1000;

    struct DuelUpdate {
        int newRating1 = 0;
        int newRating2 = 0;
        int delta1 = 0;
        int delta2 = 0;
    };

    /*
     * Result of a 2v2 game. Index 0..3 is seat 1..4. Team 1 is seats
     * 1 and 3 (indices 0 and 2); team 2 is seats 2 and 4 (indices 1 and 3).
     * Both members of a team receive the same delta.
     */
    struct TeamUpdate {
        std::array<int, 4> newRating = {{0, 0, 0, 0}};
        std::array<int, 4> delta = {{0, 0, 0, 0}};
    };

    /* Expected score of A against B in [0, 1]. */
    static double expectedScore(double ratingA, double ratingB);

    static DuelUpdate updateDuel(int rating1, int rating2, EloOutcome outcome,
                                 int k = kDefaultK);

    static TeamUpdate updateTeams(const std::array<int, 4> &ratings,
                                  EloOutcome outcome, int k = kDefaultK);

    static int clampRating(int rating);
};

} // namespace octi

#endif // OCTI_SERVER_ELO_CALCULATOR_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
