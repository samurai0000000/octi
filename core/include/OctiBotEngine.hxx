/*
 * OctiBotEngine.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_BOT_ENGINE_HXX
#define OCTI_BOT_ENGINE_HXX

#include <cstdint>
#include <string>
#include <vector>
#include "OctiBoard.hxx"
#include "OctiDefs.hxx"
#include "OctiGame.hxx"
#include "OctiMove.hxx"

namespace octi {

enum class BotDifficulty {
    CASUAL   = 1,
    TACTICAL = 2,
    MASTER   = 3,
};

struct BotSearchStats {
    int depthReached = 0;
    std::uint64_t nodesVisited = 0;
    int bestScore = 0;
    long elapsedMs = 0;
};

class OctiBotEngine {
public:
    explicit OctiBotEngine(BotDifficulty difficulty = BotDifficulty::TACTICAL,
                           std::uint64_t rngSeed = 0x5EED2026ULL);
    ~OctiBotEngine();

    void setDifficulty(BotDifficulty d);
    BotDifficulty difficulty() const;

    void setTimeBudgetMs(int maxTimeMs);
    int timeBudgetMs() const;

    std::string chooseMove(const OctiBoard &board,
                           BotSearchStats *outStats = nullptr);

    static int evaluatePosition(const OctiBoard &board,
                                OctiPlayer perspective);

    static int defaultBudgetForDifficulty(BotDifficulty d);
    static int maxDepthForDifficulty(BotDifficulty d);
    static int rootCapForDifficulty(BotDifficulty d);
    static int branchCapForDifficulty(BotDifficulty d);

private:
    struct TranspositionEntry {
        std::uint64_t key = 0;
        int depth = -1;
        int score = 0;
        int flag = 0; // 0 = exact, 1 = lowerbound, 2 = upperbound
        std::string bestMove;
    };

    std::uint64_t nextRandom();
    std::uint64_t computePositionHash(const OctiBoard &board) const;

    int negamax(OctiBoard &scratchBoard,
                int depth,
                int ply,
                int alpha,
                int beta,
                OctiPlayer currentPerspective,
                std::uint64_t &nodesVisited,
                long deadlineMs,
                bool &timedOut);

    BotDifficulty _difficulty;
    int _timeBudgetMs;
    bool _customTimeBudget;
    std::uint64_t _rngState;
    std::vector<TranspositionEntry> _tt;
};

} // namespace octi

#endif // OCTI_BOT_ENGINE_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
