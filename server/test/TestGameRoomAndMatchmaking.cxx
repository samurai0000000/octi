/*
 * TestGameRoomAndMatchmaking.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AccountStore.hxx"
#include "EloCalculator.hxx"
#include "GameRoomManager.hxx"
#include "MatchmakingQueue.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"

#include <nlohmann/json.hpp>
#include <rapidcheck.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

using namespace octi;

namespace {

const int64_t kT0 = 1000000;            /* injected monotonic start, ms */
const int64_t kWall0 = 1800000000;      /* injected wall clock, epoch s */

/* mkdtemp scratch directory removed on destruction. */
class TempDir {
public:
    TempDir()
    {
        const char *base = std::getenv("TMPDIR");
        std::string tmpl = std::string(base && *base ? base : "/tmp") +
                           "/octi_room_test_XXXXXX";
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        if (mkdtemp(buf.data()) != nullptr) {
            _path = buf.data();
        }
    }
    ~TempDir()
    {
        if (!_path.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(_path, ec);
        }
    }
    const std::string &path() const { return _path; }

private:
    std::string _path;
};

VerifiedClaims makeClaims(const std::string &sub, const std::string &name)
{
    VerifiedClaims c;
    c.provider = "google";
    c.sub = sub;
    c.name = name;
    c.email = "user@example.com";
    c.issuer = "https://accounts.google.com";
    c.exp = 4000000000LL;
    return c;
}

#define EXPECT_CODE(expected, expr)                                          \
    do {                                                                     \
        RoomResult _res = (expr);                                            \
        STRCMP_EQUAL_TEXT(toString(RoomCode::expected), toString(_res.code), \
                          _res.message.c_str());                             \
    } while (0)

#define EXPECT_OK(expr) EXPECT_CODE(OK, expr)

/* Engine ground truth for a move list, independent of the server. */
struct EngineReplay {
    std::string fen;
    bool over = false;
    int winner = 0;
    int plies = 0;
};

EngineReplay replayOnEngine(OctiGameMode mode, bool bases1,
                            const std::vector<std::string> &moves)
{
    OctiBoard b(mode, false, false, bases1 ? GAME_BASES_1 : GAME_BASES_3);
    OctiGame *g = b.getGame();
    g->start(PLAYER_A);
    for (size_t i = 0; i < moves.size(); ++i) {
        g->move(moves[i]);
    }
    EngineReplay r;
    r.fen = b.toPositionString();
    r.over = g->isGameOver();
    r.winner = static_cast<int>(g->getWinner());
    r.plies = g->getNumMoves();
    return r;
}

/* Which seat the engine says is to move before each ply of a game. */
std::vector<int> moverSeats(OctiGameMode mode, bool bases1,
                            const std::vector<std::string> &moves)
{
    OctiBoard b(mode, false, false, bases1 ? GAME_BASES_1 : GAME_BASES_3);
    OctiGame *g = b.getGame();
    g->start(PLAYER_A);
    std::vector<int> seats;
    for (size_t i = 0; i < moves.size(); ++i) {
        seats.push_back(static_cast<int>(g->getTurn()));
        g->move(moves[i]);
    }
    return seats;
}

/* Complete games found by search and verified against the engine. */
const std::vector<std::string> kOfkWin = {"32+a",  "56+a",   "32a-33", "36+e",
                                          "33a-34", "36e-35", "34a-36"};
const std::vector<std::string> kTwoPlayerWin = {
    "73+a", "77+e", "73a-74", "77e-76", "74a-75", "57+h", "75a-77"};
const std::vector<std::string> kFourPlayerWin = {
    "33+a",   "73",      "77+e",    "73+b",    "33a-34",  "37+h",
    "77e-76", "73b-84",  "34a-35",  "37h+f-h", "76e-75",  "84b+g",
    "35a-36", "37f-26",  "75e-74",  "84bg+h",  "36a-37",  "74e-73"};

} // namespace

/* ================================================================== */
/* EloCalculator                                                       */
/* ================================================================== */

TEST_GROUP(EloCalculatorTest)
{
};

TEST(EloCalculatorTest, EqualRatingsWinnerGainsHalfK)
{
    EloCalculator::DuelUpdate u =
        EloCalculator::updateDuel(1200, 1200, EloOutcome::FIRST_WINS);
    CHECK_EQUAL(16, u.delta1);
    CHECK_EQUAL(-16, u.delta2);
    CHECK_EQUAL(1216, u.newRating1);
    CHECK_EQUAL(1184, u.newRating2);

    u = EloCalculator::updateDuel(1200, 1200, EloOutcome::SECOND_WINS);
    CHECK_EQUAL(-16, u.delta1);
    CHECK_EQUAL(16, u.delta2);
}

TEST(EloCalculatorTest, UpsetPaysMoreThanExpectedWin)
{
    /* 400 points apart: E(underdog) = 1/11. */
    EloCalculator::DuelUpdate upset =
        EloCalculator::updateDuel(1000, 1400, EloOutcome::FIRST_WINS);
    CHECK_EQUAL(29, upset.delta1);
    CHECK_EQUAL(-29, upset.delta2);

    EloCalculator::DuelUpdate expected =
        EloCalculator::updateDuel(1400, 1000, EloOutcome::FIRST_WINS);
    CHECK_EQUAL(3, expected.delta1);
    CHECK_EQUAL(-3, expected.delta2);
}

TEST(EloCalculatorTest, DrawMovesRatingsTowardEachOther)
{
    EloCalculator::DuelUpdate eq =
        EloCalculator::updateDuel(1300, 1300, EloOutcome::DRAW);
    CHECK_EQUAL(0, eq.delta1);
    CHECK_EQUAL(0, eq.delta2);

    EloCalculator::DuelUpdate d =
        EloCalculator::updateDuel(1400, 1000, EloOutcome::DRAW);
    CHECK_EQUAL(-13, d.delta1);
    CHECK_EQUAL(13, d.delta2);
}

TEST(EloCalculatorTest, ZeroKChangesNothingAndKIsBounded)
{
    EloCalculator::DuelUpdate z =
        EloCalculator::updateDuel(1200, 1200, EloOutcome::FIRST_WINS, 0);
    CHECK_EQUAL(0, z.delta1);
    EloCalculator::DuelUpdate neg =
        EloCalculator::updateDuel(1200, 1200, EloOutcome::FIRST_WINS, -50);
    CHECK_EQUAL(0, neg.delta1);
    EloCalculator::DuelUpdate huge =
        EloCalculator::updateDuel(1200, 1200, EloOutcome::FIRST_WINS, 1000000);
    CHECK_EQUAL(EloCalculator::kMaxK / 2, huge.delta1);
}

TEST(EloCalculatorTest, ResultsStayInsideStoreRangeAndStayZeroSum)
{
    EloCalculator::DuelUpdate top = EloCalculator::updateDuel(
        EloCalculator::kMaxRating, EloCalculator::kMaxRating - 10,
        EloOutcome::FIRST_WINS);
    CHECK_EQUAL(0, top.delta1);
    CHECK_EQUAL(EloCalculator::kMaxRating, top.newRating1);

    /* Both at 5: the loser can only fall 5 points, the winner gains 5. */
    EloCalculator::DuelUpdate low =
        EloCalculator::updateDuel(5, 5, EloOutcome::SECOND_WINS);
    CHECK_EQUAL(-5, low.delta1);
    CHECK_EQUAL(5, low.delta2);
    CHECK_EQUAL(0, low.newRating1);
    CHECK_EQUAL(10, low.newRating2);

    /* Out-of-range inputs are clamped first. */
    EloCalculator::DuelUpdate wild =
        EloCalculator::updateDuel(-500, 99999, EloOutcome::DRAW);
    CHECK_TRUE(wild.newRating1 >= EloCalculator::kMinRating);
    CHECK_TRUE(wild.newRating2 <= EloCalculator::kMaxRating);
    CHECK_EQUAL(0, wild.delta1 + wild.delta2);
}

TEST(EloCalculatorTest, ExpectedScoreIsComplementary)
{
    DOUBLES_EQUAL(0.5, EloCalculator::expectedScore(1200, 1200), 1e-12);
    DOUBLES_EQUAL(1.0, EloCalculator::expectedScore(1500, 900) +
                           EloCalculator::expectedScore(900, 1500), 1e-12);
    DOUBLES_EQUAL(1.0 / 11.0, EloCalculator::expectedScore(1000, 1400), 1e-12);
}

TEST(EloCalculatorTest, TeamUpdateGivesBothMembersTheSameDelta)
{
    std::array<int, 4> r = {{1200, 1200, 1200, 1200}};
    EloCalculator::TeamUpdate u =
        EloCalculator::updateTeams(r, EloOutcome::FIRST_WINS);
    CHECK_EQUAL(16, u.delta[0]);
    CHECK_EQUAL(-16, u.delta[1]);
    CHECK_EQUAL(16, u.delta[2]);
    CHECK_EQUAL(-16, u.delta[3]);
    CHECK_EQUAL(1216, u.newRating[0]);
    CHECK_EQUAL(1184, u.newRating[3]);
}

TEST(EloCalculatorTest, TeamUpdateUsesTeamAverages)
{
    /* team 1 = seats 1+3 average 900, team 2 = seats 2+4 average 1400. */
    std::array<int, 4> r = {{800, 1400, 1000, 1400}};
    EloCalculator::TeamUpdate u =
        EloCalculator::updateTeams(r, EloOutcome::FIRST_WINS);
    CHECK_EQUAL(30, u.delta[0]);
    CHECK_EQUAL(30, u.delta[2]);
    CHECK_EQUAL(-30, u.delta[1]);
    CHECK_EQUAL(-30, u.delta[3]);
    CHECK_EQUAL(830, u.newRating[0]);
    CHECK_EQUAL(1030, u.newRating[2]);

    EloCalculator::TeamUpdate d =
        EloCalculator::updateTeams(r, EloOutcome::DRAW);
    CHECK_TRUE(d.delta[0] > 0);
    CHECK_EQUAL(d.delta[0], d.delta[2]);
    CHECK_EQUAL(-d.delta[0], d.delta[1]);
}

TEST(EloCalculatorTest, TeamUpdateIsClampedByTheWeakestMember)
{
    std::array<int, 4> r = {{9999, 3, 9999, 3}};
    EloCalculator::TeamUpdate u =
        EloCalculator::updateTeams(r, EloOutcome::SECOND_WINS);
    int sum = 0;
    for (size_t i = 0; i < 4; ++i) {
        sum += u.delta[i];
        CHECK_TRUE(u.newRating[i] >= EloCalculator::kMinRating);
        CHECK_TRUE(u.newRating[i] <= EloCalculator::kMaxRating);
    }
    CHECK_EQUAL(0, sum);
}

TEST(EloCalculatorTest, RapidCheckDuelIsZeroSumAndBounded)
{
    bool passed = rc::check("duel update is zero-sum and in range", []() {
        int r1 = *rc::gen::inRange(-200, 10300);
        int r2 = *rc::gen::inRange(-200, 10300);
        int k = *rc::gen::inRange(-5, 1200);
        int o = *rc::gen::inRange(0, 3);
        EloOutcome oc = o == 0   ? EloOutcome::FIRST_WINS
                        : o == 1 ? EloOutcome::SECOND_WINS
                                 : EloOutcome::DRAW;
        EloCalculator::DuelUpdate u = EloCalculator::updateDuel(r1, r2, oc, k);
        RC_ASSERT(u.delta1 + u.delta2 == 0);
        RC_ASSERT(u.newRating1 == EloCalculator::clampRating(r1) + u.delta1);
        RC_ASSERT(u.newRating2 == EloCalculator::clampRating(r2) + u.delta2);
        RC_ASSERT(u.newRating1 >= EloCalculator::kMinRating &&
                  u.newRating1 <= EloCalculator::kMaxRating);
        RC_ASSERT(u.newRating2 >= EloCalculator::kMinRating &&
                  u.newRating2 <= EloCalculator::kMaxRating);
        /* the winner never loses points, the loser never gains any */
        if (oc == EloOutcome::FIRST_WINS) {
            RC_ASSERT(u.delta1 >= 0);
        } else if (oc == EloOutcome::SECOND_WINS) {
            RC_ASSERT(u.delta1 <= 0);
        }
    });
    CHECK_TRUE(passed);
}

TEST(EloCalculatorTest, RapidCheckTeamDeltasSumToZero)
{
    bool passed = rc::check("team update is zero-sum and in range", []() {
        std::array<int, 4> r;
        for (size_t i = 0; i < r.size(); ++i) {
            r[i] = *rc::gen::inRange(-200, 10300);
        }
        int k = *rc::gen::inRange(0, 1200);
        int o = *rc::gen::inRange(0, 3);
        EloOutcome oc = o == 0   ? EloOutcome::FIRST_WINS
                        : o == 1 ? EloOutcome::SECOND_WINS
                                 : EloOutcome::DRAW;
        EloCalculator::TeamUpdate u = EloCalculator::updateTeams(r, oc, k);
        RC_ASSERT(u.delta[0] + u.delta[1] + u.delta[2] + u.delta[3] == 0);
        RC_ASSERT(u.delta[0] == u.delta[2]);
        RC_ASSERT(u.delta[1] == u.delta[3]);
        RC_ASSERT(u.delta[0] == -u.delta[1]);
        for (size_t i = 0; i < r.size(); ++i) {
            RC_ASSERT(u.newRating[i] ==
                      EloCalculator::clampRating(r[i]) + u.delta[i]);
            RC_ASSERT(u.newRating[i] >= EloCalculator::kMinRating &&
                      u.newRating[i] <= EloCalculator::kMaxRating);
        }
    });
    CHECK_TRUE(passed);
}

/* ================================================================== */
/* MatchmakingQueue                                                    */
/* ================================================================== */

TEST_GROUP(MatchmakingQueueTest)
{
    int64_t now;
    MatchmakingQueue *q;

    void setup()
    {
        now = kT0;
        q = new MatchmakingQueue();
        q->setClock([this]() -> int64_t { return now; });
    }

    void teardown()
    {
        delete q;
        q = nullptr;
    }

    static MatchRuleset rules(OctiGameMode mode = GAME_MODE_2)
    {
        MatchRuleset r;
        r.mode = mode;
        return r;
    }
};

TEST(MatchmakingQueueTest, PairsPlayersInsideTheWindow)
{
    CHECK(q->enqueue(1, 1200, rules()) == EnqueueStatus::OK);
    CHECK(q->enqueue(2, 1290, rules()) == EnqueueStatus::OK);
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    CHECK_EQUAL(2u, g[0].players.size());
    CHECK_EQUAL(0u, q->size());
    CHECK_FALSE(q->isQueued(1));
    CHECK_FALSE(q->isQueued(2));
}

TEST(MatchmakingQueueTest, WindowBoundaryIsInclusive)
{
    q->enqueue(1, 1200, rules());
    q->enqueue(2, 1301, rules());
    CHECK_EQUAL(0u, q->findMatches().size());     /* 101 > 100 */
    CHECK_EQUAL(2u, q->size());
    q->dequeue(2);
    q->enqueue(3, 1300, rules());
    CHECK_EQUAL(1u, q->findMatches().size());     /* 100 <= 100 */
}

TEST(MatchmakingQueueTest, WindowWidensWithWaitTime)
{
    q->enqueue(1, 1000, rules());
    q->enqueue(2, 1300, rules());
    CHECK_EQUAL(100, q->windowForWait(0));
    CHECK_EQUAL(100, q->windowForWait(9999));
    CHECK_EQUAL(150, q->windowForWait(10000));
    CHECK_EQUAL(300, q->windowForWait(40000));
    now = kT0 + 39999;
    CHECK_EQUAL(0u, q->findMatches().size());
    now = kT0 + 40000;                          /* window 300 == distance */
    CHECK_EQUAL(1u, q->findMatches().size());
}

TEST(MatchmakingQueueTest, WindowIsCappedAndNegativeWaitCountsAsZero)
{
    CHECK_EQUAL(1000, q->windowForWait(100LL * 3600 * 1000));
    CHECK_EQUAL(1000, q->windowForWait(INT64_MAX));
    CHECK_EQUAL(100, q->windowForWait(-5));
    MatchmakingConfig c;
    c.baseWindow = 10;
    c.windowStep = 5;
    c.stepIntervalMs = 1000;
    c.maxWindow = 20;
    q->setConfig(c);
    CHECK_EQUAL(15, q->windowForWait(1000));
    CHECK_EQUAL(20, q->windowForWait(60000));
    /* nonsense configuration is repaired, not trusted */
    MatchmakingConfig bad;
    bad.baseWindow = -4;
    bad.windowStep = -1;
    bad.stepIntervalMs = 0;
    bad.maxWindow = -9;
    q->setConfig(bad);
    CHECK_EQUAL(0, q->windowForWait(123456));
}

TEST(MatchmakingQueueTest, NarrowerOfTheTwoWindowsGoverns)
{
    q->enqueue(1, 1000, rules());
    now = kT0 + 60000;                          /* player 1 window = 400 */
    q->enqueue(2, 1300, rules());               /* player 2 window = 100 */
    CHECK_EQUAL(0u, q->findMatches().size());
    now = kT0 + 60000 + 40000;                  /* player 2 window = 300 */
    CHECK_EQUAL(1u, q->findMatches().size());
}

TEST(MatchmakingQueueTest, DifferentRulesetsNeverMeet)
{
    MatchRuleset base = rules();
    MatchRuleset oneBase = rules();
    oneBase.bases = GAME_BASES_1;
    MatchRuleset super = rules();
    super.useSuperProngs = true;
    MatchRuleset edgeless = rules();
    edgeless.useEdgelessBoard = true;
    MatchRuleset timed = rules();
    timed.initialMs = 60000;
    timed.incrementMs = 1000;
    MatchRuleset timed2 = timed;
    timed2.incrementMs = 2000;
    MatchRuleset ofk = rules(GAME_MODE_OFK);

    const MatchRuleset all[] = {base, oneBase, super, edgeless, timed, timed2, ofk};
    for (int i = 0; i < 7; ++i) {
        CHECK(q->enqueue(100 + i, 1200, all[i]) == EnqueueStatus::OK);
    }
    CHECK_EQUAL(0u, q->findMatches().size());
    CHECK_EQUAL(7u, q->size());
    /* a second player in exactly one bucket pairs only inside it */
    q->enqueue(200, 1200, timed2);
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    CHECK(g[0].ruleset == timed2);
    CHECK_EQUAL(6u, q->size());
}

TEST(MatchmakingQueueTest, OptionsTheEngineIgnoresShareABucket)
{
    MatchRuleset a = rules(GAME_MODE_OFK);
    a.bases = GAME_BASES_3;
    a.useSuperProngs = true;
    MatchRuleset b = rules(GAME_MODE_OFK);
    b.bases = GAME_BASES_1;
    q->enqueue(1, 1200, a);
    q->enqueue(2, 1200, b);
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    CHECK(g[0].ruleset.bases == GAME_BASES_1);
    CHECK_FALSE(g[0].ruleset.useSuperProngs);

    /* an untimed request drops a stray increment */
    MatchRuleset c = rules();
    c.incrementMs = 5000;
    MatchRuleset d = rules();
    q->enqueue(3, 1200, c);
    q->enqueue(4, 1200, d);
    CHECK_EQUAL(1u, q->findMatches().size());
}

TEST(MatchmakingQueueTest, RejectsDuplicateInvalidAndOverflowingRequests)
{
    CHECK(q->enqueue(1, 1200, rules()) == EnqueueStatus::OK);
    CHECK(q->enqueue(1, 1300, rules()) == EnqueueStatus::ALREADY_QUEUED);
    CHECK(q->enqueue(0, 1200, rules()) == EnqueueStatus::INVALID);
    CHECK(q->enqueue(-3, 1200, rules()) == EnqueueStatus::INVALID);
    CHECK(q->enqueue(2, -1, rules()) == EnqueueStatus::INVALID);
    MatchRuleset bad = rules();
    bad.mode = GAME_MODE_UNDEF;
    CHECK(q->enqueue(2, 1200, bad) == EnqueueStatus::INVALID);
    MatchRuleset badClock = rules();
    badClock.initialMs = 5LL * 3600 * 1000;
    CHECK(q->enqueue(2, 1200, badClock) == EnqueueStatus::INVALID);
    CHECK_FALSE(q->dequeue(99));
    CHECK_TRUE(q->dequeue(1));
    CHECK_FALSE(q->dequeue(1));
    CHECK_EQUAL(0u, q->size());

    for (size_t i = 1; i <= MatchmakingQueue::kMaxQueueSize; ++i) {
        CHECK(q->enqueue(static_cast<int64_t>(i), 1000000, rules()) ==
              EnqueueStatus::OK);
    }
    CHECK(q->enqueue(999999, 1200, rules()) == EnqueueStatus::QUEUE_FULL);
}

TEST(MatchmakingQueueTest, QueueInfoReportsWaitAndWindow)
{
    MatchRuleset t = rules();
    t.initialMs = 30000;
    q->enqueue(7, 1234, t);
    now = kT0 + 25000;
    QueueInfo qi;
    CHECK_TRUE(q->queueInfo(7, qi));
    CHECK_EQUAL(1234, qi.rating);
    CHECK_EQUAL(25000, qi.waitedMs);
    CHECK_EQUAL(200, qi.window);
    CHECK(qi.ruleset == t);
    CHECK_FALSE(q->queueInfo(8, qi));
}

TEST(MatchmakingQueueTest, OldestTicketAnchorsAndClosestRatingWins)
{
    q->enqueue(1, 1200, rules());
    now = kT0 + 1;
    q->enqueue(2, 1500, rules());
    now = kT0 + 2;
    q->enqueue(3, 1210, rules());
    now = kT0 + 3;
    q->enqueue(4, 1150, rules());
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    /* player 1 anchors; 3 (diff 10) beats 4 (diff 50); 2 is out of window */
    CHECK_EQUAL(1, g[0].players[0].playerId);
    CHECK_EQUAL(3, g[0].players[1].playerId);
    CHECK_EQUAL(2u, q->size());
    CHECK_TRUE(q->isQueued(2));
    CHECK_TRUE(q->isQueued(4));
}

TEST(MatchmakingQueueTest, LongerWaiterTakesTheFirstMoverSeat)
{
    q->enqueue(5, 1200, rules());
    now = kT0 + 1000;
    q->enqueue(6, 1200, rules());
    now = kT0 + 4000;
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    CHECK_EQUAL(5, g[0].players[0].playerId);
    CHECK_EQUAL(4000, g[0].players[0].waitedMs);
    CHECK_EQUAL(6, g[0].players[1].playerId);
    CHECK_EQUAL(3000, g[0].players[1].waitedMs);
}

TEST(MatchmakingQueueTest, FourPlayerGroupBalancesTheTeams)
{
    MatchRuleset r4 = rules(GAME_MODE_4);
    q->enqueue(1, 1000, r4);
    q->enqueue(2, 1100, r4);
    q->enqueue(3, 1050, r4);
    q->enqueue(4, 1020, r4);
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    CHECK_EQUAL(4u, g[0].players.size());
    /* seats 1+3 = strongest + weakest; seats 2+4 = the two in between */
    CHECK_EQUAL(2, g[0].players[0].playerId);   /* 1100 */
    CHECK_EQUAL(3, g[0].players[1].playerId);   /* 1050 */
    CHECK_EQUAL(1, g[0].players[2].playerId);   /* 1000 */
    CHECK_EQUAL(4, g[0].players[3].playerId);   /* 1020 */
    CHECK_EQUAL(0u, q->size());
}

TEST(MatchmakingQueueTest, FourPlayerNeedsFourCompatiblePlayers)
{
    MatchRuleset r4 = rules(GAME_MODE_4);
    q->enqueue(1, 1000, r4);
    q->enqueue(2, 1010, r4);
    q->enqueue(3, 1020, r4);
    CHECK_EQUAL(0u, q->findMatches().size());
    q->enqueue(4, 1600, r4);                    /* far outside every window */
    CHECK_EQUAL(0u, q->findMatches().size());
    CHECK_EQUAL(4u, q->size());
    q->enqueue(5, 1030, r4);
    std::vector<MatchGroup> g = q->findMatches();
    CHECK_EQUAL(1u, g.size());
    for (size_t i = 0; i < g[0].players.size(); ++i) {
        CHECK_TRUE(g[0].players[i].playerId != 4);
    }
    CHECK_TRUE(q->isQueued(4));
}

TEST(MatchmakingQueueTest, ResultIsDeterministicForTheSameArrivals)
{
    std::vector<std::vector<int64_t>> runs;
    for (int run = 0; run < 2; ++run) {
        MatchmakingQueue local;
        int64_t t = kT0;
        local.setClock([&t]() -> int64_t { return t; });
        const int ratings[] = {1200, 1250, 1180, 1500, 1490, 1210, 1300};
        for (int i = 0; i < 7; ++i) {
            local.enqueue(10 + i, ratings[i], rules());
            t += 7;
        }
        std::vector<int64_t> flat;
        std::vector<MatchGroup> g = local.findMatches();
        for (size_t i = 0; i < g.size(); ++i) {
            for (size_t k = 0; k < g[i].players.size(); ++k) {
                flat.push_back(g[i].players[k].playerId);
            }
            flat.push_back(-1);
        }
        runs.push_back(flat);
    }
    CHECK_TRUE(runs[0] == runs[1]);
    CHECK_TRUE(runs[0].size() > 2);
}

TEST(MatchmakingQueueTest, ClockInjectionDoesNotTouchTheRealClock)
{
    /* The queue only ever reads the injected clock: moving it backwards must
     * not crash or underflow a wait. */
    q->enqueue(1, 1200, rules());
    now = kT0 - 5000;
    QueueInfo qi;
    CHECK_TRUE(q->queueInfo(1, qi));
    CHECK_EQUAL(0, qi.waitedMs);
    CHECK_EQUAL(100, qi.window);
}

TEST(MatchmakingQueueTest, RapidCheckQueueInvariants)
{
    bool passed = rc::check("matchmaking queue invariants", []() {
        MatchmakingQueue local;
        int64_t t = kT0;
        local.setClock([&t]() -> int64_t { return t; });

        struct Ticket {
            int rating;
            MatchRuleset rules;
            int64_t at;
        };
        std::map<int64_t, Ticket> queued;       /* model of waiting players */
        std::set<int64_t> everMatched;

        std::vector<MatchRuleset> pool;
        for (int m = 0; m < 3; ++m) {
            MatchRuleset r;
            r.mode = m == 0 ? GAME_MODE_2 : m == 1 ? GAME_MODE_4 : GAME_MODE_OFK;
            pool.push_back(r.normalized());
            MatchRuleset t2 = r;
            t2.initialMs = 60000;
            t2.incrementMs = 2000;
            pool.push_back(t2.normalized());
        }

        int steps = *rc::gen::inRange(1, 60);
        for (int s = 0; s < steps; ++s) {
            int op = *rc::gen::inRange(0, 10);
            if (op < 5) {
                int64_t id = *rc::gen::inRange(1, 25);
                int rating = *rc::gen::inRange(800, 1800);
                const MatchRuleset &rs =
                    pool[static_cast<size_t>(*rc::gen::inRange(0, 6))];
                EnqueueStatus st = local.enqueue(id, rating, rs);
                if (queued.count(id) != 0 || everMatched.count(id) != 0) {
                    if (queued.count(id) != 0) {
                        RC_ASSERT(st == EnqueueStatus::ALREADY_QUEUED);
                    } else {
                        /* a matched player left the queue and may re-enter */
                        RC_ASSERT(st == EnqueueStatus::OK);
                        everMatched.erase(id);
                        Ticket tk;
                        tk.rating = rating;
                        tk.rules = rs;
                        tk.at = t;
                        queued[id] = tk;
                    }
                } else {
                    RC_ASSERT(st == EnqueueStatus::OK);
                    Ticket tk;
                    tk.rating = rating;
                    tk.rules = rs;
                    tk.at = t;
                    queued[id] = tk;
                }
            } else if (op < 6) {
                int64_t id = *rc::gen::inRange(1, 25);
                bool removed = local.dequeue(id);
                RC_ASSERT(removed == (queued.count(id) != 0));
                queued.erase(id);
            } else if (op < 8) {
                t += *rc::gen::inRange(0, 25000);
            } else {
                std::vector<MatchGroup> groups = local.findMatches();
                std::set<int64_t> inThisRound;
                for (size_t gi = 0; gi < groups.size(); ++gi) {
                    const MatchGroup &g = groups[gi];
                    RC_ASSERT(static_cast<int>(g.players.size()) ==
                              g.ruleset.playersRequired());
                    for (size_t a = 0; a < g.players.size(); ++a) {
                        const MatchedPlayer &mp = g.players[a];
                        /* matched exactly once, and only if it was waiting */
                        RC_ASSERT(inThisRound.insert(mp.playerId).second);
                        RC_ASSERT(queued.count(mp.playerId) == 1);
                        const Ticket &tk = queued[mp.playerId];
                        RC_ASSERT(tk.rules == g.ruleset);
                        RC_ASSERT(tk.rating == mp.rating);
                        RC_ASSERT(mp.waitedMs == t - tk.at);
                        for (size_t b = a + 1; b < g.players.size(); ++b) {
                            const MatchedPlayer &o = g.players[b];
                            int window = std::min(
                                local.windowForWait(mp.waitedMs),
                                local.windowForWait(o.waitedMs));
                            RC_ASSERT(std::abs(mp.rating - o.rating) <= window);
                        }
                    }
                }
                for (std::set<int64_t>::const_iterator it = inThisRound.begin();
                     it != inThisRound.end(); ++it) {
                    queued.erase(*it);
                    everMatched.insert(*it);
                    RC_ASSERT(!local.isQueued(*it));
                }
            }
            RC_ASSERT(local.size() == queued.size());
            for (std::map<int64_t, Ticket>::const_iterator it = queued.begin();
                 it != queued.end(); ++it) {
                RC_ASSERT(local.isQueued(it->first));
            }
        }
    });
    CHECK_TRUE(passed);
}

/* ================================================================== */
/* GameRoomManager                                                     */
/* ================================================================== */

TEST_GROUP(GameRoomManagerTest)
{
    TempDir *dir;
    AccountStore *store;
    GameRoomManager *mgr;
    int64_t now;
    int64_t wall;
    std::vector<int64_t> p;
    std::vector<std::string> handle;

    void setup()
    {
        now = kT0;
        wall = kWall0;
        dir = new TempDir();
        store = new AccountStore();
        store->setClock([this]() -> int64_t { return wall; });
        std::string err;
        CHECK_TRUE_TEXT(!dir->path().empty(), "mkdtemp failed");
        CHECK_TRUE_TEXT(store->openOrCreate(dir->path() + "/data/accounts.db", err),
                        err.c_str());
        const char *names[] = {"alice", "bob",   "carol", "dave",
                               "erin",  "frank", "grace", "heidi"};
        for (int i = 0; i < 8; ++i) {
            LoginResult r = store->loginWithSso(
                makeClaims(std::string("sub-") + names[i], names[i]), "test");
            CHECK_TRUE_TEXT(r.ok, r.error.c_str());
            p.push_back(r.profile.playerId);
            handle.push_back(r.profile.handle);
        }
        mgr = new GameRoomManager(*store);
        mgr->setClock([this]() -> int64_t { return now; });
        mgr->setWallClock([this]() -> int64_t { return wall; });
    }

    void teardown()
    {
        delete mgr;
        mgr = nullptr;
        delete store;
        store = nullptr;
        delete dir;
        dir = nullptr;
    }

    GameTableConfig cfg(OctiGameMode mode = GAME_MODE_2)
    {
        GameTableConfig c;
        c.mode = mode;
        return c;
    }

    GameTableConfig timedCfg(int64_t initialMs, int64_t incrementMs,
                             OctiGameMode mode = GAME_MODE_2)
    {
        GameTableConfig c = cfg(mode);
        c.timeControl.enabled = true;
        c.timeControl.initialMs = initialMs;
        c.timeControl.incrementMs = incrementMs;
        return c;
    }

    /* Host p[0] opens the table, p[1..players-1] take the next free seats. */
    std::string startTable(const GameTableConfig &c, int players)
    {
        RoomResult r = mgr->createTable(p[0], c);
        CHECK_TRUE_TEXT(r.ok(), r.message.c_str());
        for (int i = 1; i < players; ++i) {
            RoomResult j = mgr->joinTable(r.roomId, p[static_cast<size_t>(i)], 0,
                                          c.passcode);
            CHECK_TRUE_TEXT(j.ok(), j.message.c_str());
        }
        return r.roomId;
    }

    std::string start2P() { return startTable(cfg(GAME_MODE_2), 2); }
    std::string startOfk() { return startTable(cfg(GAME_MODE_OFK), 2); }
    std::string start4P() { return startTable(cfg(GAME_MODE_4), 4); }

    RoomView view(const std::string &room, int64_t viewer = 0)
    {
        RoomView v;
        RoomResult r = mgr->getRoomView(room, viewer, v);
        CHECK_TRUE_TEXT(r.ok(), r.message.c_str());
        return v;
    }

    std::vector<RoomEvent> events(const std::string &room, int64_t viewer,
                                  int64_t since = 0)
    {
        RoomPoll poll;
        RoomResult r = mgr->pollRoomEvents(room, viewer, since, 500, poll);
        CHECK_TRUE_TEXT(r.ok(), r.message.c_str());
        return poll.events;
    }

    static size_t countType(const std::vector<RoomEvent> &ev, RoomEventType t)
    {
        size_t n = 0;
        for (size_t i = 0; i < ev.size(); ++i) {
            if (ev[i].type == t) {
                ++n;
            }
        }
        return n;
    }

    /* Seat s is played by p[s-1] (see startTable). */
    RoomResult play(const std::string &room, const std::vector<std::string> &moves,
                    int seats)
    {
        RoomResult last;
        for (size_t i = 0; i < moves.size(); ++i) {
            last = mgr->submitMove(room, p[i % static_cast<size_t>(seats)], moves[i]);
            CHECK_TRUE_TEXT(last.ok(),
                            (moves[i] + ": " + last.message).c_str());
        }
        return last;
    }

    /* Plays moves[i] as seat seats[i] (seat s is p[s-1]). */
    RoomResult playAs(const std::string &room, const std::vector<std::string> &moves,
                      const std::vector<int> &seats)
    {
        RoomResult last;
        for (size_t i = 0; i < moves.size(); ++i) {
            last = mgr->submitMove(room, p[static_cast<size_t>(seats[i] - 1)], moves[i]);
            CHECK_TRUE_TEXT(last.ok(), (moves[i] + ": " + last.message).c_str());
        }
        return last;
    }

    PlayerProfile profile(int64_t id)
    {
        PlayerProfile pr;
        CHECK_TRUE(store->getProfileById(id, pr));
        return pr;
    }

    void setRating2p(int64_t id, int elo)
    {
        std::vector<RatingUpdate> u(1);
        u[0].playerId = id;
        u[0].newElo = elo;
        u[0].outcome = GameOutcome::DRAW;
        std::string err;
        CHECK_TRUE_TEXT(store->applyRatingUpdates("2P", u, err), err.c_str());
    }

    GameRecord archived(const std::string &room)
    {
        GameRecord rec;
        CHECK_TRUE_TEXT(store->getArchivedGame(room, rec), room.c_str());
        return rec;
    }

    static bool startsWith(const std::string &s, const std::string &prefix)
    {
        return s.compare(0, prefix.size(), prefix) == 0;
    }

    std::vector<std::string> presenceTrail(int64_t id)
    {
        std::vector<std::string> trail;
        LobbyPoll lp = mgr->pollLobbyEvents(0, 0, 500);
        for (size_t i = 0; i < lp.events.size(); ++i) {
            if (lp.events[i].type == LobbyEventType::PRESENCE &&
                lp.events[i].playerId == id) {
                trail.push_back(toString(lp.events[i].status));
            }
        }
        return trail;
    }

    /* Replays a finished room's transcript through the engine. */
    void checkTranscriptReplays(const std::string &room, const GameRecord &rec)
    {
        OctiBoard rb;
        rb.deserializeGame(rec.octiTranscript);
        STRCMP_EQUAL(rec.finalFen.c_str(), rb.toPositionString().c_str());
        CHECK_EQUAL(rec.plyCount, rb.getGame()->getNumMoves());
        std::string live;
        EXPECT_OK(mgr->getTranscript(room, 0, live));
        STRCMP_EQUAL(rec.octiTranscript.c_str(), live.c_str());
    }
};

/* ---- lobby presence & chat ------------------------------------------- */

TEST(GameRoomManagerTest, LobbyPresenceFollowsTheTableLifecycle)
{
    EXPECT_OK(mgr->enterLobby(p[0]));
    EXPECT_OK(mgr->enterLobby(p[1]));
    EXPECT_OK(mgr->enterLobby(p[2]));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[0])));
    STRCMP_EQUAL("OFFLINE", toString(mgr->presenceOf(p[5])));

    RoomResult c = mgr->createTable(p[0], cfg());
    EXPECT_OK(c);
    STRCMP_EQUAL("AT_TABLE", toString(mgr->presenceOf(p[0])));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[1])));

    RoomResult j = mgr->joinTable(c.roomId, p[1], 0, "");
    EXPECT_OK(j);
    CHECK_TRUE(j.gameStarted);
    STRCMP_EQUAL("PLAYING", toString(mgr->presenceOf(p[0])));
    STRCMP_EQUAL("PLAYING", toString(mgr->presenceOf(p[1])));

    EXPECT_OK(mgr->observeRoom(c.roomId, p[2]));
    STRCMP_EQUAL("OBSERVING", toString(mgr->presenceOf(p[2])));

    LobbySnapshot snap = mgr->lobbyState();
    CHECK_EQUAL(3u, snap.players.size());
    int playing = 0;
    for (size_t i = 0; i < snap.players.size(); ++i) {
        if (snap.players[i].status == PresenceStatus::PLAYING) {
            ++playing;
            STRCMP_EQUAL(c.roomId.c_str(), snap.players[i].roomId.c_str());
        }
    }
    CHECK_EQUAL(2, playing);
    CHECK_EQUAL(1u, snap.tables.size());
    CHECK_EQUAL(2, snap.tables[0].seatsFilled);

    EXPECT_OK(mgr->resign(c.roomId, p[1]));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[0])));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[1])));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[2])));
    CHECK_EQUAL(0u, mgr->lobbyState().tables.size());

    std::vector<std::string> trail = presenceTrail(p[0]);
    CHECK_EQUAL(4u, trail.size());
    STRCMP_EQUAL("IN_LOBBY", trail[0].c_str());
    STRCMP_EQUAL("AT_TABLE", trail[1].c_str());
    STRCMP_EQUAL("PLAYING", trail[2].c_str());
    STRCMP_EQUAL("IN_LOBBY", trail[3].c_str());

    EXPECT_OK(mgr->stopObserving(c.roomId, p[2]));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->enterLobby(987654));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->enterLobby(0));
}

TEST(GameRoomManagerTest, LeavingTheLobbyDropsPresenceQueueAndWaitingSeat)
{
    EXPECT_OK(mgr->enterLobby(p[0]));
    EXPECT_OK(mgr->enterLobby(p[1]));
    EXPECT_OK(mgr->enqueueMatchmaking(p[1], MatchRuleset()));
    RoomResult c = mgr->createTable(p[0], cfg());
    EXPECT_OK(c);

    EXPECT_OK(mgr->leaveLobby(p[1]));
    STRCMP_EQUAL("OFFLINE", toString(mgr->presenceOf(p[1])));
    CHECK_FALSE(mgr->matchmakingStatus(p[1]).queued);
    CHECK_EQUAL(1u, mgr->lobbyState().players.size());

    /* the only seated player leaving a waiting table closes it */
    EXPECT_OK(mgr->leaveLobby(p[0]));
    CHECK_EQUAL(0u, mgr->roomCount());
    RoomView v;
    EXPECT_CODE(NOT_FOUND, mgr->getRoomView(c.roomId, 0, v));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->leaveLobby(424242));
}

TEST(GameRoomManagerTest, LeavingTheLobbyMidGameMarksTheSeatDisconnected)
{
    std::string room = start2P();
    EXPECT_OK(mgr->enterLobby(p[1]));
    EXPECT_OK(mgr->leaveLobby(p[1]));
    RoomView v = view(room, p[0]);
    CHECK_FALSE(v.seats[1].connected);
    CHECK_TRUE(v.seats[0].connected);
    CHECK_EQUAL(1u, countType(events(room, p[0]), RoomEventType::PLAYER_DISCONNECTED));
    /* coming back reconnects the seat */
    EXPECT_OK(mgr->enterLobby(p[1]));
    CHECK_TRUE(view(room, p[0]).seats[1].connected);
    CHECK_EQUAL(1u, countType(events(room, p[0]), RoomEventType::PLAYER_RECONNECTED));
    STRCMP_EQUAL("PLAYING", toString(mgr->presenceOf(p[1])));
}

TEST(GameRoomManagerTest, LobbyChatIsDeliveredInOrderAndPolledIncrementally)
{
    RoomResult a = mgr->postLobbyChat(p[0], "  hello lobby  ");
    EXPECT_OK(a);
    now += 10;
    RoomResult b = mgr->postLobbyChat(p[1], "second");
    EXPECT_OK(b);
    CHECK_TRUE(b.seq > a.seq);

    LobbyPoll all = mgr->pollLobbyEvents(0, 0, 500);
    int chats = 0;
    for (size_t i = 0; i < all.events.size(); ++i) {
        if (all.events[i].type == LobbyEventType::LOBBY_CHAT) {
            ++chats;
        }
    }
    CHECK_EQUAL(2, chats);
    LobbySnapshot snap = mgr->lobbyState();
    CHECK_EQUAL(2u, snap.recentChat.size());
    STRCMP_EQUAL("hello lobby", snap.recentChat[0].text.c_str());
    STRCMP_EQUAL(handle[0].c_str(), snap.recentChat[0].handle.c_str());

    LobbyPoll since = mgr->pollLobbyEvents(0, a.seq, 500);
    int chatsAfterA = 0;
    for (size_t i = 0; i < since.events.size(); ++i) {
        CHECK_TRUE(since.events[i].seq > a.seq);
        if (since.events[i].type == LobbyEventType::LOBBY_CHAT) {
            STRCMP_EQUAL("second", since.events[i].text.c_str());
            ++chatsAfterA;
        }
    }
    CHECK_EQUAL(1, chatsAfterA);
    LobbyPoll none = mgr->pollLobbyEvents(0, all.latestSeq, 500);
    CHECK_EQUAL(0u, none.events.size());
    CHECK_FALSE(none.hasMore);

    LobbyPoll limited = mgr->pollLobbyEvents(0, 0, 1);
    CHECK_EQUAL(1u, limited.events.size());
    CHECK_TRUE(limited.hasMore);
}

TEST(GameRoomManagerTest, LobbyChatRateLimitBoundary)
{
    for (int i = 0; i < 5; ++i) {
        EXPECT_OK(mgr->postLobbyChat(p[0], "msg " + std::to_string(i)));
    }
    EXPECT_CODE(RATE_LIMITED, mgr->postLobbyChat(p[0], "sixth"));
    now += 9999;
    EXPECT_CODE(RATE_LIMITED, mgr->postLobbyChat(p[0], "still sixth"));
    /* another player has an independent budget */
    EXPECT_OK(mgr->postLobbyChat(p[1], "unaffected"));
    now += 1;                                   /* exactly 10 s after the burst */
    EXPECT_OK(mgr->postLobbyChat(p[0], "window reopened"));
    /* a rejected message was not recorded and consumes no budget */
    for (int i = 0; i < 4; ++i) {
        EXPECT_OK(mgr->postLobbyChat(p[0], "again " + std::to_string(i)));
    }
    EXPECT_CODE(RATE_LIMITED, mgr->postLobbyChat(p[0], "over"));
}

TEST(GameRoomManagerTest, LobbyChatValidatesText)
{
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postLobbyChat(p[0], ""));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postLobbyChat(p[0], "   \t "));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postLobbyChat(p[0], "bad\nline"));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postLobbyChat(p[0], std::string("nul\0x", 5)));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postLobbyChat(p[0], "\xC3\x28 broken utf8"));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->postLobbyChat(p[0], std::string(GameRoomManager::kMaxChatBytes + 1, 'x')));
    EXPECT_OK(mgr->postLobbyChat(p[0], std::string(GameRoomManager::kMaxChatBytes, 'x')));
    EXPECT_OK(mgr->postLobbyChat(p[0], "caf\xC3\xA9 \xE2\x99\x9E"));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->postLobbyChat(777777, "ghost"));
}

TEST(GameRoomManagerTest, LobbyChatKeepsOnlyTheNewest200Messages)
{
    for (int i = 0; i < 250; ++i) {
        EXPECT_OK(mgr->postLobbyChat(p[static_cast<size_t>(i % 3)],
                                     "msg " + std::to_string(i)));
        now += 3000;
    }
    LobbySnapshot snap = mgr->lobbyState();
    CHECK_EQUAL(200u, snap.recentChat.size());
    STRCMP_EQUAL("msg 50", snap.recentChat.front().text.c_str());
    STRCMP_EQUAL("msg 249", snap.recentChat.back().text.c_str());
    for (size_t i = 1; i < snap.recentChat.size(); ++i) {
        CHECK_TRUE(snap.recentChat[i].seq > snap.recentChat[i - 1].seq);
    }
    LobbyPoll stale = mgr->pollLobbyEvents(0, 0, 500);
    CHECK_TRUE(stale.gap);
    LobbyPoll fresh = mgr->pollLobbyEvents(0, snap.latestSeq, 500);
    CHECK_FALSE(fresh.gap);
}

TEST(GameRoomManagerTest, MatchFoundEventsArePrivate)
{
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    EXPECT_OK(mgr->enqueueMatchmaking(p[1], MatchRuleset()));
    TickReport rep = mgr->tick();
    CHECK_EQUAL(1, rep.matchesSpawned);
    for (int i = 0; i < 3; ++i) {
        LobbyPoll lp = mgr->pollLobbyEvents(p[static_cast<size_t>(i)], 0, 500);
        int found = 0;
        for (size_t k = 0; k < lp.events.size(); ++k) {
            if (lp.events[k].type == LobbyEventType::MATCH_FOUND) {
                ++found;
                LONGS_EQUAL(p[static_cast<size_t>(i)], lp.events[k].targetPlayerId);
            }
        }
        CHECK_EQUAL(i < 2 ? 1 : 0, found);
    }
    LobbyPoll anon = mgr->pollLobbyEvents(0, 0, 500);
    for (size_t k = 0; k < anon.events.size(); ++k) {
        CHECK_TRUE(anon.events[k].type != LobbyEventType::MATCH_FOUND);
    }
}

/* ---- tables ----------------------------------------------------------- */

TEST(GameRoomManagerTest, CreateAndJoinAStandardTable)
{
    RoomResult c = mgr->createTable(p[0], cfg());
    EXPECT_OK(c);
    STRCMP_EQUAL("g-20270115-0001", c.roomId.c_str());
    CHECK_EQUAL(1, c.seat);

    RoomView w = view(c.roomId);
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", toString(w.phase));
    CHECK_EQUAL(2u, w.seats.size());
    LONGS_EQUAL(p[0], w.seats[0].playerId);
    STRCMP_EQUAL(handle[0].c_str(), w.seats[0].handle.c_str());
    CHECK_EQUAL(1200, w.seats[0].rating);
    LONGS_EQUAL(0, w.seats[1].playerId);
    LONGS_EQUAL(p[0], w.hostPlayerId);
    CHECK_EQUAL(0, w.turnSeat);

    RoomResult j = mgr->joinTable(c.roomId, p[1], 0, "");
    EXPECT_OK(j);
    CHECK_EQUAL(2, j.seat);
    CHECK_TRUE(j.gameStarted);
    RoomView v = view(c.roomId, p[1]);
    STRCMP_EQUAL("IN_PROGRESS", toString(v.phase));
    CHECK_EQUAL(1, v.turnSeat);
    CHECK_EQUAL(2, v.viewerSeat);
    STRCMP_EQUAL("2:3:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77",
                 v.fen.c_str());

    /* full table, second table, double seating */
    EXPECT_CODE(INVALID_STATE, mgr->joinTable(c.roomId, p[2], 0, ""));
    EXPECT_CODE(ALREADY_SEATED, mgr->createTable(p[0], cfg()));
    CHECK_EQUAL(1u, mgr->roomCount());
    EXPECT_CODE(NOT_FOUND, mgr->joinTable("g-nope", p[2], 0, ""));
}

TEST(GameRoomManagerTest, SeatSelectionAndDoubleSeating)
{
    RoomResult c = mgr->createTable(p[0], cfg());
    EXPECT_OK(c);
    EXPECT_CODE(INVALID_ARGUMENT, mgr->joinTable(c.roomId, p[1], 3, ""));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->joinTable(c.roomId, p[1], -1, ""));
    EXPECT_CODE(SEAT_TAKEN, mgr->joinTable(c.roomId, p[1], 1, ""));
    EXPECT_CODE(ALREADY_SEATED, mgr->joinTable(c.roomId, p[0], 2, ""));
    RoomResult j = mgr->joinTable(c.roomId, p[1], 2, "");
    EXPECT_OK(j);
    CHECK_EQUAL(2, j.seat);
}

TEST(GameRoomManagerTest, FourPlayerTableStartsOnlyWhenFull)
{
    GameTableConfig c4 = cfg(GAME_MODE_4);
    RoomResult c = mgr->createTable(p[0], c4);
    EXPECT_OK(c);
    RoomResult j1 = mgr->joinTable(c.roomId, p[1], 4, "");
    RoomResult j2 = mgr->joinTable(c.roomId, p[2], 3, "");
    EXPECT_OK(j1);
    EXPECT_OK(j2);
    CHECK_FALSE(j1.gameStarted);
    CHECK_FALSE(j2.gameStarted);
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", toString(view(c.roomId).phase));
    EXPECT_CODE(SEAT_TAKEN, mgr->joinTable(c.roomId, p[3], 4, ""));
    RoomResult j3 = mgr->joinTable(c.roomId, p[3], 2, "");
    EXPECT_OK(j3);
    CHECK_TRUE(j3.gameStarted);
    RoomView v = view(c.roomId);
    CHECK_EQUAL(4u, v.seats.size());
    LONGS_EQUAL(p[3], v.seats[1].playerId);
    LONGS_EQUAL(p[2], v.seats[2].playerId);
    LONGS_EQUAL(p[1], v.seats[3].playerId);
    CHECK_TRUE(startsWith(v.fen, "4:3:0:0:P:A:-:0:3,0,12,0:3,0,12,0:3,0,12,0:3,0,12,0:"));
    CHECK_EQUAL(1, v.turnSeat);
}

TEST(GameRoomManagerTest, OctiForKidsTableNormalizesIgnoredOptions)
{
    GameTableConfig c = cfg(GAME_MODE_OFK);
    c.bases = GAME_BASES_3;
    c.useSuperProngs = true;
    RoomResult r = mgr->createTable(p[0], c);
    EXPECT_OK(r);
    EXPECT_OK(mgr->joinTable(r.roomId, p[1], 0, ""));
    RoomView v = view(r.roomId);
    CHECK_EQUAL(static_cast<int>(GAME_BASES_1), static_cast<int>(v.config.bases));
    CHECK_FALSE(v.config.useSuperProngs);
    STRCMP_EQUAL("OFK:1:0:0:P:A:-:0:0,0,12,0:0,0,12,0:A22,A32,A42,A52,B26,B36,B46,B56",
                 v.fen.c_str());
}

TEST(GameRoomManagerTest, PasscodeGuardsTheSeatAndIsNeverExposed)
{
    GameTableConfig c = cfg();
    c.passcode = "s3cret";
    RoomResult r = mgr->createTable(p[0], c);
    EXPECT_OK(r);
    EXPECT_CODE(FORBIDDEN_PASSCODE, mgr->joinTable(r.roomId, p[1], 0, ""));
    EXPECT_CODE(FORBIDDEN_PASSCODE, mgr->joinTable(r.roomId, p[1], 0, "s3cre"));
    EXPECT_CODE(FORBIDDEN_PASSCODE, mgr->joinTable(r.roomId, p[1], 0, "s3cretx"));
    EXPECT_CODE(FORBIDDEN_PASSCODE, mgr->joinTable(r.roomId, p[1], 0, "S3CRET"));

    RoomView v = view(r.roomId);
    CHECK_TRUE(v.hasPasscode);
    CHECK_TRUE(v.config.passcode.empty());
    std::vector<TableSummary> tables = mgr->listTables();
    CHECK_EQUAL(1u, tables.size());
    CHECK_TRUE(tables[0].hasPasscode);
    CHECK_TRUE(tables[0].config.passcode.empty());

    EXPECT_OK(mgr->joinTable(r.roomId, p[1], 0, "s3cret"));
}

TEST(GameRoomManagerTest, TableConfigurationIsValidated)
{
    GameTableConfig bad = cfg();
    bad.mode = GAME_MODE_UNDEF;
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = cfg();
    bad.bases = static_cast<OctiGameBases>(2);
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = timedCfg(0, 0);
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = timedCfg(1000, -1);
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = timedCfg(GameRoomManager::kMaxClockSettingMs + 1, 0);
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = cfg();
    bad.timeControl.disconnectGraceMs = GameRoomManager::kMaxGraceMs + 1;
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = cfg();
    bad.passcode = std::string(GameRoomManager::kMaxPasscodeBytes + 1, 'p');
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = cfg();
    bad.passcode = "tab\there";
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    bad = cfg();
    bad.observerPolicy = static_cast<ObserverPolicy>(9);
    EXPECT_CODE(INVALID_ARGUMENT, mgr->createTable(p[0], bad));
    CHECK_EQUAL(0u, mgr->roomCount());
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->createTable(31337, cfg()));

    /* a rated table can never allow takebacks; a casual one can */
    GameTableConfig rated = cfg();
    rated.rated = true;
    rated.allowTakeback = true;
    RoomResult r1 = mgr->createTable(p[0], rated);
    EXPECT_OK(r1);
    CHECK_FALSE(view(r1.roomId).config.allowTakeback);
    GameTableConfig casual = cfg();
    casual.rated = false;
    casual.allowTakeback = true;
    RoomResult r2 = mgr->createTable(p[1], casual);
    EXPECT_OK(r2);
    CHECK_TRUE(view(r2.roomId).config.allowTakeback);

    /* an untimed table forgets stray clock settings */
    GameTableConfig stray = cfg();
    stray.timeControl.initialMs = 5000;
    stray.timeControl.incrementMs = 100;
    RoomResult r3 = mgr->createTable(p[2], stray);
    EXPECT_OK(r3);
    CHECK_EQUAL(0, static_cast<int>(view(r3.roomId).config.timeControl.initialMs));
    CHECK_EQUAL(-1, static_cast<int>(view(r3.roomId).seats[0].clockMs));
}

TEST(GameRoomManagerTest, RoomLimitIsEnforced)
{
    RoomLimits lim;
    lim.maxRooms = 2;
    mgr->setLimits(lim);
    EXPECT_OK(mgr->createTable(p[0], cfg()));
    EXPECT_OK(mgr->createTable(p[1], cfg()));
    EXPECT_CODE(LIMIT_REACHED, mgr->createTable(p[2], cfg()));
}

TEST(GameRoomManagerTest, LeavingAWaitingTableTransfersOrClosesIt)
{
    GameTableConfig c = cfg(GAME_MODE_4);
    RoomResult r = mgr->createTable(p[0], c);
    EXPECT_OK(r);
    EXPECT_OK(mgr->joinTable(r.roomId, p[1], 3, ""));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->leaveTable(r.roomId, p[2]));

    EXPECT_OK(mgr->leaveTable(r.roomId, p[0]));       /* host leaves */
    RoomView v = view(r.roomId);
    LONGS_EQUAL(p[1], v.hostPlayerId);
    LONGS_EQUAL(0, v.seats[0].playerId);
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[0])));
    CHECK_EQUAL(1u, countType(events(r.roomId, 0), RoomEventType::HOST_CHANGED));

    /* freed seat can be reused; the last one out closes the table */
    EXPECT_OK(mgr->joinTable(r.roomId, p[2], 1, ""));
    EXPECT_OK(mgr->leaveTable(r.roomId, p[2]));
    EXPECT_OK(mgr->leaveTable(r.roomId, p[1]));
    RoomView gone;
    EXPECT_CODE(NOT_FOUND, mgr->getRoomView(r.roomId, 0, gone));

    /* a started game cannot be left, only resigned */
    std::string room = startTable(cfg(), 2);
    EXPECT_CODE(INVALID_STATE, mgr->leaveTable(room, p[1]));
}

TEST(GameRoomManagerTest, OnlyTheHostMayKickAWaitingSeat)
{
    RoomResult r = mgr->createTable(p[0], cfg(GAME_MODE_4));
    EXPECT_OK(r);
    EXPECT_OK(mgr->joinTable(r.roomId, p[1], 0, ""));
    EXPECT_OK(mgr->joinTable(r.roomId, p[2], 0, ""));
    EXPECT_CODE(FORBIDDEN_NOT_HOST, mgr->kickSeat(r.roomId, p[1], p[2]));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->kickSeat(r.roomId, p[5], p[2]));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->kickSeat(r.roomId, p[0], p[0]));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->kickSeat(r.roomId, p[0], p[6]));
    EXPECT_OK(mgr->kickSeat(r.roomId, p[0], p[2]));
    RoomView v = view(r.roomId);
    LONGS_EQUAL(0, v.seats[2].playerId);
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[2])));
    /* a kick is not a ban */
    EXPECT_OK(mgr->joinTable(r.roomId, p[2], 3, ""));
    CHECK_EQUAL(1u, countType(events(r.roomId, 0), RoomEventType::SEAT_KICKED));
    /* no kicking once the game runs */
    EXPECT_OK(mgr->joinTable(r.roomId, p[3], 0, ""));
    EXPECT_CODE(INVALID_STATE, mgr->kickSeat(r.roomId, p[0], p[1]));
}

TEST(GameRoomManagerTest, TableListShowsOpenAndLiveTablesOnly)
{
    RoomResult open = mgr->createTable(p[0], cfg());
    EXPECT_OK(open);
    /* p[0] is already seated, so seat a second pair with other players */
    RoomResult t2 = mgr->createTable(p[2], cfg());
    EXPECT_OK(t2);
    EXPECT_OK(mgr->joinTable(t2.roomId, p[3], 0, ""));
    std::vector<TableSummary> tables = mgr->listTables();
    CHECK_EQUAL(2u, tables.size());
    EXPECT_OK(mgr->resign(t2.roomId, p[3]));
    CHECK_EQUAL(1u, mgr->listTables().size());
    CHECK_EQUAL(2u, mgr->listTables(true).size());
}

/* ---- authoritative moves ---------------------------------------------- */

TEST(GameRoomManagerTest, OutOfTurnMovePreservesBoardAndClock)
{
    std::string room = startTable(timedCfg(60000, 0), 2);
    now += 3000;
    RoomView before = view(room);
    EXPECT_CODE(NOT_YOUR_TURN, mgr->submitMove(room, p[1], "37+e"));
    RoomView after = view(room);
    STRCMP_EQUAL(before.fen.c_str(), after.fen.c_str());
    CHECK_EQUAL(0, after.plyCount);
    LONGS_EQUAL(before.latestSeq, after.latestSeq);
    LONGS_EQUAL(57000, after.seats[0].clockMs);
    LONGS_EQUAL(60000, after.seats[1].clockMs);
    CHECK_EQUAL(1, after.turnSeat);
    /* and the rightful player still moves, charged for the whole 3 s */
    RoomResult ok = mgr->submitMove(room, p[0], "33+a");
    EXPECT_OK(ok);
    LONGS_EQUAL(3000, ok.elapsedMs);
    LONGS_EQUAL(57000, ok.clockMs);
}

TEST(GameRoomManagerTest, MalformedAndIllegalMovesAreRejectedWithoutSideEffects)
{
    std::string room = startTable(timedCfg(60000, 1000), 2);
    now += 2500;
    RoomView before = view(room);

    EXPECT_CODE(ILLEGAL_SYNTAX, mgr->submitMove(room, p[0], "zz"));
    EXPECT_CODE(ILLEGAL_SYNTAX, mgr->submitMove(room, p[0], ""));
    EXPECT_CODE(ILLEGAL_SYNTAX, mgr->submitMove(room, p[0], "   "));
    EXPECT_CODE(ILLEGAL_SYNTAX, mgr->submitMove(room, p[0], "33+a\n37+e"));
    EXPECT_CODE(ILLEGAL_SYNTAX, mgr->submitMove(room, p[0], "33aa+b"));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitMove(room, p[0], std::string(GameRoomManager::kMaxMoveBytes + 1, '3')));
    /* syntactically fine, but pod 33 has no 'a' prong to move along */
    RoomResult rule = mgr->submitMove(room, p[0], "33a-34");
    CHECK_TRUE(rule.code == RoomCode::ILLEGAL_MOVE || rule.code == RoomCode::ILLEGAL_SYNTAX);

    RoomView after = view(room);
    STRCMP_EQUAL(before.fen.c_str(), after.fen.c_str());
    CHECK_EQUAL(0, after.plyCount);
    LONGS_EQUAL(before.latestSeq, after.latestSeq);
    LONGS_EQUAL(before.seats[0].clockMs, after.seats[0].clockMs);
    CHECK_EQUAL(1, after.turnSeat);
    CHECK_EQUAL(0u, countType(events(room, 0), RoomEventType::MOVE));

    /* the failed attempts did not restart the clock: emt covers all 2.5 s */
    RoomResult ok = mgr->submitMove(room, p[0], "33+a");
    EXPECT_OK(ok);
    LONGS_EQUAL(2500, ok.elapsedMs);
    LONGS_EQUAL(60000 - 2500 + 1000, ok.clockMs);
}

TEST(GameRoomManagerTest, RuleViolationInOctiForKidsIsRejectedByTheEngine)
{
    std::string room = startOfk();
    RoomView before = view(room);
    /* OFK forbids pod placement (core: DisallowPodPlacementAndReposition) */
    EXPECT_CODE(ILLEGAL_MOVE, mgr->submitMove(room, p[0], "22"));
    RoomView after = view(room);
    STRCMP_EQUAL(before.fen.c_str(), after.fen.c_str());
    LONGS_EQUAL(before.latestSeq, after.latestSeq);
    EXPECT_OK(mgr->submitMove(room, p[0], "22+a"));
    EXPECT_OK(mgr->submitMove(room, p[1], "26+e"));
    /* ... and stacking */
    EXPECT_OK(mgr->submitMove(room, p[0], "32+c"));
    EXPECT_OK(mgr->submitMove(room, p[1], "36+e"));
    EXPECT_CODE(ILLEGAL_MOVE, mgr->submitMove(room, p[0], "32c-42"));
}

TEST(GameRoomManagerTest, OnlySeatedPlayersMoveAndOnlyWhileInProgress)
{
    RoomResult c = mgr->createTable(p[0], cfg());
    EXPECT_OK(c);
    EXPECT_CODE(INVALID_STATE, mgr->submitMove(c.roomId, p[0], "33+a"));
    EXPECT_OK(mgr->joinTable(c.roomId, p[1], 0, ""));
    EXPECT_OK(mgr->observeRoom(c.roomId, p[2]));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->submitMove(c.roomId, p[2], "33+a"));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->submitMove(c.roomId, p[3], "33+a"));
    EXPECT_CODE(NOT_FOUND, mgr->submitMove("g-none", p[0], "33+a"));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->submitMove(c.roomId, 31337, "33+a"));
    EXPECT_OK(mgr->resign(c.roomId, p[0]));
    EXPECT_CODE(INVALID_STATE, mgr->submitMove(c.roomId, p[1], "37+e"));
}

TEST(GameRoomManagerTest, CompleteOctiForKidsGameEndsArchivesAndRates)
{
    std::string room = startOfk();
    EngineReplay truth = replayOnEngine(GAME_MODE_OFK, true, kOfkWin);
    CHECK_TRUE(truth.over);

    std::vector<std::string> head(kOfkWin.begin(), kOfkWin.end() - 1);
    RoomResult mid = play(room, head, 2);
    CHECK_FALSE(mid.gameOver);
    STRCMP_EQUAL("IN_PROGRESS", toString(view(room).phase));
    RoomResult last = mgr->submitMove(room, p[0], kOfkWin.back());
    EXPECT_OK(last);
    CHECK_TRUE(last.gameOver);

    RoomView v = view(room);
    STRCMP_EQUAL("FINISHED", toString(v.phase));
    STRCMP_EQUAL(truth.winner == 1 ? "P1_WIN" : "P2_WIN", v.result.c_str());
    STRCMP_EQUAL("BASE_CAPTURE", v.termination.c_str());
    CHECK_EQUAL(truth.winner, v.winnerSeat);
    STRCMP_EQUAL(truth.fen.c_str(), v.fen.c_str());
    CHECK_EQUAL(static_cast<int>(kOfkWin.size()), v.plyCount);
    CHECK_EQUAL(0, v.turnSeat);
    CHECK_TRUE(v.archived);
    CHECK_TRUE(v.archiveError.empty());

    GameRecord rec = archived(room);
    STRCMP_EQUAL("OFK", rec.gameMode.c_str());
    CHECK_EQUAL(1, rec.gameBases);
    STRCMP_EQUAL("untimed", rec.timeControl.c_str());
    LONGS_EQUAL(p[0], rec.p1PlayerId);
    LONGS_EQUAL(p[1], rec.p2PlayerId);
    LONGS_EQUAL(0, rec.p3PlayerId);
    STRCMP_EQUAL(handle[0].c_str(), rec.p1Handle.c_str());
    STRCMP_EQUAL(v.result.c_str(), rec.result.c_str());
    CHECK_EQUAL(truth.winner, rec.winnerSeat);
    STRCMP_EQUAL("BASE_CAPTURE", rec.termination.c_str());
    CHECK_EQUAL(static_cast<int>(kOfkWin.size()), rec.plyCount);
    STRCMP_EQUAL(truth.fen.c_str(), rec.finalFen.c_str());
    LONGS_EQUAL(kWall0, rec.startedAt);
    checkTranscriptReplays(room, rec);

    /* Elo: winner +16, loser -16 in the OFK ladder only */
    int64_t winner = p[static_cast<size_t>(truth.winner - 1)];
    int64_t loser = p[static_cast<size_t>(2 - truth.winner)];
    PlayerProfile w = profile(winner);
    PlayerProfile l = profile(loser);
    CHECK_EQUAL(1216, w.eloOfk);
    CHECK_EQUAL(1184, l.eloOfk);
    CHECK_EQUAL(1200, w.elo2p);
    CHECK_EQUAL(1, w.wins);
    CHECK_EQUAL(1, l.losses);
    CHECK_EQUAL(1, w.gamesPlayed);

    /* both players are free again */
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[0])));
    EXPECT_OK(mgr->createTable(p[0], cfg()));
}

TEST(GameRoomManagerTest, CompleteTwoPlayerGameWithOneBase)
{
    GameTableConfig c = cfg(GAME_MODE_2);
    c.bases = GAME_BASES_1;
    std::string room = startTable(c, 2);
    EngineReplay truth = replayOnEngine(GAME_MODE_2, true, kTwoPlayerWin);
    CHECK_TRUE(truth.over);
    RoomResult last = play(room, kTwoPlayerWin, 2);
    CHECK_TRUE(last.gameOver);
    RoomView v = view(room);
    STRCMP_EQUAL(truth.fen.c_str(), v.fen.c_str());
    STRCMP_EQUAL("BASE_CAPTURE", v.termination.c_str());
    GameRecord rec = archived(room);
    CHECK_EQUAL(1, rec.gameBases);
    checkTranscriptReplays(room, rec);
    PlayerProfile w = profile(p[static_cast<size_t>(truth.winner - 1)]);
    CHECK_EQUAL(1216, w.elo2p);
    CHECK_EQUAL(1200, w.eloOfk);
}

TEST(GameRoomManagerTest, CompleteFourPlayerGameRatesBothTeams)
{
    std::string room = start4P();
    EngineReplay truth = replayOnEngine(GAME_MODE_4, false, kFourPlayerWin);
    CHECK_TRUE(truth.over);
    std::vector<int> seats = moverSeats(GAME_MODE_4, false, kFourPlayerWin);
    RoomResult last = playAs(room, kFourPlayerWin, seats);
    CHECK_TRUE(last.gameOver);

    RoomView v = view(room);
    STRCMP_EQUAL(truth.fen.c_str(), v.fen.c_str());
    STRCMP_EQUAL(truth.winner == 1 ? "TEAM_13_WIN" : "TEAM_24_WIN", v.result.c_str());
    STRCMP_EQUAL("BASE_CAPTURE", v.termination.c_str());
    GameRecord rec = archived(room);
    LONGS_EQUAL(p[2], rec.p3PlayerId);
    LONGS_EQUAL(p[3], rec.p4PlayerId);
    STRCMP_EQUAL(handle[3].c_str(), rec.p4Handle.c_str());
    CHECK_EQUAL(truth.winner, rec.winnerSeat);
    checkTranscriptReplays(room, rec);

    int sum = 0;
    for (int s = 1; s <= 4; ++s) {
        PlayerProfile pr = profile(p[static_cast<size_t>(s - 1)]);
        bool winners = (truth.winner == 1) ? (s % 2 == 1) : (s % 2 == 0);
        CHECK_EQUAL(winners ? 1216 : 1184, pr.elo4p);
        CHECK_EQUAL(winners ? 1 : 0, pr.wins);
        CHECK_EQUAL(winners ? 0 : 1, pr.losses);
        CHECK_EQUAL(1200, pr.elo2p);
        sum += pr.elo4p - 1200;
    }
    CHECK_EQUAL(0, sum);
}

TEST(GameRoomManagerTest, UnratedGamesAreArchivedButNeverRated)
{
    GameTableConfig c = cfg(GAME_MODE_OFK);
    c.rated = false;
    std::string room = startTable(c, 2);
    play(room, kOfkWin, 2);
    CHECK_TRUE(view(room).archived);
    archived(room);
    for (int i = 0; i < 2; ++i) {
        PlayerProfile pr = profile(p[static_cast<size_t>(i)]);
        CHECK_EQUAL(1200, pr.eloOfk);
        CHECK_EQUAL(0, pr.gamesPlayed);
    }
}

/* ---- Fischer clocks ---------------------------------------------------- */

TEST(GameRoomManagerTest, FischerClockDeductsElapsedAndAddsIncrement)
{
    std::string room = startTable(timedCfg(60000, 2000), 2);
    CHECK_EQUAL(60000, static_cast<int>(view(room).seats[0].clockMs));

    now += 1500;
    RoomResult m1 = mgr->submitMove(room, p[0], "33+a");
    EXPECT_OK(m1);
    LONGS_EQUAL(1500, m1.elapsedMs);
    LONGS_EQUAL(60500, m1.clockMs);
    STRCMP_EQUAL("33+a", m1.move.c_str());

    now += 700;
    RoomResult m2 = mgr->submitMove(room, p[1], "37+e");
    EXPECT_OK(m2);
    LONGS_EQUAL(61300, m2.clockMs);

    now += 400;                                 /* seat 1 is on the clock */
    RoomView v = view(room);
    LONGS_EQUAL(60100, v.seats[0].clockMs);
    LONGS_EQUAL(61300, v.seats[1].clockMs);

    std::vector<RoomEvent> ev = events(room, 0);
    int moves = 0;
    for (size_t i = 0; i < ev.size(); ++i) {
        if (ev[i].type == RoomEventType::MOVE) {
            ++moves;
            if (moves == 1) {
                LONGS_EQUAL(60500, ev[i].clockMs);
                LONGS_EQUAL(1500, ev[i].elapsedMs);
                CHECK_EQUAL(1, ev[i].ply);
            }
        }
    }
    CHECK_EQUAL(2, moves);

    std::string tr;
    EXPECT_OK(mgr->getTranscript(room, 0, tr));
    auto jtr = nlohmann::json::parse(tr);
    CHECK_EQUAL("60+2", jtr.value("timeControl", ""));
    CHECK_EQUAL(2, static_cast<int>(jtr["plies"].size()));
    CHECK_EQUAL("33+a", jtr["plies"][0].value("move", ""));
    CHECK_EQUAL(60500, jtr["plies"][0].value("clockAfterMs", 0));
    CHECK_EQUAL(1500, jtr["plies"][0].value("elapsedMs", 0));
    CHECK_EQUAL("37+e", jtr["plies"][1].value("move", ""));
    CHECK_EQUAL(61300, jtr["plies"][1].value("clockAfterMs", 0));
    CHECK_EQUAL(700, jtr["plies"][1].value("elapsedMs", 0));
    /* annotated transcripts still replay through the engine parser */
    OctiBoard rb;
    rb.deserializeGame(tr);
    STRCMP_EQUAL(view(room).fen.c_str(), rb.toPositionString().c_str());
}

TEST(GameRoomManagerTest, ClockReachingZeroForfeitsOnTime)
{
    std::string room = startTable(timedCfg(5000, 0), 2);
    now += 1000;
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));      /* seat 1: 4000 left */
    now += 500;
    EXPECT_OK(mgr->submitMove(room, p[1], "37+e"));      /* seat 2: 4500 left */

    now += 3999;                                         /* seat 1: 1 ms left */
    TickReport quiet = mgr->tick();
    CHECK_EQUAL(0, quiet.gamesTimedOut);
    LONGS_EQUAL(1, view(room).seats[0].clockMs);
    STRCMP_EQUAL("IN_PROGRESS", toString(view(room).phase));

    now += 1;                                            /* exactly 0 ms */
    TickReport rep = mgr->tick();
    CHECK_EQUAL(1, rep.gamesTimedOut);
    RoomView v = view(room);
    STRCMP_EQUAL("FINISHED", toString(v.phase));
    STRCMP_EQUAL("TIMEOUT", v.termination.c_str());
    STRCMP_EQUAL("P2_WIN", v.result.c_str());
    CHECK_EQUAL(2, v.winnerSeat);
    LONGS_EQUAL(0, v.seats[0].clockMs);
    CHECK_TRUE(v.archived);

    GameRecord rec = archived(room);
    STRCMP_EQUAL("TIMEOUT", rec.termination.c_str());
    STRCMP_EQUAL("5+0", rec.timeControl.c_str());
    auto jrec = nlohmann::json::parse(rec.octiTranscript);
    CHECK_EQUAL("TIMEOUT", jrec.value("termination", ""));
    CHECK_EQUAL("RESIGNED", jrec.value("status", ""));
    checkTranscriptReplays(room, rec);
    CHECK_EQUAL(1216, profile(p[1]).elo2p);
    CHECK_EQUAL(1184, profile(p[0]).elo2p);
    EXPECT_CODE(INVALID_STATE, mgr->submitMove(room, p[0], "33a-34"));
}

TEST(GameRoomManagerTest, MoveArrivingAfterTheFlagFellIsTooLate)
{
    std::string room = startTable(timedCfg(4000, 5000), 2);
    now += 4000;                    /* no tick ran; the move itself notices */
    EXPECT_CODE(INVALID_STATE, mgr->submitMove(room, p[0], "33+a"));
    RoomView v = view(room);
    STRCMP_EQUAL("TIMEOUT", v.termination.c_str());
    CHECK_EQUAL(0, v.plyCount);
    CHECK_EQUAL(1, countType(events(room, 0), RoomEventType::GAME_OVER));
}

TEST(GameRoomManagerTest, OneMillisecondBeforeTheFlagTheMoveStillCounts)
{
    std::string room = startTable(timedCfg(4000, 0), 2);
    now += 3999;
    RoomResult r = mgr->submitMove(room, p[0], "33+a");
    EXPECT_OK(r);
    LONGS_EQUAL(1, r.clockMs);
    STRCMP_EQUAL("IN_PROGRESS", toString(view(room).phase));
}

TEST(GameRoomManagerTest, UntimedGamesNeverFlagAndHaveNoClockAnnotations)
{
    std::string room = start2P();
    now += 100LL * 24 * 3600 * 1000;
    mgr->tick();
    RoomView v = view(room);
    STRCMP_EQUAL("IN_PROGRESS", toString(v.phase));
    LONGS_EQUAL(-1, v.seats[0].clockMs);
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));
    std::string tr;
    EXPECT_OK(mgr->getTranscript(room, 0, tr));
    auto jtr = nlohmann::json::parse(tr);
    CHECK_EQUAL("untimed", jtr.value("timeControl", ""));
    CHECK_EQUAL(1, static_cast<int>(jtr["plies"].size()));
    CHECK_EQUAL("33+a", jtr["plies"][0].value("move", ""));
    CHECK_FALSE(jtr["plies"][0].contains("clockAfterMs"));
}

/* ---- pause / resume ---------------------------------------------------- */

TEST(GameRoomManagerTest, MutualPauseFreezesTheClocksUntilResume)
{
    std::string room = startTable(timedCfg(60000, 0), 2);
    now += 1000;
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));      /* seat 1: 59000 */
    now += 3000;                                         /* seat 2 thinking */

    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::PAUSE_REQUEST));
    EXPECT_CODE(CONFLICT, mgr->submitProposal(room, p[1], ProposalType::PAUSE_REQUEST));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->respondProposal(room, p[0], ProposalType::PAUSE_REQUEST, true));
    CHECK_FALSE(view(room).paused);
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::PAUSE_REQUEST, true));

    RoomView v = view(room);
    CHECK_TRUE(v.paused);
    CHECK_EQUAL(0u, v.proposals.size());
    LONGS_EQUAL(57000, v.seats[1].clockMs);
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::GAME_PAUSED));

    now += 10LL * 3600 * 1000;                           /* hours pass */
    TickReport rep = mgr->tick();
    CHECK_EQUAL(0, rep.gamesTimedOut);
    v = view(room);
    LONGS_EQUAL(57000, v.seats[1].clockMs);
    LONGS_EQUAL(59000, v.seats[0].clockMs);
    EXPECT_CODE(INVALID_STATE, mgr->submitMove(room, p[1], "37+e"));
    EXPECT_CODE(INVALID_STATE, mgr->submitProposal(room, p[1], ProposalType::PAUSE_REQUEST));

    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->resumeGame(room, p[4]));
    EXPECT_OK(mgr->resumeGame(room, p[0]));              /* any seated player */
    CHECK_FALSE(view(room).paused);
    EXPECT_CODE(INVALID_STATE, mgr->resumeGame(room, p[0]));
    now += 1000;
    LONGS_EQUAL(56000, view(room).seats[1].clockMs);
    RoomResult m = mgr->submitMove(room, p[1], "37+e");
    EXPECT_OK(m);
    LONGS_EQUAL(4000, m.elapsedMs);                      /* 3 s + 1 s, no pause */
    LONGS_EQUAL(56000, m.clockMs);
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::GAME_RESUMED));
}

TEST(GameRoomManagerTest, PauseCanBeDeclinedAndProposalsExpire)
{
    std::string room = start2P();
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::PAUSE_REQUEST));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::PAUSE_REQUEST, false));
    CHECK_FALSE(view(room).paused);
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::PROPOSAL_DECLINED));
    EXPECT_CODE(INVALID_STATE,
                mgr->respondProposal(room, p[1], ProposalType::PAUSE_REQUEST, true));

    EXPECT_OK(mgr->submitProposal(room, p[1], ProposalType::PAUSE_REQUEST));
    now += 59999;
    CHECK_EQUAL(1u, view(room).proposals.size());
    now += 1;
    CHECK_EQUAL(0u, view(room).proposals.size());
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::PROPOSAL_EXPIRED));
    EXPECT_CODE(INVALID_STATE,
                mgr->respondProposal(room, p[0], ProposalType::PAUSE_REQUEST, true));
}

/* ---- undo / takeback --------------------------------------------------- */

TEST(GameRoomManagerTest, MutualTakebackRollsBackTheLastPlyOnACasualTable)
{
    GameTableConfig c = timedCfg(60000, 1000);
    c.rated = false;
    c.allowTakeback = true;
    std::string room = startTable(c, 2);
    std::string startFen = view(room).fen;

    now += 2000;
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));
    std::string afterOne = view(room).fen;
    now += 1000;
    EXPECT_OK(mgr->submitMove(room, p[1], "37+e"));
    CHECK_EQUAL(2, view(room).plyCount);

    /* take back only the last ply (black's move) */
    EXPECT_OK(mgr->submitProposal(room, p[1], ProposalType::UNDO_REQUEST));
    EXPECT_OK(mgr->respondProposal(room, p[0], ProposalType::UNDO_REQUEST, true));
    RoomView v = view(room);
    STRCMP_EQUAL(afterOne.c_str(), v.fen.c_str());
    CHECK_EQUAL(1, v.plyCount);
    CHECK_EQUAL(2, v.turnSeat);
    LONGS_EQUAL(60000, v.seats[1].clockMs);              /* restored */
    LONGS_EQUAL(59000, v.seats[0].clockMs);              /* untouched */
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::UNDO_APPLIED));

    std::string tr;
    EXPECT_OK(mgr->getTranscript(room, 0, tr));
    auto jtr = nlohmann::json::parse(tr);
    CHECK_EQUAL(1, static_cast<int>(jtr["plies"].size()));
    CHECK_TRUE(tr.find("37+e") == std::string::npos);

    /* undo the first ply as well */
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::UNDO_REQUEST));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::UNDO_REQUEST, true));
    v = view(room);
    STRCMP_EQUAL(startFen.c_str(), v.fen.c_str());
    CHECK_EQUAL(0, v.plyCount);
    CHECK_EQUAL(1, v.turnSeat);
    LONGS_EQUAL(60000, v.seats[0].clockMs);
    EXPECT_CODE(INVALID_STATE, mgr->submitProposal(room, p[0], ProposalType::UNDO_REQUEST));

    /* the game goes on normally; the turn clock restarted at the undo */
    now += 500;
    RoomResult m = mgr->submitMove(room, p[0], "33+b");
    EXPECT_OK(m);
    LONGS_EQUAL(500, m.elapsedMs);
}

TEST(GameRoomManagerTest, TakebackIsRefusedWhereNotAllowed)
{
    std::string rated = start2P();
    EXPECT_OK(mgr->submitMove(rated, p[0], "33+a"));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->submitProposal(rated, p[1], ProposalType::UNDO_REQUEST));
    EXPECT_OK(mgr->resign(rated, p[0]));

    GameTableConfig noUndo = cfg();
    noUndo.rated = false;
    noUndo.allowTakeback = false;
    RoomResult c = mgr->createTable(p[2], noUndo);
    EXPECT_OK(c);
    EXPECT_OK(mgr->joinTable(c.roomId, p[3], 0, ""));
    EXPECT_OK(mgr->submitMove(c.roomId, p[2], "33+a"));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->submitProposal(c.roomId, p[3], ProposalType::UNDO_REQUEST));
}

TEST(GameRoomManagerTest, TakebackNeedsAMoveAnAnswerAndIsSupersededByPlay)
{
    GameTableConfig c = cfg();
    c.rated = false;
    c.allowTakeback = true;
    std::string room = startTable(c, 2);
    EXPECT_CODE(INVALID_STATE, mgr->submitProposal(room, p[0], ProposalType::UNDO_REQUEST));
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));

    /* declined */
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::UNDO_REQUEST));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::UNDO_REQUEST, false));
    CHECK_EQUAL(1, view(room).plyCount);

    /* a move by the opponent supersedes a pending request */
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::UNDO_REQUEST));
    CHECK_EQUAL(1u, view(room).proposals.size());
    EXPECT_OK(mgr->submitMove(room, p[1], "37+e"));
    CHECK_EQUAL(0u, view(room).proposals.size());
    EXPECT_CODE(INVALID_STATE,
                mgr->respondProposal(room, p[1], ProposalType::UNDO_REQUEST, true));
    CHECK_EQUAL(2, view(room).plyCount);
}

/* ---- draw, resign, rematch -------------------------------------------- */

TEST(GameRoomManagerTest, AgreedDrawEndsTheGameAndPaysTheRatingGap)
{
    setRating2p(p[0], 1400);
    setRating2p(p[1], 1000);
    const int drawsBefore = profile(p[0]).draws;          /* setRating2p counts one */
    const int playedBefore = profile(p[0]).gamesPlayed;
    std::string room = start2P();
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::DRAW_OFFER));
    EXPECT_CODE(CONFLICT, mgr->submitProposal(room, p[1], ProposalType::DRAW_OFFER));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED,
                mgr->respondProposal(room, p[2], ProposalType::DRAW_OFFER, true));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->respondProposal(room, p[0], ProposalType::DRAW_OFFER, true));
    STRCMP_EQUAL("IN_PROGRESS", toString(view(room).phase));
    RoomResult acc = mgr->respondProposal(room, p[1], ProposalType::DRAW_OFFER, true);
    EXPECT_OK(acc);
    CHECK_TRUE(acc.gameOver);

    RoomView v = view(room);
    STRCMP_EQUAL("DRAW", v.result.c_str());
    STRCMP_EQUAL("AGREED_DRAW", v.termination.c_str());
    CHECK_EQUAL(0, v.winnerSeat);
    GameRecord rec = archived(room);
    STRCMP_EQUAL("DRAW", rec.result.c_str());
    CHECK_EQUAL(0, rec.winnerSeat);
    auto jrec = nlohmann::json::parse(rec.octiTranscript);
    CHECK_EQUAL("GAME_OVER", jrec.value("status", ""));
    CHECK_EQUAL("NONE", jrec.value("winner", ""));
    CHECK_EQUAL("AGREED_DRAW", jrec.value("termination", ""));
    PlayerProfile a = profile(p[0]);
    PlayerProfile b = profile(p[1]);
    CHECK_EQUAL(1387, a.elo2p);
    CHECK_EQUAL(1013, b.elo2p);
    CHECK_EQUAL(drawsBefore + 1, a.draws);
    CHECK_EQUAL(drawsBefore + 1, b.draws);
    CHECK_EQUAL(playedBefore + 1, a.gamesPlayed);
}

TEST(GameRoomManagerTest, DrawOfferCanBeDeclinedAndIsClearedByAMove)
{
    std::string room = start2P();
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::DRAW_OFFER));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::DRAW_OFFER, false));
    STRCMP_EQUAL("IN_PROGRESS", toString(view(room).phase));

    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::DRAW_OFFER));
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));
    CHECK_EQUAL(0u, view(room).proposals.size());
    EXPECT_CODE(INVALID_STATE, mgr->respondProposal(room, p[1], ProposalType::DRAW_OFFER, true));

    /* the proposer may withdraw */
    EXPECT_OK(mgr->submitProposal(room, p[1], ProposalType::DRAW_OFFER));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::DRAW_OFFER, false));
    CHECK_EQUAL(0u, view(room).proposals.size());
}

TEST(GameRoomManagerTest, FourPlayerDrawNeedsEveryOtherActiveSeat)
{
    std::string room = start4P();
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::DRAW_OFFER));
    RoomView v = view(room);
    CHECK_EQUAL(1u, v.proposals.size());
    CHECK_EQUAL(3u, v.proposals[0].awaiting.size());
    EXPECT_OK(mgr->respondProposal(room, p[2], ProposalType::DRAW_OFFER, true));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::DRAW_OFFER, true));
    STRCMP_EQUAL("IN_PROGRESS", toString(view(room).phase));
    CHECK_EQUAL(1u, view(room).proposals[0].awaiting.size());
    EXPECT_OK(mgr->respondProposal(room, p[3], ProposalType::DRAW_OFFER, true));
    RoomView done = view(room);
    STRCMP_EQUAL("DRAW", done.result.c_str());
    CHECK_TRUE(done.archived);
    for (int i = 0; i < 4; ++i) {
        CHECK_EQUAL(1, profile(p[static_cast<size_t>(i)]).draws);
    }

    /* a single decline kills it */
    std::string room2;
    RoomResult c = mgr->createTable(p[4], cfg(GAME_MODE_4));
    EXPECT_OK(c);
    EXPECT_OK(mgr->joinTable(c.roomId, p[5], 0, ""));
    EXPECT_OK(mgr->joinTable(c.roomId, p[6], 0, ""));
    EXPECT_OK(mgr->joinTable(c.roomId, p[7], 0, ""));
    EXPECT_OK(mgr->submitProposal(c.roomId, p[4], ProposalType::DRAW_OFFER));
    EXPECT_OK(mgr->respondProposal(c.roomId, p[5], ProposalType::DRAW_OFFER, true));
    EXPECT_OK(mgr->respondProposal(c.roomId, p[6], ProposalType::DRAW_OFFER, false));
    STRCMP_EQUAL("IN_PROGRESS", toString(view(c.roomId).phase));
    CHECK_EQUAL(0u, view(c.roomId).proposals.size());
}

TEST(GameRoomManagerTest, ResignationEndsTheGameForTheResigningSideOnly)
{
    std::string room = start2P();
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->resign(room, p[5]));
    RoomResult r = mgr->resign(room, p[1]);
    EXPECT_OK(r);
    CHECK_TRUE(r.gameOver);
    RoomView v = view(room);
    STRCMP_EQUAL("P1_WIN", v.result.c_str());
    STRCMP_EQUAL("RESIGNATION", v.termination.c_str());
    GameRecord rec = archived(room);
    auto jrec = nlohmann::json::parse(rec.octiTranscript);
    CHECK_EQUAL("RESIGNED", jrec.value("status", ""));
    CHECK_EQUAL("A", jrec.value("winner", ""));
    checkTranscriptReplays(room, rec);
    CHECK_EQUAL(1216, profile(p[0]).elo2p);
    CHECK_EQUAL(1184, profile(p[1]).elo2p);
    EXPECT_CODE(INVALID_STATE, mgr->resign(room, p[0]));
}

TEST(GameRoomManagerTest, FourPlayerResignationLosesForTheWholeTeam)
{
    std::string room = start4P();
    EXPECT_OK(mgr->resign(room, p[2]));                  /* seat 3: team 1 */
    RoomView v = view(room);
    STRCMP_EQUAL("TEAM_24_WIN", v.result.c_str());
    CHECK_EQUAL(2, v.winnerSeat);
    CHECK_EQUAL(1184, profile(p[0]).elo4p);
    CHECK_EQUAL(1184, profile(p[2]).elo4p);
    CHECK_EQUAL(1216, profile(p[1]).elo4p);
    CHECK_EQUAL(1216, profile(p[3]).elo4p);
    checkTranscriptReplays(room, archived(room));
}

TEST(GameRoomManagerTest, RematchSwapsSeatsAndKeepsTheTable)
{
    GameTableConfig c = cfg();
    c.observerPolicy = ObserverPolicy::SILENT_OBSERVERS;
    c.voicePolicy = VoicePolicy::ALL_ALLOWED;
    std::string room = startTable(c, 2);
    EXPECT_CODE(INVALID_STATE, mgr->submitProposal(room, p[0], ProposalType::REMATCH_REQUEST));
    EXPECT_OK(mgr->resign(room, p[0]));

    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::REMATCH_REQUEST));
    CHECK_EQUAL(1u, view(room).proposals.size());
    RoomResult acc = mgr->respondProposal(room, p[1], ProposalType::REMATCH_REQUEST, true);
    EXPECT_OK(acc);
    CHECK_FALSE(acc.roomId.empty());
    CHECK_TRUE(acc.roomId != room);

    RoomView nv = view(acc.roomId);
    STRCMP_EQUAL("IN_PROGRESS", toString(nv.phase));
    LONGS_EQUAL(p[1], nv.seats[0].playerId);             /* seats swapped */
    LONGS_EQUAL(p[0], nv.seats[1].playerId);
    CHECK_EQUAL(1, nv.turnSeat);
    CHECK_TRUE(nv.config.rated);
    STRCMP_EQUAL("SILENT_OBSERVERS", toString(nv.config.observerPolicy));
    STRCMP_EQUAL("ALL_ALLOWED", toString(nv.config.voicePolicy));
    STRCMP_EQUAL(acc.roomId.c_str(), view(room).rematchRoomId.c_str());
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::REMATCH_CREATED));
    STRCMP_EQUAL("PLAYING", toString(mgr->presenceOf(p[0])));
    EXPECT_CODE(INVALID_STATE, mgr->submitProposal(room, p[0], ProposalType::REMATCH_REQUEST));

    /* the new game works and p[1] (now seat 1) moves first */
    EXPECT_CODE(NOT_YOUR_TURN, mgr->submitMove(acc.roomId, p[0], "37+e"));
    EXPECT_OK(mgr->submitMove(acc.roomId, p[1], "33+a"));
    EXPECT_CODE(INVALID_STATE, mgr->submitProposal(acc.roomId, p[0], ProposalType::REMATCH_REQUEST));
}

TEST(GameRoomManagerTest, FourPlayerRematchRotatesTheStartingTeam)
{
    std::string room = start4P();
    EXPECT_OK(mgr->resign(room, p[0]));
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::REMATCH_REQUEST));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::REMATCH_REQUEST, true));
    EXPECT_OK(mgr->respondProposal(room, p[2], ProposalType::REMATCH_REQUEST, true));
    RoomResult last = mgr->respondProposal(room, p[3], ProposalType::REMATCH_REQUEST, true);
    EXPECT_OK(last);
    RoomView nv = view(last.roomId);
    LONGS_EQUAL(p[1], nv.seats[0].playerId);
    LONGS_EQUAL(p[0], nv.seats[1].playerId);
    LONGS_EQUAL(p[3], nv.seats[2].playerId);
    LONGS_EQUAL(p[2], nv.seats[3].playerId);
    STRCMP_EQUAL("IN_PROGRESS", toString(nv.phase));
}

TEST(GameRoomManagerTest, RematchFailsWhenAPlayerSatDownElsewhere)
{
    std::string room = start2P();
    EXPECT_OK(mgr->resign(room, p[0]));
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::REMATCH_REQUEST));
    EXPECT_OK(mgr->createTable(p[1], cfg()));            /* p[1] moved on */
    EXPECT_CODE(INVALID_STATE,
                mgr->respondProposal(room, p[1], ProposalType::REMATCH_REQUEST, true));
    CHECK_TRUE(view(room).rematchRoomId.empty());
}

/* ---- disconnects ------------------------------------------------------- */

TEST(GameRoomManagerTest, DisconnectForfeitOnlyAfterTheGraceWindow)
{
    GameTableConfig c = cfg();
    c.timeControl.disconnectGraceMs = 5000;
    std::string room = startTable(c, 2);

    EXPECT_OK(mgr->markDisconnected(room, p[1]));
    EXPECT_OK(mgr->markDisconnected(room, p[1]));        /* idempotent */
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::PLAYER_DISCONNECTED));
    now += 4999;
    EXPECT_CODE(INVALID_STATE, mgr->claimDisconnectForfeit(room, p[0]));
    RoomView v = view(room);
    CHECK_FALSE(v.seats[1].connected);
    CHECK_FALSE(v.seats[1].forfeitClaimable);
    LONGS_EQUAL(4999, v.seats[1].disconnectedForMs);

    now += 1;
    CHECK_TRUE(view(room).seats[1].forfeitClaimable);
    EXPECT_CODE(INVALID_STATE, mgr->claimDisconnectForfeit(room, p[1]));  /* self */
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->claimDisconnectForfeit(room, p[4]));
    RoomResult r = mgr->claimDisconnectForfeit(room, p[0]);
    EXPECT_OK(r);
    CHECK_TRUE(r.gameOver);
    CHECK_EQUAL(2, r.seat);
    RoomView done = view(room);
    STRCMP_EQUAL("P1_WIN", done.result.c_str());
    STRCMP_EQUAL("DISCONNECT_FORFEIT", done.termination.c_str());
    CHECK_EQUAL(1216, profile(p[0]).elo2p);
    GameRecord rec = archived(room);
    auto jrec = nlohmann::json::parse(rec.octiTranscript);
    CHECK_EQUAL("DISCONNECT_FORFEIT", jrec.value("termination", ""));
    checkTranscriptReplays(room, rec);
}

TEST(GameRoomManagerTest, ReconnectingInsideTheGraceWindowCancelsTheClaim)
{
    GameTableConfig c = cfg();
    c.timeControl.disconnectGraceMs = 5000;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->markDisconnected(room, p[1]));
    now += 4000;
    EXPECT_OK(mgr->markReconnected(room, p[1]));
    EXPECT_OK(mgr->markReconnected(room, p[1]));         /* idempotent */
    now += 60000;
    EXPECT_CODE(INVALID_STATE, mgr->claimDisconnectForfeit(room, p[0]));
    CHECK_TRUE(view(room).seats[1].connected);
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::PLAYER_RECONNECTED));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->markDisconnected(room, p[5]));
    /* and a paused game cannot be claimed either */
    EXPECT_OK(mgr->markDisconnected(room, p[1]));
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::PAUSE_REQUEST));
    EXPECT_OK(mgr->respondProposal(room, p[1], ProposalType::PAUSE_REQUEST, true));
    now += 60000;
    EXPECT_CODE(INVALID_STATE, mgr->claimDisconnectForfeit(room, p[0]));
}

/* ---- board text chat --------------------------------------------------- */

TEST(GameRoomManagerTest, BoardChatReachesEveryoneWhoWatches)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "  good luck "));
    EXPECT_OK(mgr->postBoardChat(room, p[2], ChatScope::ALL, "nice opening"));

    const int64_t viewers[] = {p[0], p[1], p[2], 0};
    for (int v = 0; v < 4; ++v) {
        std::vector<RoomEvent> ev = events(room, viewers[v]);
        std::vector<RoomEvent> chats;
        for (size_t i = 0; i < ev.size(); ++i) {
            if (ev[i].type == RoomEventType::CHAT) {
                chats.push_back(ev[i]);
            }
        }
        CHECK_EQUAL(2u, chats.size());
        STRCMP_EQUAL("good luck", chats[0].text.c_str());
        STRCMP_EQUAL(handle[0].c_str(), chats[0].handle.c_str());
        CHECK_EQUAL(1, chats[0].seat);
        STRCMP_EQUAL("nice opening", chats[1].text.c_str());
        CHECK_EQUAL(0, chats[1].seat);
        CHECK_TRUE(chats[1].seq > chats[0].seq);
    }
}

TEST(GameRoomManagerTest, TeamChatIsVisibleToTheTeamOnly)
{
    std::string room = start4P();
    EXPECT_OK(mgr->observeRoom(room, p[4]));
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::TEAM, "flank left"));
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "hello all"));

    const struct { int64_t id; size_t expectTeam; } viewers[] = {
        {p[0], 1}, {p[2], 1}, {p[1], 0}, {p[3], 0}, {p[4], 0}, {0, 0}};
    for (size_t v = 0; v < 6; ++v) {
        std::vector<RoomEvent> ev = events(room, viewers[v].id);
        size_t team = 0;
        size_t all = 0;
        for (size_t i = 0; i < ev.size(); ++i) {
            if (ev[i].type != RoomEventType::CHAT) {
                continue;
            }
            (ev[i].scope == ChatScope::TEAM ? team : all) += 1;
        }
        CHECK_EQUAL(viewers[v].expectTeam, team);
        CHECK_EQUAL(1u, all);
    }
    /* team 2 has its own channel */
    EXPECT_OK(mgr->postBoardChat(room, p[3], ChatScope::TEAM, "hold the centre"));
    size_t teamForSeat2 = 0;
    size_t teamForSeat1 = 0;
    std::vector<RoomEvent> ev2 = events(room, p[1]);
    std::vector<RoomEvent> ev1 = events(room, p[0]);
    for (size_t i = 0; i < ev2.size(); ++i) {
        if (ev2[i].type == RoomEventType::CHAT && ev2[i].scope == ChatScope::TEAM) {
            STRCMP_EQUAL("hold the centre", ev2[i].text.c_str());
            ++teamForSeat2;
        }
    }
    for (size_t i = 0; i < ev1.size(); ++i) {
        if (ev1[i].type == RoomEventType::CHAT && ev1[i].scope == ChatScope::TEAM) {
            STRCMP_EQUAL("flank left", ev1[i].text.c_str());
            ++teamForSeat1;
        }
    }
    CHECK_EQUAL(1u, teamForSeat2);
    CHECK_EQUAL(1u, teamForSeat1);
    EXPECT_CODE(FORBIDDEN_NOT_SEATED,
                mgr->postBoardChat(room, p[4], ChatScope::TEAM, "psst"));
}

TEST(GameRoomManagerTest, TeamChatDoesNotExistInTwoPlayerGames)
{
    std::string room = start2P();
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postBoardChat(room, p[0], ChatScope::TEAM, "x"));
}

TEST(GameRoomManagerTest, ObserversMustRegisterBeforeTheyCanChat)
{
    std::string room = start2P();
    EXPECT_CODE(FORBIDDEN_NOT_PARTICIPANT,
                mgr->postBoardChat(room, p[2], ChatScope::ALL, "hi"));
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->observeRoom(room, p[2]));            /* idempotent */
    CHECK_EQUAL(1u, countType(events(room, 0), RoomEventType::OBSERVER_JOINED));
    EXPECT_OK(mgr->postBoardChat(room, p[2], ChatScope::ALL, "hi"));
    EXPECT_OK(mgr->stopObserving(room, p[2]));
    EXPECT_CODE(INVALID_STATE, mgr->stopObserving(room, p[2]));
    EXPECT_CODE(FORBIDDEN_NOT_PARTICIPANT,
                mgr->postBoardChat(room, p[2], ChatScope::ALL, "hi again"));
    EXPECT_CODE(INVALID_STATE, mgr->observeRoom(room, p[0]));    /* seated */
    EXPECT_CODE(NOT_FOUND, mgr->observeRoom("g-none", p[2]));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->observeRoom(room, 424242));
}

TEST(GameRoomManagerTest, SilentObserversWatchButDoNotSpeak)
{
    GameTableConfig c = cfg();
    c.observerPolicy = ObserverPolicy::SILENT_OBSERVERS;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->postBoardChat(room, p[2], ChatScope::ALL, "psst"));
    EXPECT_OK(mgr->postBoardChat(room, p[1], ChatScope::ALL, "players can talk"));
    view(room, p[2]);                                   /* may still watch */
    EXPECT_OK(mgr->updateRoomPolicies(room, p[0], ObserverPolicy::OPEN, VoicePolicy::PLAYERS_ONLY));
    EXPECT_OK(mgr->postBoardChat(room, p[2], ChatScope::ALL, "now I may"));
}

TEST(GameRoomManagerTest, BoardChatIsValidatedAndRateLimited)
{
    std::string room = start2P();
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postBoardChat(room, p[0], ChatScope::ALL, ""));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->postBoardChat(room, p[0], ChatScope::ALL, "a\tb\x01"));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->postBoardChat(room, p[0], ChatScope::ALL,
                                   std::string(GameRoomManager::kMaxChatBytes + 1, 'y')));
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL,
                                 std::string(GameRoomManager::kMaxChatBytes, 'y')));
    for (int i = 0; i < 9; ++i) {
        EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "m" + std::to_string(i)));
    }
    EXPECT_CODE(RATE_LIMITED, mgr->postBoardChat(room, p[0], ChatScope::ALL, "eleventh"));
    EXPECT_OK(mgr->postBoardChat(room, p[1], ChatScope::ALL, "other player is fine"));
    now += 9999;
    EXPECT_CODE(RATE_LIMITED, mgr->postBoardChat(room, p[0], ChatScope::ALL, "eleventh"));
    now += 1;
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "eleventh"));
    EXPECT_CODE(NOT_FOUND, mgr->postBoardChat("g-none", p[0], ChatScope::ALL, "x"));
}

/* ---- P2P voice signaling and meta ------------------------------------- */

TEST(GameRoomManagerTest, VoiceSignalsAreRelayedOnlyToTheNamedPeer)
{
    GameTableConfig c = cfg();
    c.voicePolicy = VoicePolicy::ALL_ALLOWED;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->observeRoom(room, p[2]));

    const std::string offer = "{\"sdp\":\"v=0 secret-offer\",\"type\":\"offer\"}";
    RoomResult sent = mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, offer);
    EXPECT_OK(sent);
    CHECK_TRUE(sent.seq > 0);

    std::vector<RoomEvent> forPeer = events(room, p[1]);
    CHECK_EQUAL(1u, countType(forPeer, RoomEventType::VOICE_SIGNAL));
    for (size_t i = 0; i < forPeer.size(); ++i) {
        if (forPeer[i].type == RoomEventType::VOICE_SIGNAL) {
            STRCMP_EQUAL("SDP_OFFER", forPeer[i].detail.c_str());
            STRCMP_EQUAL(offer.c_str(), forPeer[i].text.c_str());
            LONGS_EQUAL(p[0], forPeer[i].playerId);
            LONGS_EQUAL(p[1], forPeer[i].targetPlayerId);
        }
    }
    /* nobody else sees it: not the sender, not the observer, not a spectator */
    CHECK_EQUAL(0u, countType(events(room, p[0]), RoomEventType::VOICE_SIGNAL));
    CHECK_EQUAL(0u, countType(events(room, p[2]), RoomEventType::VOICE_SIGNAL));
    CHECK_EQUAL(0u, countType(events(room, 0), RoomEventType::VOICE_SIGNAL));

    /* answer and ICE flow back the other way, observer can be a peer too */
    EXPECT_OK(mgr->submitVoiceSignal(room, p[1], p[0], VoiceSignalType::SDP_ANSWER,
                                     "{\"sdp\":\"v=0 answer\"}"));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[1], p[0], VoiceSignalType::ICE_CANDIDATE,
                                     "{\"candidate\":\"candidate:1 1 UDP 1 10.0.0.1 9 typ host\"}"));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[2], VoiceSignalType::ICE_CANDIDATE, "{}"));
    std::vector<RoomEvent> back = events(room, p[0]);
    CHECK_EQUAL(2u, countType(back, RoomEventType::VOICE_SIGNAL));
    CHECK_EQUAL(1u, countType(events(room, p[2]), RoomEventType::VOICE_SIGNAL));
}

TEST(GameRoomManagerTest, DeliveredSignalsAreForgottenAndNeverArchived)
{
    std::string room = start2P();
    const std::string secret = "{\"sdp\":\"never-store-this-offer\"}";
    RoomResult sent = mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, secret);
    EXPECT_OK(sent);

    RoomPoll first;
    EXPECT_OK(mgr->pollRoomEvents(room, p[1], 0, 100, first));
    CHECK_EQUAL(1u, countType(first.events, RoomEventType::VOICE_SIGNAL));
    /* the target's cursor passing the signal acknowledges it */
    RoomPoll second;
    EXPECT_OK(mgr->pollRoomEvents(room, p[1], first.latestSeq, 100, second));
    CHECK_EQUAL(0u, second.events.size());
    CHECK_EQUAL(0u, countType(events(room, p[1], 0), RoomEventType::VOICE_SIGNAL));

    /* an unacknowledged signal times out instead of piling up */
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::ICE_CANDIDATE, "{}"));
    now += 59999;
    CHECK_EQUAL(1u, countType(events(room, p[1]), RoomEventType::VOICE_SIGNAL));
    now += 1;
    CHECK_EQUAL(0u, countType(events(room, p[1]), RoomEventType::VOICE_SIGNAL));

    /* the payload is nowhere in the view, the transcript or the archive */
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, secret));
    std::string tr;
    EXPECT_OK(mgr->getTranscript(room, p[1], tr));
    CHECK_TRUE(tr.find("never-store") == std::string::npos);
    EXPECT_OK(mgr->resign(room, p[1]));
    GameRecord rec = archived(room);
    CHECK_TRUE(rec.octiTranscript.find("never-store") == std::string::npos);
    CHECK_TRUE(rec.finalFen.find("never-store") == std::string::npos);
}

TEST(GameRoomManagerTest, OnlyTheTargetsOwnCursorAcknowledgesASignal)
{
    std::string room = start2P();
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, "{}"));
    RoomPoll mine;
    EXPECT_OK(mgr->pollRoomEvents(room, p[0], 0, 100, mine));
    RoomPoll anon;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, mine.latestSeq + 100, 100, anon));
    /* neither the sender's nor a spectator's far-future cursor consumed it */
    CHECK_EQUAL(1u, countType(events(room, p[1]), RoomEventType::VOICE_SIGNAL));
}

TEST(GameRoomManagerTest, VoicePendingSignalBufferIsBounded)
{
    RoomLimits lim;
    lim.maxPendingSignals = 3;
    mgr->setLimits(lim);
    std::string room = start2P();
    for (int i = 0; i < 5; ++i) {
        EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::ICE_CANDIDATE,
                                         "{\"n\":" + std::to_string(i) + "}"));
    }
    std::vector<RoomEvent> ev = events(room, p[1]);
    CHECK_EQUAL(3u, countType(ev, RoomEventType::VOICE_SIGNAL));
    std::vector<std::string> payloads;
    for (size_t i = 0; i < ev.size(); ++i) {
        if (ev[i].type == RoomEventType::VOICE_SIGNAL) {
            payloads.push_back(ev[i].text);
        }
    }
    STRCMP_EQUAL("{\"n\":2}", payloads[0].c_str());
    STRCMP_EQUAL("{\"n\":4}", payloads[2].c_str());
}

TEST(GameRoomManagerTest, VoicePayloadIsLimitedToFourKiB)
{
    std::string room = start2P();
    const size_t overhead = std::string("{\"sdp\":\"\"}").size();
    std::string atLimit = "{\"sdp\":\"" +
                          std::string(GameRoomManager::kMaxVoicePayloadBytes - overhead, 'a') + "\"}";
    CHECK_EQUAL(GameRoomManager::kMaxVoicePayloadBytes, atLimit.size());
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, atLimit));

    std::string over = "{\"sdp\":\"" +
                       std::string(GameRoomManager::kMaxVoicePayloadBytes - overhead + 1, 'a') + "\"}";
    CHECK_EQUAL(GameRoomManager::kMaxVoicePayloadBytes + 1, over.size());
    EXPECT_CODE(PAYLOAD_TOO_LARGE,
                mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, over));
    EXPECT_CODE(PAYLOAD_TOO_LARGE,
                mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER,
                                       std::string(1 << 20, '{')));

    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, ""));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, "{not json"));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER,
                                       std::string(3000, '[')));
    CHECK_EQUAL(1u, countType(events(room, p[1]), RoomEventType::VOICE_SIGNAL));
}

TEST(GameRoomManagerTest, VoiceTargetsMustBeEligibleParticipants)
{
    std::string room = start2P();
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], p[5], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], 0, VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_NOT_PARTICIPANT,
                mgr->submitVoiceSignal(room, p[5], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_NOT_PARTICIPANT,
                mgr->submitVoiceMeta(room, p[5], VoiceMetaAction::JOINED_VOICE));
    EXPECT_CODE(NOT_FOUND,
                mgr->submitVoiceSignal("g-none", p[0], p[1], VoiceSignalType::SDP_OFFER, "{}"));
}

TEST(GameRoomManagerTest, DisabledVoicePolicyRefusesAllSignalingAndMeta)
{
    GameTableConfig c = cfg();
    c.voicePolicy = VoicePolicy::DISABLED;
    std::string room = startTable(c, 2);
    EXPECT_CODE(FORBIDDEN_POLICY,
                mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::JOINED_VOICE));
    CHECK_EQUAL(0u, view(room).voicePeers.size());
    EXPECT_OK(mgr->updateRoomPolicies(room, p[1], ObserverPolicy::OPEN, VoicePolicy::PLAYERS_ONLY));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::JOINED_VOICE));
}

TEST(GameRoomManagerTest, PlayersOnlyVoiceExcludesObservers)
{
    std::string room = start2P();            /* default PLAYERS_ONLY */
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_CODE(FORBIDDEN_POLICY,
                mgr->submitVoiceSignal(room, p[2], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::JOINED_VOICE));
    /* a player cannot open a link to an observer either */
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->submitVoiceSignal(room, p[0], p[2], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::JOINED_VOICE));
}

TEST(GameRoomManagerTest, AllAllowedVoiceLetsUnmutedObserversTalkUnlessSilenced)
{
    GameTableConfig c = cfg();
    c.voicePolicy = VoicePolicy::ALL_ALLOWED;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::JOINED_VOICE));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[2], p[0], VoiceSignalType::SDP_OFFER, "{}"));

    /* a voice mute silences signaling and meta, and drops the peer */
    EXPECT_OK(mgr->muteUser(room, p[0], p[2], false, true));
    EXPECT_CODE(FORBIDDEN_MUTED,
                mgr->submitVoiceSignal(room, p[2], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_MUTED, mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::MIC_UNMUTED));
    CHECK_EQUAL(0u, view(room).voicePeers.size());
    /* text chat is a separate switch */
    EXPECT_OK(mgr->postBoardChat(room, p[2], ChatScope::ALL, "can still type"));
    EXPECT_OK(mgr->muteUser(room, p[1], p[2], false, false));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::JOINED_VOICE));

    /* silent observers never speak, even where voice is open to all */
    EXPECT_OK(mgr->updateRoomPolicies(room, p[0], ObserverPolicy::SILENT_OBSERVERS,
                                      VoicePolicy::ALL_ALLOWED));
    EXPECT_CODE(FORBIDDEN_POLICY,
                mgr->submitVoiceSignal(room, p[2], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::MIC_MUTED));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[1], VoiceMetaAction::JOINED_VOICE));
}

TEST(GameRoomManagerTest, VoiceMetaIsRecordedAndTracksPeers)
{
    std::string room = start2P();
    EXPECT_CODE(INVALID_STATE, mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::MIC_MUTED));
    EXPECT_CODE(INVALID_STATE, mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::LEFT_VOICE));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::JOINED_VOICE));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::MIC_MUTED));
    RoomView v = view(room);
    CHECK_EQUAL(1u, v.voicePeers.size());
    CHECK_TRUE(v.voicePeers[0].micMuted);
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::MIC_UNMUTED));
    CHECK_FALSE(view(room).voicePeers[0].micMuted);
    EXPECT_OK(mgr->submitVoiceMeta(room, p[1], VoiceMetaAction::JOINED_VOICE));
    CHECK_EQUAL(2u, view(room).voicePeers.size());
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::LEFT_VOICE));
    CHECK_EQUAL(1u, view(room).voicePeers.size());

    std::vector<std::string> actions;
    std::vector<RoomEvent> ev = events(room, 0);
    for (size_t i = 0; i < ev.size(); ++i) {
        if (ev[i].type == RoomEventType::VOICE_META) {
            actions.push_back(ev[i].detail);
            CHECK_TRUE(ev[i].text.empty());              /* no payload at all */
        }
    }
    CHECK_EQUAL(5u, actions.size());
    STRCMP_EQUAL("JOINED_VOICE", actions[0].c_str());
    STRCMP_EQUAL("MIC_MUTED", actions[1].c_str());
    STRCMP_EQUAL("MIC_UNMUTED", actions[2].c_str());
    STRCMP_EQUAL("JOINED_VOICE", actions[3].c_str());
    STRCMP_EQUAL("LEFT_VOICE", actions[4].c_str());
}

TEST(GameRoomManagerTest, ChangingPoliciesTearsDownVoiceAndDropsStaleSignals)
{
    GameTableConfig c = cfg();
    c.voicePolicy = VoicePolicy::ALL_ALLOWED;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[0], VoiceMetaAction::JOINED_VOICE));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::JOINED_VOICE));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[2], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[0], p[1], VoiceSignalType::SDP_OFFER, "{}"));
    CHECK_EQUAL(2u, view(room).voicePeers.size());

    RoomResult upd = mgr->updateRoomPolicies(room, p[1], ObserverPolicy::OPEN,
                                             VoicePolicy::PLAYERS_ONLY);
    EXPECT_OK(upd);
    CHECK_TRUE(upd.seq > 0);
    RoomView v = view(room);
    CHECK_EQUAL(1u, v.voicePeers.size());
    LONGS_EQUAL(p[0], v.voicePeers[0].playerId);
    CHECK_EQUAL(0u, countType(events(room, p[2]), RoomEventType::VOICE_SIGNAL));
    CHECK_EQUAL(1u, countType(events(room, p[1]), RoomEventType::VOICE_SIGNAL));
    std::vector<RoomEvent> ev = events(room, 0);
    bool sawPolicy = false;
    for (size_t i = 0; i < ev.size(); ++i) {
        if (ev[i].type == RoomEventType::POLICY_UPDATED) {
            sawPolicy = true;
            STRCMP_EQUAL("observer_policy=OPEN;voice_policy=PLAYERS_ONLY", ev[i].detail.c_str());
        }
    }
    CHECK_TRUE(sawPolicy);

    EXPECT_OK(mgr->updateRoomPolicies(room, p[0], ObserverPolicy::OPEN, VoicePolicy::DISABLED));
    CHECK_EQUAL(0u, view(room).voicePeers.size());
    CHECK_EQUAL(0u, countType(events(room, p[1]), RoomEventType::VOICE_SIGNAL));
}

/* ---- moderation: ban, mute, policies ----------------------------------- */

TEST(GameRoomManagerTest, BanningAnObserverBlocksThemEverywhereAndPurgesTheirTraffic)
{
    GameTableConfig c = cfg();
    c.voicePolicy = VoicePolicy::ALL_ALLOWED;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->enterLobby(p[2]));
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->observeRoom(room, p[3]));
    EXPECT_OK(mgr->postBoardChat(room, p[2], ChatScope::ALL, "spam spam spam"));
    EXPECT_OK(mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::JOINED_VOICE));
    EXPECT_OK(mgr->submitVoiceSignal(room, p[2], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_OK(mgr->postBoardChat(room, p[3], ChatScope::ALL, "innocent remark"));
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "hello"));
    STRCMP_EQUAL("OBSERVING", toString(mgr->presenceOf(p[2])));

    std::vector<RoomEvent> before = events(room, p[1]);
    CHECK_EQUAL(1u, countType(before, RoomEventType::VOICE_META));
    CHECK_EQUAL(3u, countType(before, RoomEventType::CHAT));
    CHECK_EQUAL(2u, countType(before, RoomEventType::OBSERVER_JOINED));

    RoomResult ban = mgr->banUser(room, p[0], p[2], "spamming the board");
    EXPECT_OK(ban);
    CHECK_TRUE(ban.seq > 0);
    CHECK_TRUE(mgr->isBanned(room, p[2]));
    CHECK_FALSE(mgr->isBanned(room, p[3]));
    std::vector<int64_t> banned = mgr->bannedPlayers(room);
    CHECK_EQUAL(1u, banned.size());
    LONGS_EQUAL(p[2], banned[0]);
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[2])));

    /* every door is shut for the banned observer */
    RoomView rv;
    RoomPoll rp;
    std::string tr;
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->getRoomView(room, p[2], rv));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->pollRoomEvents(room, p[2], 0, 100, rp));
    CHECK_EQUAL(0u, rp.events.size());
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->getTranscript(room, p[2], tr));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->postBoardChat(room, p[2], ChatScope::ALL, "i'm back"));
    EXPECT_CODE(FORBIDDEN_BANNED,
                mgr->submitVoiceSignal(room, p[2], p[0], VoiceSignalType::SDP_OFFER, "{}"));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->submitVoiceMeta(room, p[2], VoiceMetaAction::JOINED_VOICE));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->observeRoom(room, p[2]));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->joinTable(room, p[2], 0, ""));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->banUser(room, p[2], p[3], ""));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->muteUser(room, p[2], p[3], true, false));
    EXPECT_CODE(FORBIDDEN_BANNED,
                mgr->updateRoomPolicies(room, p[2], ObserverPolicy::OPEN, VoicePolicy::DISABLED));

    /* the room's history lost everything the observer produced */
    std::vector<RoomEvent> after = events(room, p[1]);
    CHECK_EQUAL(0u, countType(after, RoomEventType::VOICE_META));
    CHECK_EQUAL(2u, countType(after, RoomEventType::CHAT));
    CHECK_EQUAL(1u, countType(after, RoomEventType::OBSERVER_JOINED));
    for (size_t i = 0; i < after.size(); ++i) {
        if (after[i].type == RoomEventType::CHAT) {
            CHECK_TRUE(after[i].playerId != p[2]);
            CHECK_TRUE(after[i].text.find("spam") == std::string::npos);
        }
    }
    CHECK_EQUAL(0u, countType(events(room, p[0]), RoomEventType::VOICE_SIGNAL));
    std::vector<RoomEvent> bans;
    for (size_t i = 0; i < after.size(); ++i) {
        if (after[i].type == RoomEventType::OBSERVER_BANNED) {
            bans.push_back(after[i]);
        }
    }
    CHECK_EQUAL(1u, bans.size());
    LONGS_EQUAL(p[0], bans[0].playerId);
    LONGS_EQUAL(p[2], bans[0].targetPlayerId);
    STRCMP_EQUAL("spamming the board", bans[0].text.c_str());
    for (size_t i = 1; i < after.size(); ++i) {
        CHECK_TRUE(after[i].seq > after[i - 1].seq);      /* gaps, never reuse */
    }

    RoomView v = view(room, p[1]);
    CHECK_EQUAL(1u, v.observerIds.size());
    LONGS_EQUAL(p[3], v.observerIds[0]);
    CHECK_EQUAL(0u, v.voicePeers.size());

    /* a ban is per room: the same player can use another table */
    RoomResult other = mgr->createTable(p[4], cfg());
    EXPECT_OK(other);
    EXPECT_OK(mgr->joinTable(other.roomId, p[5], 0, ""));
    EXPECT_OK(mgr->observeRoom(other.roomId, p[2]));
    view(other.roomId, p[2]);
}

TEST(GameRoomManagerTest, OnlySeatedPlayersMayBan)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->observeRoom(room, p[3]));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->banUser(room, p[2], p[3], "mine"));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->banUser(room, p[5], p[3], "stranger"));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->banUser(room, p[2], p[0], "coup"));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->banUser(room, p[2], p[1], "coup"));
    CHECK_EQUAL(0u, mgr->bannedPlayers(room).size());
    EXPECT_CODE(NOT_FOUND, mgr->banUser("g-none", p[0], p[2], ""));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->banUser(room, 909090, p[2], ""));
}

TEST(GameRoomManagerTest, ActiveOpponentsAndTeammatesCannotBeBannedMidGame)
{
    std::string room = start2P();
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->banUser(room, p[0], p[1], "tilted"));
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->banUser(room, p[1], p[0], "tilted"));
    CHECK_EQUAL(0u, mgr->bannedPlayers(room).size());
    view(room, p[1]);
    EXPECT_OK(mgr->postBoardChat(room, p[1], ChatScope::ALL, "still here"));
    EXPECT_OK(mgr->submitMove(room, p[1], "37+e"));

    RoomResult c = mgr->createTable(p[2], cfg(GAME_MODE_4));
    EXPECT_OK(c);
    EXPECT_OK(mgr->joinTable(c.roomId, p[3], 0, ""));
    EXPECT_OK(mgr->joinTable(c.roomId, p[4], 0, ""));
    EXPECT_OK(mgr->joinTable(c.roomId, p[5], 0, ""));
    /* seats: p2, p3, p4, p5 -> p2/p4 form a team */
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->banUser(c.roomId, p[2], p[4], "teammate"));
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->banUser(c.roomId, p[2], p[3], "opponent"));
    /* ... and not once the game is over either */
    EXPECT_OK(mgr->resign(c.roomId, p[2]));
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->banUser(c.roomId, p[3], p[2], "sore loser"));
}

TEST(GameRoomManagerTest, HostMayKickAndBanASeatBeforeTheGameStarts)
{
    RoomResult c = mgr->createTable(p[0], cfg(GAME_MODE_4));
    EXPECT_OK(c);
    EXPECT_OK(mgr->joinTable(c.roomId, p[1], 0, ""));
    EXPECT_OK(mgr->joinTable(c.roomId, p[2], 0, ""));
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->banUser(c.roomId, p[1], p[2], "not host"));
    EXPECT_OK(mgr->banUser(c.roomId, p[0], p[2], "griefer"));
    RoomView v = view(c.roomId, p[0]);
    LONGS_EQUAL(0, v.seats[2].playerId);
    CHECK_TRUE(mgr->isBanned(c.roomId, p[2]));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[2])));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->joinTable(c.roomId, p[2], 0, ""));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->banUser(c.roomId, p[0], p[0], ""));

    /* the table fills with other players and starts */
    EXPECT_OK(mgr->joinTable(c.roomId, p[3], 0, ""));
    RoomResult last = mgr->joinTable(c.roomId, p[4], 0, "");
    EXPECT_OK(last);
    CHECK_TRUE(last.gameStarted);
}

TEST(GameRoomManagerTest, BanEdgeCases)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->banUser(room, p[0], p[0], "me"));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->banUser(room, p[0], 777777, ""));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->banUser(room, p[0], p[2], std::string(GameRoomManager::kMaxReasonBytes + 1, 'r')));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->banUser(room, p[0], p[2], "bad\nreason"));
    CHECK_FALSE(mgr->isBanned(room, p[2]));

    RoomResult first = mgr->banUser(room, p[0], p[2], "");
    EXPECT_OK(first);
    size_t bannedEvents = countType(events(room, p[0]), RoomEventType::OBSERVER_BANNED);
    CHECK_EQUAL(1u, bannedEvents);
    RoomResult again = mgr->banUser(room, p[1], p[2], "twice");
    EXPECT_OK(again);
    CHECK_EQUAL(0, static_cast<int>(again.seq));
    STRCMP_EQUAL("already banned", again.message.c_str());
    CHECK_EQUAL(1u, countType(events(room, p[0]), RoomEventType::OBSERVER_BANNED));

    /* a stranger who never entered the room can be banned pre-emptively */
    EXPECT_OK(mgr->banUser(room, p[0], p[6], "known troll"));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->observeRoom(room, p[6]));
    CHECK_FALSE(mgr->isBanned("g-none", p[2]));
    CHECK_EQUAL(0u, mgr->bannedPlayers("g-none").size());
}

TEST(GameRoomManagerTest, BansCanBeIssuedAfterTheGameAndCarryIntoARematch)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->resign(room, p[1]));
    EXPECT_OK(mgr->banUser(room, p[0], p[2], "post-game abuse"));
    RoomView hidden;
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->getRoomView(room, p[2], hidden));

    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::REMATCH_REQUEST));
    RoomResult rm = mgr->respondProposal(room, p[1], ProposalType::REMATCH_REQUEST, true);
    EXPECT_OK(rm);
    CHECK_TRUE(mgr->isBanned(rm.roomId, p[2]));
    EXPECT_CODE(FORBIDDEN_BANNED, mgr->observeRoom(rm.roomId, p[2]));
}

TEST(GameRoomManagerTest, NoObserversPolicyHidesAnInProgressRoom)
{
    GameTableConfig c = cfg();
    c.observerPolicy = ObserverPolicy::NO_OBSERVERS;
    RoomResult t = mgr->createTable(p[0], c);
    EXPECT_OK(t);
    view(t.roomId, p[2]);                       /* a waiting table is visible */
    EXPECT_OK(mgr->joinTable(t.roomId, p[1], 0, ""));

    RoomView v;
    RoomPoll rp;
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->observeRoom(t.roomId, p[2]));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->getRoomView(t.roomId, p[2], v));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->getRoomView(t.roomId, 0, v));
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->pollRoomEvents(t.roomId, p[2], 0, 10, rp));
    std::string tr;
    EXPECT_CODE(FORBIDDEN_POLICY, mgr->getTranscript(t.roomId, p[2], tr));
    view(t.roomId, p[0]);
    view(t.roomId, p[1]);

    EXPECT_OK(mgr->resign(t.roomId, p[0]));
    view(t.roomId, p[2]);                       /* finished rooms are public */
    EXPECT_OK(mgr->observeRoom(t.roomId, p[2]));
}

TEST(GameRoomManagerTest, SwitchingToNoObserversEvictsWithoutBanning)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->observeRoom(room, p[3]));
    CHECK_EQUAL(2u, view(room).observerIds.size());
    EXPECT_OK(mgr->updateRoomPolicies(room, p[1], ObserverPolicy::NO_OBSERVERS,
                                      VoicePolicy::PLAYERS_ONLY));
    CHECK_EQUAL(0u, view(room, p[0]).observerIds.size());
    std::vector<RoomEvent> ev = events(room, p[0]);
    size_t evicted = 0;
    for (size_t i = 0; i < ev.size(); ++i) {
        if (ev[i].type == RoomEventType::OBSERVER_LEFT && ev[i].detail == "evicted") {
            ++evicted;
        }
    }
    CHECK_EQUAL(2u, evicted);
    CHECK_FALSE(mgr->isBanned(room, p[2]));
    EXPECT_CODE(FORBIDDEN_NOT_PARTICIPANT, mgr->postBoardChat(room, p[2], ChatScope::ALL, "hi"));
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[2])));
    EXPECT_OK(mgr->updateRoomPolicies(room, p[0], ObserverPolicy::OPEN, VoicePolicy::PLAYERS_ONLY));
    EXPECT_OK(mgr->observeRoom(room, p[2]));
}

TEST(GameRoomManagerTest, MuteChatAndVoiceAreIndependentAndReversible)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    RoomResult m = mgr->muteUser(room, p[0], p[2], true, false);
    EXPECT_OK(m);
    CHECK_TRUE(m.seq > 0);
    EXPECT_CODE(FORBIDDEN_MUTED, mgr->postBoardChat(room, p[2], ChatScope::ALL, "hmm"));
    RoomResult dup = mgr->muteUser(room, p[1], p[2], true, false);
    EXPECT_OK(dup);
    CHECK_EQUAL(0, static_cast<int>(dup.seq));           /* unchanged: no event */
    EXPECT_OK(mgr->muteUser(room, p[0], p[2], false, false));
    EXPECT_OK(mgr->postBoardChat(room, p[2], ChatScope::ALL, "unmuted"));

    std::vector<std::string> details;
    std::vector<RoomEvent> ev = events(room, 0);
    for (size_t i = 0; i < ev.size(); ++i) {
        if (ev[i].type == RoomEventType::USER_MUTED) {
            details.push_back(ev[i].detail);
            LONGS_EQUAL(p[2], ev[i].targetPlayerId);
        }
    }
    CHECK_EQUAL(2u, details.size());
    STRCMP_EQUAL("mute_chat=1;mute_voice=0", details[0].c_str());
    STRCMP_EQUAL("mute_chat=0;mute_voice=0", details[1].c_str());
}

TEST(GameRoomManagerTest, MuteRules)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_OK(mgr->observeRoom(room, p[3]));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->muteUser(room, p[2], p[3], true, true));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED, mgr->muteUser(room, p[6], p[3], true, true));
    EXPECT_CODE(INVALID_ARGUMENT, mgr->muteUser(room, p[0], p[0], true, true));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->muteUser(room, p[0], 55555, true, true));
    /* mutes are room-wide, so a seated opponent cannot be silenced */
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->muteUser(room, p[0], p[1], true, false));
    EXPECT_CODE(FORBIDDEN_SEATED_TARGET, mgr->muteUser(room, p[0], p[1], false, true));
    EXPECT_OK(mgr->muteUser(room, p[0], p[1], false, false));     /* unmute is harmless */
    EXPECT_OK(mgr->postBoardChat(room, p[1], ChatScope::ALL, "free speech"));
    EXPECT_CODE(NOT_FOUND, mgr->muteUser("g-none", p[0], p[2], true, true));
}

TEST(GameRoomManagerTest, PolicyUpdatesNeedASeatAndEmitOnlyOnChange)
{
    std::string room = start2P();
    EXPECT_OK(mgr->observeRoom(room, p[2]));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED,
                mgr->updateRoomPolicies(room, p[2], ObserverPolicy::OPEN, VoicePolicy::DISABLED));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED,
                mgr->updateRoomPolicies(room, p[6], ObserverPolicy::OPEN, VoicePolicy::DISABLED));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->updateRoomPolicies(room, p[0], static_cast<ObserverPolicy>(7),
                                        VoicePolicy::DISABLED));
    EXPECT_CODE(INVALID_ARGUMENT,
                mgr->updateRoomPolicies(room, p[0], ObserverPolicy::OPEN,
                                        static_cast<VoicePolicy>(-1)));
    RoomResult same = mgr->updateRoomPolicies(room, p[0], ObserverPolicy::OPEN,
                                              VoicePolicy::PLAYERS_ONLY);
    EXPECT_OK(same);
    CHECK_EQUAL(0, static_cast<int>(same.seq));
    CHECK_EQUAL(0u, countType(events(room, 0), RoomEventType::POLICY_UPDATED));
    RoomResult chg = mgr->updateRoomPolicies(room, p[1], ObserverPolicy::SILENT_OBSERVERS,
                                             VoicePolicy::ALL_ALLOWED);
    EXPECT_OK(chg);
    CHECK_TRUE(chg.seq > 0);
    RoomView v = view(room);
    STRCMP_EQUAL("SILENT_OBSERVERS", toString(v.config.observerPolicy));
    STRCMP_EQUAL("ALL_ALLOWED", toString(v.config.voicePolicy));
    EXPECT_CODE(NOT_FOUND, mgr->updateRoomPolicies("g-none", p[0], ObserverPolicy::OPEN,
                                                   VoicePolicy::PLAYERS_ONLY));
}

/* ---- event stream ------------------------------------------------------ */

TEST(GameRoomManagerTest, EventSequenceIsMonotonicAndCursorPollingHonoursBoundaries)
{
    std::string room = start2P();
    EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "one"));
    EXPECT_OK(mgr->postBoardChat(room, p[1], ChatScope::ALL, "two"));
    EXPECT_OK(mgr->submitMove(room, p[0], "33+a"));

    RoomPoll all;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, 0, 500, all));
    CHECK_TRUE(all.events.size() >= 6);                 /* created, 2 seats, started, 2 chats, move */
    LONGS_EQUAL(1, all.events.front().seq);
    for (size_t i = 1; i < all.events.size(); ++i) {
        CHECK_TRUE(all.events[i].seq > all.events[i - 1].seq);
    }
    LONGS_EQUAL(all.events.back().seq, all.latestSeq);
    LONGS_EQUAL(all.latestSeq, view(room).latestSeq);
    CHECK_FALSE(all.gap);
    CHECK_FALSE(all.hasMore);

    RoomPoll none;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, all.latestSeq, 500, none));
    CHECK_EQUAL(0u, none.events.size());
    LONGS_EQUAL(all.latestSeq, none.latestSeq);

    RoomPoll one;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, all.latestSeq - 1, 500, one));
    CHECK_EQUAL(1u, one.events.size());
    CHECK_TRUE(one.events[0].type == RoomEventType::MOVE);

    RoomPoll future;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, all.latestSeq + 1000, 500, future));
    CHECK_EQUAL(0u, future.events.size());

    RoomPoll paged;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, 0, 2, paged));
    CHECK_EQUAL(2u, paged.events.size());
    CHECK_TRUE(paged.hasMore);
    RoomPoll next;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, paged.events.back().seq, 500, next));
    CHECK_EQUAL(all.events.size() - 2u, next.events.size());
    RoomPoll zeroLimit;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, 0, 0, zeroLimit));
    CHECK_EQUAL(1u, zeroLimit.events.size());            /* limit is clamped to >= 1 */

    RoomPoll missing;
    EXPECT_CODE(NOT_FOUND, mgr->pollRoomEvents("g-none", 0, 0, 10, missing));
}

TEST(GameRoomManagerTest, OverflowingTheEventBufferReportsAGap)
{
    RoomLimits lim;
    lim.maxRoomEvents = 10;
    lim.boardChatBurst = 100000;
    mgr->setLimits(lim);
    std::string room = start2P();
    for (int i = 0; i < 30; ++i) {
        EXPECT_OK(mgr->postBoardChat(room, p[0], ChatScope::ALL, "c" + std::to_string(i)));
    }
    RoomPoll stale;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, 0, 500, stale));
    CHECK_TRUE(stale.gap);
    CHECK_EQUAL(10u, stale.events.size());
    RoomPoll fresh;
    EXPECT_OK(mgr->pollRoomEvents(room, 0, stale.latestSeq - 3, 500, fresh));
    CHECK_FALSE(fresh.gap);
    CHECK_EQUAL(3u, fresh.events.size());
}

/* ---- archive, transcript, housekeeping -------------------------------- */

TEST(GameRoomManagerTest, TranscriptCarriesRulesetAndPlayerMetadata)
{
    GameTableConfig c = cfg(GAME_MODE_2);
    c.useSuperProngs = true;
    c.useEdgelessBoard = true;
    c.bases = GAME_BASES_1;
    std::string room = startTable(c, 2);
    EXPECT_OK(mgr->resign(room, p[0]));
    GameRecord rec = archived(room);
    CHECK_TRUE(rec.superProngs);
    CHECK_TRUE(rec.edgeless);
    CHECK_EQUAL(1, rec.gameBases);
    auto jrec = nlohmann::json::parse(rec.octiTranscript);
    CHECK_EQUAL("octi.transcript.v3", jrec.value("format", ""));
    CHECK_EQUAL("2P", jrec.value("mode", ""));
    CHECK_EQUAL(1, jrec.value("bases", 0));
    CHECK_TRUE(jrec.value("superProngs", false));
    CHECK_TRUE(jrec.value("edgeless", false));
    CHECK_EQUAL("A", jrec.value("startPlayer", ""));
    CHECK_EQUAL("RESIGNED", jrec.value("status", ""));
    CHECK_EQUAL("B", jrec.value("winner", ""));
    CHECK_EQUAL("RESIGNATION", jrec.value("termination", ""));
    CHECK_EQUAL("CC-BY-4.0", jrec.value("license", ""));
    CHECK_EQUAL("Octi game data, Charles Chiou (selfso.com)", jrec.value("attribution", ""));
    CHECK_EQUAL(room, jrec.value("gameId", ""));
    CHECK_TRUE(jrec.contains("players") && jrec["players"].is_array());
    CHECK_EQUAL(2, static_cast<int>(jrec["players"].size()));
    CHECK_EQUAL(handle[0], jrec["players"][0].value("handle", ""));
    CHECK_EQUAL(handle[1], jrec["players"][1].value("handle", ""));
    checkTranscriptReplays(room, rec);
}

TEST(GameRoomManagerTest, FailedArchiveIsKeptAndRetriedByTick)
{
    std::string room = start2P();
    store->close();
    EXPECT_OK(mgr->resign(room, p[1]));
    RoomView v = view(room);
    STRCMP_EQUAL("FINISHED", toString(v.phase));
    CHECK_FALSE(v.archived);
    CHECK_FALSE(v.archiveError.empty());

    /* still failing: the retry is attempted, but never over-counted */
    TickReport t1 = mgr->tick();
    CHECK_EQUAL(1, t1.archiveRetries);
    CHECK_FALSE(view(room).archived);

    std::string err;
    CHECK_TRUE_TEXT(store->openOrCreate(dir->path() + "/data/accounts.db", err), err.c_str());
    CHECK_EQUAL(1200, profile(p[0]).elo2p);               /* nothing applied yet */
    TickReport t2 = mgr->tick();
    CHECK_EQUAL(1, t2.archiveRetries);
    v = view(room);
    CHECK_TRUE(v.archived);
    CHECK_TRUE(v.archiveError.empty());
    GameRecord rec = archived(room);
    STRCMP_EQUAL("RESIGNATION", rec.termination.c_str());
    CHECK_EQUAL(1216, profile(p[0]).elo2p);
    CHECK_EQUAL(1184, profile(p[1]).elo2p);
    CHECK_EQUAL(1, profile(p[0]).gamesPlayed);

    TickReport t3 = mgr->tick();                          /* exactly once */
    CHECK_EQUAL(0, t3.archiveRetries);
    CHECK_EQUAL(1216, profile(p[0]).elo2p);
    CHECK_EQUAL(1, profile(p[0]).gamesPlayed);
}

TEST(GameRoomManagerTest, RoomIdsSkipIdsAlreadyArchived)
{
    GameRecord rec;
    rec.gameId = "g-20270115-0001";
    rec.gameMode = "2P";
    rec.gameBases = 3;
    rec.p1PlayerId = p[0];
    rec.p2PlayerId = p[1];
    rec.p1Handle = handle[0];
    rec.p2Handle = handle[1];
    rec.result = "DRAW";
    rec.termination = "AGREED_DRAW";
    rec.octiTranscript = "{\"attribution\":\"Octi game data, Charles Chiou (selfso.com)\",\"bases\":3,\"edgeless\":false,\"format\":\"octi.transcript.v3\",\"gameId\":\"g-20270115-0001\",\"license\":\"CC-BY-4.0\",\"mode\":\"2P\",\"plies\":[],\"source\":\"ONLINE\",\"startPlayer\":\"A\",\"status\":\"GAME_OVER\",\"superProngs\":false,\"termination\":\"AGREED_DRAW\",\"timeControl\":\"untimed\",\"winner\":\"NONE\"}";
    rec.finalFen = "2:3:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77";
    rec.startedAt = kWall0;
    rec.endedAt = kWall0;
    std::string err;
    CHECK(store->archiveGame(rec, err) == ArchiveStatus::OK);
    RoomResult c = mgr->createTable(p[2], cfg());
    EXPECT_OK(c);
    STRCMP_EQUAL("g-20270115-0002", c.roomId.c_str());
    wall += 86400;                                        /* next day, next prefix */
    RoomResult d = mgr->createTable(p[3], cfg());
    EXPECT_OK(d);
    STRCMP_EQUAL("g-20270116-0003", d.roomId.c_str());
}

TEST(GameRoomManagerTest, FinishedRoomsAreReapedOnlyAfterTheRetentionWindow)
{
    std::string room = start2P();
    EXPECT_OK(mgr->resign(room, p[0]));
    CHECK_TRUE(view(room).archived);
    now += 30LL * 60 * 1000 - 1;
    TickReport early = mgr->tick();
    CHECK_EQUAL(0, early.roomsReaped);
    CHECK_EQUAL(1u, mgr->roomCount());
    now += 1;
    TickReport due = mgr->tick();
    CHECK_EQUAL(1, due.roomsReaped);
    CHECK_EQUAL(0u, mgr->roomCount());
    RoomView gone;
    EXPECT_CODE(NOT_FOUND, mgr->getRoomView(room, 0, gone));
    archived(room);                                       /* the archive keeps it */
}

TEST(GameRoomManagerTest, UnarchivedFinishedRoomsSurviveReaping)
{
    std::string room = start2P();
    store->close();
    EXPECT_OK(mgr->resign(room, p[0]));
    now += 31LL * 60 * 1000;
    mgr->tick();
    CHECK_EQUAL(1u, mgr->roomCount());                    /* the only copy is kept */
    std::string err;
    CHECK_TRUE_TEXT(store->openOrCreate(dir->path() + "/data/accounts.db", err), err.c_str());
    mgr->tick();                                          /* archives */
    mgr->tick();                                          /* reaps */
    CHECK_EQUAL(0u, mgr->roomCount());
    archived(room);
}

TEST(GameRoomManagerTest, AbandonedWaitingTablesAreClosed)
{
    EXPECT_OK(mgr->enterLobby(p[0]));
    RoomResult c = mgr->createTable(p[0], cfg());
    EXPECT_OK(c);
    now += 60LL * 60 * 1000 - 1;
    CHECK_EQUAL(0, mgr->tick().roomsReaped);
    now += 1;
    CHECK_EQUAL(1, mgr->tick().roomsReaped);
    CHECK_EQUAL(0u, mgr->roomCount());
    STRCMP_EQUAL("IN_LOBBY", toString(mgr->presenceOf(p[0])));
    LobbyPoll lp = mgr->pollLobbyEvents(0, 0, 500);
    bool closed = false;
    for (size_t i = 0; i < lp.events.size(); ++i) {
        closed = closed || (lp.events[i].type == LobbyEventType::TABLE_CLOSED &&
                            lp.events[i].roomId == c.roomId);
    }
    CHECK_TRUE(closed);
    EXPECT_OK(mgr->createTable(p[0], cfg()));
}

/* ---- matchmaking through the manager ----------------------------------- */

TEST(GameRoomManagerTest, MatchmakingSpawnsARatedRoomForACompatiblePair)
{
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    now += 1000;
    EXPECT_OK(mgr->enqueueMatchmaking(p[1], MatchRuleset()));
    EXPECT_CODE(CONFLICT, mgr->enqueueMatchmaking(p[1], MatchRuleset()));
    MatchmakingStatus waiting = mgr->matchmakingStatus(p[1]);
    CHECK_TRUE(waiting.queued);
    CHECK_EQUAL(100, waiting.window);
    CHECK_EQUAL(1200, waiting.rating);

    TickReport rep = mgr->tick();
    CHECK_EQUAL(1, rep.matchesSpawned);
    MatchmakingStatus st = mgr->matchmakingStatus(p[0]);
    CHECK_FALSE(st.queued);
    CHECK_FALSE(st.matchedRoomId.empty());
    STRCMP_EQUAL(st.matchedRoomId.c_str(), mgr->matchmakingStatus(p[1]).matchedRoomId.c_str());

    RoomView v = view(st.matchedRoomId, p[0]);
    STRCMP_EQUAL("IN_PROGRESS", toString(v.phase));
    CHECK_TRUE(v.config.rated);
    CHECK_FALSE(v.config.allowTakeback);
    CHECK_TRUE(v.config.passcode.empty());
    LONGS_EQUAL(p[0], v.seats[0].playerId);              /* longer waiter moves first */
    LONGS_EQUAL(p[1], v.seats[1].playerId);
    STRCMP_EQUAL("PLAYING", toString(mgr->presenceOf(p[0])));
    EXPECT_OK(mgr->submitMove(st.matchedRoomId, p[0], "33+a"));

    /* seated players cannot queue again, and the queue is empty now */
    EXPECT_CODE(ALREADY_SEATED, mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    EXPECT_CODE(INVALID_STATE, mgr->dequeueMatchmaking(p[0]));
    CHECK_EQUAL(0, mgr->tick().matchesSpawned);

    EXPECT_OK(mgr->resign(st.matchedRoomId, p[1]));
    CHECK_EQUAL(1216, profile(p[0]).elo2p);               /* matchmaking games are rated */
}

TEST(GameRoomManagerTest, MatchmakingRatingWindowWidensWithWaiting)
{
    setRating2p(p[1], 1700);                              /* 500 apart */
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    EXPECT_OK(mgr->enqueueMatchmaking(p[1], MatchRuleset()));
    CHECK_EQUAL(1700, mgr->matchmakingStatus(p[1]).rating);
    CHECK_EQUAL(0, mgr->tick().matchesSpawned);
    now += 79999;                                         /* window 100 + 50*7 = 450 */
    CHECK_EQUAL(0, mgr->tick().matchesSpawned);
    CHECK_EQUAL(450, mgr->matchmakingStatus(p[0]).window);
    now += 1;                                             /* window 500 */
    CHECK_EQUAL(1, mgr->tick().matchesSpawned);
}

TEST(GameRoomManagerTest, MatchmakingKeepsModesAndRulesetsApart)
{
    MatchRuleset ofk;
    ofk.mode = GAME_MODE_OFK;
    MatchRuleset timed;
    timed.initialMs = 60000;
    timed.incrementMs = 1000;
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    EXPECT_OK(mgr->enqueueMatchmaking(p[1], ofk));
    EXPECT_OK(mgr->enqueueMatchmaking(p[2], timed));
    CHECK_EQUAL(0, mgr->tick().matchesSpawned);
    EXPECT_OK(mgr->enqueueMatchmaking(p[3], timed));
    CHECK_EQUAL(1, mgr->tick().matchesSpawned);
    RoomView v = view(mgr->matchmakingStatus(p[2]).matchedRoomId);
    CHECK_TRUE(v.config.timeControl.enabled);
    LONGS_EQUAL(60000, v.config.timeControl.initialMs);
    LONGS_EQUAL(1000, v.config.timeControl.incrementMs);
    LONGS_EQUAL(60000, v.seats[0].clockMs);
    CHECK_TRUE(mgr->matchmakingStatus(p[0]).queued);
    CHECK_TRUE(mgr->matchmakingStatus(p[1]).queued);

    MatchRuleset invalid;
    invalid.mode = GAME_MODE_UNDEF;
    EXPECT_CODE(INVALID_ARGUMENT, mgr->enqueueMatchmaking(p[4], invalid));
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->enqueueMatchmaking(31337, MatchRuleset()));
}

TEST(GameRoomManagerTest, FourPlayerMatchmakingFillsBalancedSeats)
{
    MatchRuleset r4;
    r4.mode = GAME_MODE_4;
    for (int i = 0; i < 4; ++i) {
        EXPECT_OK(mgr->enqueueMatchmaking(p[static_cast<size_t>(i)], r4));
        now += 10;
    }
    CHECK_EQUAL(1, mgr->tick().matchesSpawned);
    RoomView v = view(mgr->matchmakingStatus(p[0]).matchedRoomId);
    CHECK_EQUAL(4u, v.seats.size());
    STRCMP_EQUAL("IN_PROGRESS", toString(v.phase));
    /* equal ratings: order of arrival, laid out strongest/second/weakest/third */
    LONGS_EQUAL(p[0], v.seats[0].playerId);
    LONGS_EQUAL(p[1], v.seats[1].playerId);
    LONGS_EQUAL(p[3], v.seats[2].playerId);
    LONGS_EQUAL(p[2], v.seats[3].playerId);
    for (int i = 0; i < 4; ++i) {
        STRCMP_EQUAL("PLAYING", toString(mgr->presenceOf(p[static_cast<size_t>(i)])));
    }
}

TEST(GameRoomManagerTest, LeavingTheQueueOrTakingASeatCancelsTheTicket)
{
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    EXPECT_OK(mgr->dequeueMatchmaking(p[0]));
    EXPECT_CODE(INVALID_STATE, mgr->dequeueMatchmaking(p[0]));
    CHECK_FALSE(mgr->matchmakingStatus(p[0]).queued);

    EXPECT_OK(mgr->enqueueMatchmaking(p[1], MatchRuleset()));
    RoomResult c = mgr->createTable(p[1], cfg());
    EXPECT_OK(c);
    CHECK_FALSE(mgr->matchmakingStatus(p[1]).queued);       /* seating dequeues */
    EXPECT_OK(mgr->enqueueMatchmaking(p[2], MatchRuleset()));
    CHECK_EQUAL(0, mgr->tick().matchesSpawned);
    EXPECT_OK(mgr->joinTable(c.roomId, p[2], 0, ""));
    CHECK_FALSE(mgr->matchmakingStatus(p[2]).queued);
    EXPECT_CODE(UNKNOWN_PLAYER, mgr->dequeueMatchmaking(31337));
}

TEST(GameRoomManagerTest, MatchmakingUsesTheModeSpecificRating)
{
    setRating2p(p[0], 1450);
    MatchRuleset ofk;
    ofk.mode = GAME_MODE_OFK;
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], ofk));
    CHECK_EQUAL(1200, mgr->matchmakingStatus(p[0]).rating);
    EXPECT_OK(mgr->dequeueMatchmaking(p[0]));
    EXPECT_OK(mgr->enqueueMatchmaking(p[0], MatchRuleset()));
    CHECK_EQUAL(1450, mgr->matchmakingStatus(p[0]).rating);
}

/* ---- concurrency ------------------------------------------------------- */

TEST(GameRoomManagerTest, ConcurrentCallersSeeAConsistentRoom)
{
    RoomLimits lim;
    lim.boardChatBurst = 100000;
    lim.lobbyChatBurst = 100000;
    lim.maxLobbyChat = 1000;
    lim.maxLobbyEvents = 2000;
    mgr->setLimits(lim);
    std::string room = start2P();
    for (int i = 2; i < 8; ++i) {
        EXPECT_OK(mgr->observeRoom(room, p[static_cast<size_t>(i)]));
    }
    const int kThreads = 6;
    const int kPosts = 40;
    std::atomic<int> failures(0);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.push_back(std::thread([&, t]() {
            const int64_t me = p[static_cast<size_t>(2 + t)];
            for (int i = 0; i < kPosts; ++i) {
                if (!mgr->postBoardChat(room, me, ChatScope::ALL, "t" + std::to_string(t) + "-" + std::to_string(i)).ok()) {
                    ++failures;
                }
                if (!mgr->postLobbyChat(me, "lobby " + std::to_string(i)).ok()) {
                    ++failures;
                }
                RoomPoll rp;
                if (!mgr->pollRoomEvents(room, me, 0, 500, rp).ok()) {
                    ++failures;
                }
                RoomView rv;
                if (!mgr->getRoomView(room, me, rv).ok()) {
                    ++failures;
                }
                mgr->tick();
                mgr->lobbyState();
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    CHECK_EQUAL(0, failures.load());

    std::vector<RoomEvent> ev = events(room, p[0]);
    std::set<int64_t> seqs;
    size_t chats = 0;
    for (size_t i = 0; i < ev.size(); ++i) {
        CHECK_TRUE(seqs.insert(ev[i].seq).second);
        if (i > 0) {
            CHECK_TRUE(ev[i].seq > ev[i - 1].seq);
        }
        if (ev[i].type == RoomEventType::CHAT) {
            ++chats;
        }
    }
    CHECK_EQUAL(static_cast<size_t>(kThreads * kPosts), chats);
    size_t lobbyChats = 0;
    LobbyPoll lp = mgr->pollLobbyEvents(0, 0, 500);
    for (size_t i = 0; i < lp.events.size(); ++i) {
        if (lp.events[i].type == LobbyEventType::LOBBY_CHAT) {
            ++lobbyChats;
        }
    }
    CHECK_EQUAL(static_cast<size_t>(kThreads * kPosts), lobbyChats);
}

/* ---- RapidCheck properties --------------------------------------------- */

TEST(GameRoomManagerTest, RapidCheckBanAndVoicePolicyInvariantsUnderArbitraryOperations)
{
    bool passed = rc::check("banned set and voice policy invariants", [&]() {
        std::vector<LiveRoomRow> existingRows;
        std::string delErr;
        if (store->listLiveRooms(existingRows, delErr)) {
            for (const auto &row : existingRows) {
                store->deleteLiveRoom(row.roomId, delErr);
            }
        }
        GameRoomManager local(*store);
        int64_t t = kT0;
        local.setClock([&t]() -> int64_t { return t; });
        local.setWallClock([]() -> int64_t { return kWall0; });

        const int modeSel = *rc::gen::inRange(0, 3);
        GameTableConfig c;
        c.mode = modeSel == 0 ? GAME_MODE_2 : modeSel == 1 ? GAME_MODE_4 : GAME_MODE_OFK;
        c.rated = *rc::gen::arbitrary<bool>();
        c.observerPolicy = static_cast<ObserverPolicy>(*rc::gen::inRange(0, 3));
        c.voicePolicy = static_cast<VoicePolicy>(*rc::gen::inRange(0, 3));
        const int seats = c.mode == GAME_MODE_4 ? 4 : 2;
        RoomResult cr = local.createTable(p[0], c);
        RC_ASSERT(cr.ok());
        for (int i = 1; i < seats; ++i) {
            RC_ASSERT(local.joinTable(cr.roomId, p[static_cast<size_t>(i)], 0, "").ok());
        }
        const std::string room = cr.roomId;

        /* independent model of the moderation state */
        std::set<int64_t> banned;
        ObserverPolicy op = c.observerPolicy;
        VoicePolicy vp = c.voicePolicy;
        std::map<int64_t, std::pair<bool, bool>> muted;     /* chat, voice */
        std::set<int64_t> obs;                               /* current observers */
        int64_t lastSeq = 0;
        const char *chatTexts[] = {"hi", "gg", "nice move"};
        const char *moves[] = {"33+a", "37+e", "22+a", "26+e", "33a-34", "zz"};
        auto seated = [&](int i) { return i < seats; };
        auto idOf = [&](int i) { return p[static_cast<size_t>(i)]; };
        auto isMuted = [&](int64_t id, bool voice) {
            std::map<int64_t, std::pair<bool, bool>>::const_iterator it = muted.find(id);
            return it != muted.end() && (voice ? it->second.second : it->second.first);
        };
        /* may this player take part in voice under the model's policies? */
        auto voiceOk = [&](int i, const std::set<int64_t> &observers) {
            if (banned.count(idOf(i)) != 0 || vp == VoicePolicy::DISABLED) {
                return false;
            }
            if (seated(i)) {
                return true;
            }
            return vp == VoicePolicy::ALL_ALLOWED && op == ObserverPolicy::OPEN &&
                   observers.count(idOf(i)) != 0;
        };

        const int steps = *rc::gen::inRange(1, 36);
        for (int s = 0; s < steps; ++s) {
            const int op_ = *rc::gen::inRange(0, 13);
            const int a = *rc::gen::inRange(0, 8);
            const int b = *rc::gen::inRange(0, 8);
            const std::set<int64_t> obsBefore = obs;
            RoomView before;
            RC_ASSERT(local.getRoomView(room, p[0], before).ok());
            const bool finishedBefore = before.phase == RoomPhase::FINISHED;

            RoomResult r;
            switch (op_) {
            case 0: {                                             /* observe */
                r = local.observeRoom(room, idOf(a));
                RoomCode want;
                if (banned.count(idOf(a)) != 0) {
                    want = RoomCode::FORBIDDEN_BANNED;
                } else if (seated(a)) {
                    want = RoomCode::INVALID_STATE;
                } else if (op == ObserverPolicy::NO_OBSERVERS && !finishedBefore) {
                    want = RoomCode::FORBIDDEN_POLICY;
                } else {
                    want = RoomCode::OK;
                }
                RC_LOG() << "observe " << a << ": " << toString(r.code) << " want "
                         << toString(want) << "\n";
                RC_ASSERT(r.code == want);
                break;
            }
            case 1: {                                             /* stop observing */
                r = local.stopObserving(room, idOf(a));
                RC_ASSERT(r.code == (obsBefore.count(idOf(a)) != 0 ? RoomCode::OK
                                                                   : RoomCode::INVALID_STATE));
                break;
            }
            case 2: {                                             /* board chat */
                ChatScope sc = *rc::gen::arbitrary<bool>() ? ChatScope::TEAM : ChatScope::ALL;
                r = local.postBoardChat(room, idOf(a), sc, chatTexts[b % 3]);
                if (banned.count(idOf(a)) != 0) {
                    RC_ASSERT(r.code == RoomCode::FORBIDDEN_BANNED);
                }
                if (r.ok()) {
                    RC_ASSERT(banned.count(idOf(a)) == 0);
                    RC_ASSERT(seated(a) || obsBefore.count(idOf(a)) != 0);
                    RC_ASSERT(seated(a) || op == ObserverPolicy::OPEN);
                    RC_ASSERT(!isMuted(idOf(a), false));
                    RC_ASSERT(sc == ChatScope::ALL || (c.mode == GAME_MODE_4 && seated(a)));
                } else if (seated(a) && sc == ChatScope::ALL) {
                    RC_ASSERT(r.code == RoomCode::RATE_LIMITED);
                }
                break;
            }
            case 3: {                                             /* voice signal */
                const std::string payload = "{\"n\":" + std::to_string(s) + "}";
                r = local.submitVoiceSignal(room, idOf(a), idOf(b),
                                            static_cast<VoiceSignalType>(s % 3), payload);
                if (banned.count(idOf(a)) != 0) {
                    RC_ASSERT(r.code == RoomCode::FORBIDDEN_BANNED);
                }
                if (r.ok()) {
                    RC_ASSERT(a != b);
                    RC_ASSERT(voiceOk(a, obsBefore));
                    RC_ASSERT(voiceOk(b, obsBefore));
                    RC_ASSERT(seated(a) || obsBefore.count(idOf(a)) != 0);
                    RC_ASSERT(seated(b) || obsBefore.count(idOf(b)) != 0);
                    RC_ASSERT(!isMuted(idOf(a), true));
                } else if (seated(a) && seated(b) && a != b && vp != VoicePolicy::DISABLED) {
                    RC_LOG() << "signal " << a << "->" << b << " refused: "
                             << toString(r.code) << " " << r.message << "\n";
                    RC_ASSERT(false);
                }
                if (vp == VoicePolicy::DISABLED) {
                    RC_ASSERT(!r.ok());
                }
                break;
            }
            case 4: {                                             /* voice meta */
                VoiceMetaAction act = static_cast<VoiceMetaAction>(s % 4);
                r = local.submitVoiceMeta(room, idOf(a), act);
                if (banned.count(idOf(a)) != 0) {
                    RC_ASSERT(r.code == RoomCode::FORBIDDEN_BANNED);
                }
                if (r.ok()) {
                    RC_ASSERT(voiceOk(a, obsBefore));
                    RC_ASSERT(seated(a) || obsBefore.count(idOf(a)) != 0);
                    RC_ASSERT(!isMuted(idOf(a), true));
                }
                if (vp == VoicePolicy::DISABLED) {
                    RC_ASSERT(!r.ok());
                }
                if (seated(a) && vp != VoicePolicy::DISABLED &&
                    act == VoiceMetaAction::JOINED_VOICE) {
                    RC_ASSERT(r.ok());
                }
                break;
            }
            case 5: {                                             /* ban */
                r = local.banUser(room, idOf(a), idOf(b), "x");
                RoomCode want;
                if (banned.count(idOf(a)) != 0) {
                    want = RoomCode::FORBIDDEN_BANNED;
                } else if (!seated(a)) {
                    want = RoomCode::FORBIDDEN_NOT_SEATED;
                } else if (a == b) {
                    want = RoomCode::INVALID_ARGUMENT;
                } else if (seated(b)) {
                    want = RoomCode::FORBIDDEN_SEATED_TARGET;
                } else {
                    want = RoomCode::OK;
                }
                RC_LOG() << "ban " << a << "->" << b << ": " << toString(r.code)
                         << " want " << toString(want) << "\n";
                RC_ASSERT(r.code == want);
                if (r.ok()) {
                    banned.insert(idOf(b));
                    obs.erase(idOf(b));
                }
                break;
            }
            case 6: {                                             /* mute */
                const bool mc = *rc::gen::arbitrary<bool>();
                const bool mv = *rc::gen::arbitrary<bool>();
                r = local.muteUser(room, idOf(a), idOf(b), mc, mv);
                RoomCode want;
                if (banned.count(idOf(a)) != 0) {
                    want = RoomCode::FORBIDDEN_BANNED;
                } else if (!seated(a)) {
                    want = RoomCode::FORBIDDEN_NOT_SEATED;
                } else if (a == b) {
                    want = RoomCode::INVALID_ARGUMENT;
                } else if ((mc || mv) && seated(b)) {
                    want = RoomCode::FORBIDDEN_SEATED_TARGET;
                } else {
                    want = RoomCode::OK;
                }
                RC_ASSERT(r.code == want);
                if (r.ok()) {
                    if (mc || mv) {
                        muted[idOf(b)] = std::make_pair(mc, mv);
                    } else {
                        muted.erase(idOf(b));
                    }
                }
                break;
            }
            case 7: {                                             /* policies */
                ObserverPolicy no = static_cast<ObserverPolicy>(*rc::gen::inRange(0, 3));
                VoicePolicy nv = static_cast<VoicePolicy>(*rc::gen::inRange(0, 3));
                r = local.updateRoomPolicies(room, idOf(a), no, nv);
                RoomCode want;
                if (banned.count(idOf(a)) != 0) {
                    want = RoomCode::FORBIDDEN_BANNED;
                } else if (!seated(a)) {
                    want = RoomCode::FORBIDDEN_NOT_SEATED;
                } else {
                    want = RoomCode::OK;
                }
                RC_ASSERT(r.code == want);
                if (r.ok()) {
                    op = no;
                    vp = nv;
                }
                break;
            }
            case 8: {                                             /* join a seat */
                r = local.joinTable(room, idOf(a), 0, "");
                RC_ASSERT(!r.ok());                               /* game already runs */
                if (banned.count(idOf(a)) != 0) {
                    RC_ASSERT(r.code == RoomCode::FORBIDDEN_BANNED);
                }
                break;
            }
            case 9: {                                             /* move */
                r = local.submitMove(room, idOf(a), moves[b % 6]);
                if (r.ok()) {
                    RC_ASSERT(seated(a));
                }
                break;
            }
            case 10: {                                            /* time passes */
                t += *rc::gen::inRange(0, 70000);
                local.tick();
                break;
            }
            case 11: {                                            /* rare resignation */
                if (*rc::gen::inRange(0, 6) == 0) {
                    r = local.resign(room, idOf(a));
                    RC_ASSERT(seated(a) || !r.ok());
                }
                break;
            }
            default: {                                            /* propose / respond */
                ProposalType pt = static_cast<ProposalType>(s % 4);
                if (*rc::gen::arbitrary<bool>()) {
                    local.submitProposal(room, idOf(a), pt);
                } else {
                    local.respondProposal(room, idOf(a), pt, *rc::gen::arbitrary<bool>());
                }
                break;
            }
            }

            /* ---- invariants that must hold after every operation ---- */
            RoomView v;
            RC_ASSERT(local.getRoomView(room, p[0], v).ok());
            obs = std::set<int64_t>(v.observerIds.begin(), v.observerIds.end());
            RC_ASSERT(v.latestSeq >= lastSeq);
            lastSeq = v.latestSeq;
            RC_ASSERT(v.config.observerPolicy == op);
            RC_ASSERT(v.config.voicePolicy == vp);

            std::vector<int64_t> bl = local.bannedPlayers(room);
            RC_ASSERT(std::set<int64_t>(bl.begin(), bl.end()) == banned);
            RC_ASSERT(bl.size() == banned.size());

            for (std::set<int64_t>::const_iterator o = obs.begin(); o != obs.end(); ++o) {
                RC_ASSERT(banned.count(*o) == 0);
            }
            if (op == ObserverPolicy::NO_OBSERVERS && v.phase != RoomPhase::FINISHED) {
                RC_ASSERT(obs.empty());
            }
            for (size_t i = 0; i < v.voicePeers.size(); ++i) {
                int64_t id = v.voicePeers[i].playerId;
                RC_ASSERT(banned.count(id) == 0);
                RC_ASSERT(!isMuted(id, true));
                int idx = -1;
                for (int k = 0; k < 8; ++k) {
                    if (idOf(k) == id) {
                        idx = k;
                    }
                }
                RC_ASSERT(idx >= 0);
                RC_ASSERT(voiceOk(idx, obs));
            }
            if (vp == VoicePolicy::DISABLED) {
                RC_ASSERT(v.voicePeers.empty());
            }

            /* a banned player is locked out of every read and write */
            for (std::set<int64_t>::const_iterator bi = banned.begin(); bi != banned.end(); ++bi) {
                const int64_t id = *bi;
                RC_ASSERT(local.isBanned(room, id));
                RoomView bv;
                RoomPoll bp;
                std::string tr;
                RC_ASSERT(local.getRoomView(room, id, bv).code == RoomCode::FORBIDDEN_BANNED);
                RC_ASSERT(local.pollRoomEvents(room, id, 0, 50, bp).code == RoomCode::FORBIDDEN_BANNED);
                RC_ASSERT(bp.events.empty());
                RC_ASSERT(local.getTranscript(room, id, tr).code == RoomCode::FORBIDDEN_BANNED);
                RC_ASSERT(local.postBoardChat(room, id, ChatScope::ALL, "hi").code ==
                          RoomCode::FORBIDDEN_BANNED);
                RC_ASSERT(local.submitVoiceSignal(room, id, p[0], VoiceSignalType::SDP_OFFER, "{}").code ==
                          RoomCode::FORBIDDEN_BANNED);
                RC_ASSERT(local.submitVoiceMeta(room, id, VoiceMetaAction::JOINED_VOICE).code ==
                          RoomCode::FORBIDDEN_BANNED);
                RC_ASSERT(local.observeRoom(room, id).code == RoomCode::FORBIDDEN_BANNED);
            }

            /* the event stream carries nothing from banned players and no
             * signal that was not addressed to the viewer */
            for (int viewer = 0; viewer < seats; ++viewer) {
                RoomPoll rp;
                RC_ASSERT(local.pollRoomEvents(room, idOf(viewer), 0, 500, rp).ok());
                int64_t prev = 0;
                for (size_t i = 0; i < rp.events.size(); ++i) {
                    const RoomEvent &e = rp.events[i];
                    RC_ASSERT(e.seq > prev);
                    prev = e.seq;
                    if (e.type == RoomEventType::CHAT || e.type == RoomEventType::VOICE_META) {
                        RC_ASSERT(banned.count(e.playerId) == 0);
                    }
                    if (e.type == RoomEventType::VOICE_SIGNAL) {
                        RC_ASSERT(e.targetPlayerId == idOf(viewer));
                        RC_ASSERT(banned.count(e.playerId) == 0);
                        RC_ASSERT(e.text.size() <= GameRoomManager::kMaxVoicePayloadBytes);
                    }
                    if (e.type == RoomEventType::CHAT && e.scope == ChatScope::TEAM) {
                        RC_ASSERT(c.mode == GAME_MODE_4);
                    }
                }
            }
            RoomPoll anon;
            if (local.pollRoomEvents(room, 0, 0, 500, anon).ok()) {
                for (size_t i = 0; i < anon.events.size(); ++i) {
                    RC_ASSERT(anon.events[i].type != RoomEventType::VOICE_SIGNAL);
                    RC_ASSERT(!(anon.events[i].type == RoomEventType::CHAT &&
                                anon.events[i].scope == ChatScope::TEAM));
                }
            }
        }
    });
    CHECK_TRUE(passed);
}

TEST(GameRoomManagerTest, RapidCheckRandomLegalGamesStayInSyncWithTheEngine)
{
    bool passed = rc::check("room board, clocks and transcript track the engine", [&]() {
        std::vector<LiveRoomRow> existingRows;
        std::string delErr;
        if (store->listLiveRooms(existingRows, delErr)) {
            for (const auto &row : existingRows) {
                store->deleteLiveRoom(row.roomId, delErr);
            }
        }
        GameRoomManager local(*store);
        int64_t t = kT0;
        local.setClock([&t]() -> int64_t { return t; });
        local.setWallClock([]() -> int64_t { return kWall0; });

        const int modeSel = *rc::gen::inRange(0, 3);
        GameTableConfig c;
        c.mode = modeSel == 0 ? GAME_MODE_2 : modeSel == 1 ? GAME_MODE_4 : GAME_MODE_OFK;
        c.rated = false;
        const bool timed = *rc::gen::arbitrary<bool>();
        const int64_t inc = 1000;
        const int64_t init = 600000;
        if (timed) {
            c.timeControl.enabled = true;
            c.timeControl.initialMs = init;
            c.timeControl.incrementMs = inc;
        }
        const int seats = c.mode == GAME_MODE_4 ? 4 : 2;
        RoomResult cr = local.createTable(p[0], c);
        RC_ASSERT(cr.ok());
        for (int i = 1; i < seats; ++i) {
            RC_ASSERT(local.joinTable(cr.roomId, p[static_cast<size_t>(i)], 0, "").ok());
        }
        const std::string room = cr.roomId;

        OctiBoard mirror(c.mode, false, false, c.bases);
        OctiGame *mg = mirror.getGame();
        mg->start(PLAYER_A);
        int64_t clock[5] = {0, init, init, init, init};

        const int plies = *rc::gen::inRange(1, 40);
        bool over = false;
        for (int i = 0; i < plies && !over; ++i) {
            std::vector<OctiMove> legal = mg->generateLegalMoves();
            if (legal.empty()) {
                break;
            }
            const size_t pick = static_cast<size_t>(
                *rc::gen::inRange(0, static_cast<int>(legal.size())));
            const int seat = static_cast<int>(mg->getTurn());
            const int64_t spent = *rc::gen::inRange(0, 3000);
            t += spent;
            RoomResult r = local.submitMove(room, p[static_cast<size_t>(seat - 1)],
                                            legal[pick].asString());
            RC_LOG() << "ply " << i << " seat " << seat << " " << legal[pick].asString()
                     << ": " << toString(r.code) << " " << r.message << "\n";
            RC_ASSERT(r.ok());
            mg->move(legal[pick]);
            RC_ASSERT(r.move == legal[pick].asString());
            RC_ASSERT(r.elapsedMs == spent);
            if (timed) {
                clock[seat] = clock[seat] - spent + inc;
                RC_ASSERT(r.clockMs == clock[seat]);
            }
            RoomView v;
            RC_ASSERT(local.getRoomView(room, 0, v).ok());
            RC_ASSERT(v.fen == mirror.toPositionString());
            RC_ASSERT(v.plyCount == mg->getNumMoves());
            over = mg->isGameOver();
            RC_ASSERT(r.gameOver == over);
            RC_ASSERT((v.phase == RoomPhase::FINISHED) == over);
            if (over) {
                RC_ASSERT(v.winnerSeat == static_cast<int>(mg->getWinner()));
                RC_ASSERT(v.archived);
            } else {
                RC_ASSERT(v.turnSeat == static_cast<int>(mg->getTurn()));
                if (timed) {
                    for (int s = 1; s <= seats; ++s) {
                        int64_t expect = clock[s];
                        if (s == v.turnSeat) {
                            expect = clock[s];            /* no time passed since the move */
                        }
                        RC_ASSERT(v.seats[static_cast<size_t>(s - 1)].clockMs == expect);
                    }
                }
            }
        }

        /* whatever happened, the transcript replays to the same position */
        std::string transcript;
        RC_ASSERT(local.getTranscript(room, 0, transcript).ok());
        OctiBoard replay;
        replay.deserializeGame(transcript);
        RC_ASSERT(replay.toPositionString() == mirror.toPositionString());
        RC_ASSERT(replay.getGame()->getNumMoves() == mirror.getGame()->getNumMoves());
    });
    CHECK_TRUE(passed);
}

/*
 * Regression: closing a table because its last seat left used to pass the
 * room's own id string to deleteRoomLocked(), which then kept reading that
 * string after the room (and the string) had been destroyed. Only
 * AddressSanitizer notices; run through `make test SANITIZE=1`.
 */
TEST(GameRoomManagerTest, PartialPolicyUpdateKeepsTheOtherField)
{
    GameTableConfig c = cfg();
    c.observerPolicy = ObserverPolicy::SILENT_OBSERVERS;
    c.voicePolicy = VoicePolicy::ALL_ALLOWED;
    std::string room = startTable(c, 2);
    std::optional<ObserverPolicy> noObserver;
    std::optional<VoicePolicy> noVoice;

    /* Only voice changes: the observer policy must survive untouched. */
    EXPECT_OK(mgr->updateRoomPolicies(room, p[0], noObserver,
                                      std::optional<VoicePolicy>(VoicePolicy::DISABLED)));
    CHECK_TRUE(view(room).config.observerPolicy == ObserverPolicy::SILENT_OBSERVERS);
    CHECK_TRUE(view(room).config.voicePolicy == VoicePolicy::DISABLED);

    /* Only the observer policy changes: voice must survive untouched. */
    EXPECT_OK(mgr->updateRoomPolicies(
        room, p[1], std::optional<ObserverPolicy>(ObserverPolicy::OPEN), noVoice));
    CHECK_TRUE(view(room).config.observerPolicy == ObserverPolicy::OPEN);
    CHECK_TRUE(view(room).config.voicePolicy == VoicePolicy::DISABLED);

    /* An update that changes nothing emits no event. */
    int64_t before = view(room).latestSeq;
    EXPECT_OK(mgr->updateRoomPolicies(
        room, p[0], std::optional<ObserverPolicy>(ObserverPolicy::OPEN), noVoice));
    CHECK_EQUAL(before, view(room).latestSeq);

    /* Nothing to change, a non-seated actor and an unknown room all fail. */
    EXPECT_CODE(INVALID_ARGUMENT, mgr->updateRoomPolicies(room, p[0], noObserver, noVoice));
    EXPECT_CODE(FORBIDDEN_NOT_SEATED,
                mgr->updateRoomPolicies(
                    room, p[2], std::optional<ObserverPolicy>(ObserverPolicy::OPEN), noVoice));
    EXPECT_CODE(NOT_FOUND,
                mgr->updateRoomPolicies(
                    "no-such-room", p[0], std::optional<ObserverPolicy>(ObserverPolicy::OPEN),
                    noVoice));
    CHECK_TRUE(view(room).config.observerPolicy == ObserverPolicy::OPEN);
}

TEST(GameRoomManagerTest, OnlinePlayerCountTracksLobbyPresence)
{
    CHECK_EQUAL(mgr->lobbyState().players.size(), mgr->onlinePlayerCount());
    size_t base = mgr->onlinePlayerCount();
    EXPECT_OK(mgr->enterLobby(p[0]));
    EXPECT_OK(mgr->enterLobby(p[1]));
    CHECK_EQUAL(mgr->lobbyState().players.size(), mgr->onlinePlayerCount());
    EXPECT_OK(mgr->leaveLobby(p[0]));
    CHECK_EQUAL(mgr->lobbyState().players.size(), mgr->onlinePlayerCount());
    EXPECT_OK(mgr->leaveLobby(p[1]));
    CHECK_EQUAL(base, mgr->onlinePlayerCount());
}

TEST(GameRoomManagerTest, SoleHostLeavingClosesTheRoomWithoutTouchingTheFreedId)
{
    RoomResult r = mgr->createTable(p[0], cfg());
    EXPECT_OK(r);
    CHECK_EQUAL(1, static_cast<int>(mgr->roomCount()));
    /* An observer makes the close walk the per-player maps with the id. */
    EXPECT_OK(mgr->observeRoom(r.roomId, p[3]));
    EXPECT_OK(mgr->leaveTable(r.roomId, p[0]));
    CHECK_EQUAL(0, static_cast<int>(mgr->roomCount()));
    CHECK_EQUAL(0, static_cast<int>(mgr->listTables().size()));

    /* Same close path through leaving the lobby while seated. */
    RoomResult r2 = mgr->createTable(p[1], cfg());
    EXPECT_OK(r2);
    EXPECT_OK(mgr->observeRoom(r2.roomId, p[3]));
    EXPECT_OK(mgr->leaveLobby(p[1]));
    CHECK_EQUAL(0, static_cast<int>(mgr->roomCount()));
    EXPECT_CODE(NOT_FOUND, mgr->joinTable(r2.roomId, p[2], 0, ""));
}

TEST(GameRoomManagerTest, HotRestoreAcrossManagerRecreationWaitingAndInProgressAndFinished)
{
    /* 1. WAITING room */
    RoomResult rw = mgr->createTable(p[7], cfg(GAME_MODE_2));
    EXPECT_OK(rw);
    std::string waitingId = rw.roomId;

    /* 2. IN_PROGRESS 2P timed room with moves */
    RoomResult r2p = mgr->createTable(p[0], timedCfg(60000, 1000, GAME_MODE_2));
    EXPECT_OK(r2p);
    EXPECT_OK(mgr->joinTable(r2p.roomId, p[1], 0, ""));
    std::string inProgId = r2p.roomId;
    EXPECT_OK(mgr->submitMove(inProgId, p[0], "73+a"));
    EXPECT_OK(mgr->submitMove(inProgId, p[1], "77+e"));
    EXPECT_OK(mgr->observeRoom(inProgId, p[6]));
    EXPECT_OK(mgr->muteUser(inProgId, p[0], p[6], true, false));

    /* 3. IN_PROGRESS 4P room with moves */
    RoomResult r4p = mgr->createTable(p[2], cfg(GAME_MODE_4));
    EXPECT_OK(r4p);
    EXPECT_OK(mgr->joinTable(r4p.roomId, p[3], 0, ""));
    EXPECT_OK(mgr->joinTable(r4p.roomId, p[4], 0, ""));
    EXPECT_OK(mgr->joinTable(r4p.roomId, p[5], 0, ""));
    std::string fourPId = r4p.roomId;
    EXPECT_OK(mgr->submitMove(fourPId, p[2], "33+a"));
    EXPECT_OK(mgr->submitMove(fourPId, p[3], "73"));

    /* 4. FINISHED OFK room played to victory */
    LoginResult lr1 = store->loginWithSso(makeClaims("sub-ofk1", "ofk1"), "test");
    LoginResult lr2 = store->loginWithSso(makeClaims("sub-ofk2", "ofk2"), "test");
    CHECK_TRUE(lr1.ok);
    CHECK_TRUE(lr2.ok);
    int64_t pOfk1 = lr1.profile.playerId;
    int64_t pOfk2 = lr2.profile.playerId;
    RoomResult rofk = mgr->createTable(pOfk1, cfg(GAME_MODE_OFK));
    EXPECT_OK(rofk);
    EXPECT_OK(mgr->joinTable(rofk.roomId, pOfk2, 0, ""));
    std::string ofkId = rofk.roomId;
    for (size_t i = 0; i < kOfkWin.size(); ++i) {
        int seat = (i % 2 == 0) ? 1 : 2;
        int64_t player = (seat == 1) ? pOfk1 : pOfk2;
        EXPECT_OK(mgr->submitMove(ofkId, player, kOfkWin[i]));
    }
    RoomView vOfkPre = view(ofkId);
    CHECK_EQUAL(static_cast<int>(RoomPhase::FINISHED), static_cast<int>(vOfkPre.phase));

    RoomView vWaitPre = view(waitingId);
    RoomView vInProgPre = view(inProgId);
    RoomView v4PPre = view(fourPId);

    /* Checkpoint to store and simulate restart */
    CHECK_TRUE(mgr->checkpointToStore());
    delete mgr;
    mgr = nullptr;

    /* Reconstruct on the same AccountStore */
    mgr = new GameRoomManager(*store);
    mgr->setClock([this]() -> int64_t { return now; });
    mgr->setWallClock([this]() -> int64_t { return wall; });

    CHECK_EQUAL(4, static_cast<int>(mgr->roomCount()));

    /* Verify WAITING room */
    RoomView vWaitPost = view(waitingId);
    STRCMP_EQUAL(waitingId.c_str(), vWaitPost.roomId.c_str());
    CHECK_EQUAL(static_cast<int>(RoomPhase::WAITING_FOR_PLAYERS), static_cast<int>(vWaitPost.phase));
    LONGS_EQUAL(p[7], vWaitPost.hostPlayerId);

    /* Verify 2P IN_PROGRESS room */
    RoomView vInProgPost = view(inProgId);
    STRCMP_EQUAL(inProgId.c_str(), vInProgPost.roomId.c_str());
    CHECK_EQUAL(static_cast<int>(RoomPhase::IN_PROGRESS), static_cast<int>(vInProgPost.phase));
    STRCMP_EQUAL(vInProgPre.fen.c_str(), vInProgPost.fen.c_str());
    CHECK_EQUAL(vInProgPre.moves.size(), vInProgPost.moves.size());
    LONGS_EQUAL(1, vInProgPost.turnSeat);
    CHECK_EQUAL(1u, vInProgPost.observerIds.size());
    LONGS_EQUAL(p[6], vInProgPost.observerIds[0]);

    /* Continue playing the 2P in-progress room after restore */
    EXPECT_OK(mgr->submitMove(inProgId, p[0], "73a-74"));
    RoomView vInProgPostMove = view(inProgId);
    LONGS_EQUAL(3, vInProgPostMove.plyCount);
    LONGS_EQUAL(2, vInProgPostMove.turnSeat);

    /* Verify 4P room */
    RoomView v4PPost = view(fourPId);
    CHECK_EQUAL(static_cast<int>(RoomPhase::IN_PROGRESS), static_cast<int>(v4PPost.phase));
    STRCMP_EQUAL(v4PPre.fen.c_str(), v4PPost.fen.c_str());
    CHECK_EQUAL(2u, v4PPost.moves.size());
    LONGS_EQUAL(3, v4PPost.turnSeat);

    /* Verify FINISHED room */
    RoomView vOfkPost = view(ofkId);
    CHECK_EQUAL(static_cast<int>(RoomPhase::FINISHED), static_cast<int>(vOfkPost.phase));
    STRCMP_EQUAL(vOfkPre.result.c_str(), vOfkPost.result.c_str());
    STRCMP_EQUAL(vOfkPre.termination.c_str(), vOfkPost.termination.c_str());
    CHECK_EQUAL(vOfkPre.winnerSeat, vOfkPost.winnerSeat);
}

TEST(GameRoomManagerTest, ExactClockFreezeAcrossManagerRestart)
{
    /* 60s initial clock, 0 increment */
    std::string room = startTable(timedCfg(60000, 0, GAME_MODE_2), 2);
    RoomView v0 = view(room);
    LONGS_EQUAL(60000, v0.seats[0].clockMs);
    LONGS_EQUAL(60000, v0.seats[1].clockMs);

    /* Player 1 spends 15s thinking */
    now += 15000;
    RoomView v1 = view(room);
    LONGS_EQUAL(45000, v1.seats[0].clockMs);

    /* Server shutdown for 60 seconds (1 minute upgrade window) */
    CHECK_TRUE(mgr->checkpointToStore());
    delete mgr;
    mgr = nullptr;

    now += 60000;
    wall += 60;

    /* Server relaunched */
    mgr = new GameRoomManager(*store);
    mgr->setClock([this]() -> int64_t { return now; });
    mgr->setWallClock([this]() -> int64_t { return wall; });

    /* Player 1 clock must be frozen at 45000ms, not deducted by downtime! */
    RoomView v2 = view(room);
    LONGS_EQUAL(45000, v2.seats[0].clockMs);

    /* Player 1 spends another 10s thinking after restart */
    now += 10000;
    RoomView v3 = view(room);
    LONGS_EQUAL(35000, v3.seats[0].clockMs);

    /* Player 1 submits move: total elapsed on turn was 25000ms */
    RoomResult mr = mgr->submitMove(room, p[0], "73+a");
    EXPECT_OK(mr);
    LONGS_EQUAL(25000, mr.elapsedMs);
    LONGS_EQUAL(35000, mr.clockMs);
}

TEST(GameRoomManagerTest, PreservationOfSeatDisconnectAndProposalsAndLobby)
{
    std::string room = startTable(timedCfg(60000, 0, GAME_MODE_2), 2);

    /* Disconnect seat 2 and offer draw proposal */
    EXPECT_OK(mgr->markDisconnected(room, p[1]));
    EXPECT_OK(mgr->submitProposal(room, p[0], ProposalType::DRAW_OFFER));
    now += 20000;

    /* Post lobby chat and queue matchmaking */
    EXPECT_OK(mgr->postLobbyChat(p[0], "live upgrade incoming"));
    MatchRuleset rs;
    rs.mode = GAME_MODE_2;
    EXPECT_OK(mgr->enqueueMatchmaking(p[2], rs));

    /* Shutdown and stop for 120s */
    CHECK_TRUE(mgr->checkpointToStore());
    delete mgr;
    mgr = nullptr;

    now += 120000;
    wall += 120;

    /* Restart */
    mgr = new GameRoomManager(*store);
    mgr->setClock([this]() -> int64_t { return now; });
    mgr->setWallClock([this]() -> int64_t { return wall; });

    RoomView vr = view(room);
    CHECK_FALSE(vr.seats[1].connected);
    LONGS_EQUAL(20000, vr.seats[1].disconnectedForMs);
    CHECK_EQUAL(1u, vr.proposals.size());
    CHECK_EQUAL(static_cast<int>(ProposalType::DRAW_OFFER), static_cast<int>(vr.proposals[0].type));
    /* Remaining TTL was 60000 - 20000 = 40000 */
    LONGS_EQUAL(now + 40000, vr.proposals[0].expiresAtMs);

    /* Lobby chat and sequence preserved */
    LobbySnapshot ls = mgr->lobbyState();
    CHECK_TRUE(ls.latestSeq > 0);
    bool chatFound = false;
    for (size_t i = 0; i < ls.recentChat.size(); ++i) {
        if (ls.recentChat[i].text == "live upgrade incoming") {
            chatFound = true;
            break;
        }
    }
    CHECK_TRUE(chatFound);

    /* Matchmaking queue preserved */
    MatchmakingStatus ms = mgr->matchmakingStatus(p[2]);
    CHECK_TRUE(ms.queued);
}

TEST(GameRoomManagerTest, StaleOfflineRoomsExpiredAfter24Hours)
{
    RoomResult r1 = mgr->createTable(p[0], cfg());
    EXPECT_OK(r1);
    CHECK_TRUE(mgr->checkpointToStore());

    /* A: At exact 24 hours (86400s), room is still kept */
    delete mgr;
    mgr = nullptr;
    wall += 86400;

    mgr = new GameRoomManager(*store);
    mgr->setClock([this]() -> int64_t { return now; });
    mgr->setWallClock([this]() -> int64_t { return wall; });
    CHECK_EQUAL(1, static_cast<int>(mgr->roomCount()));

    /* B: At 86401s (> 24 hours), room is expired and purged */
    delete mgr;
    mgr = nullptr;
    wall += 1;

    mgr = new GameRoomManager(*store);
    mgr->setClock([this]() -> int64_t { return now; });
    mgr->setWallClock([this]() -> int64_t { return wall; });
    CHECK_EQUAL(0, static_cast<int>(mgr->roomCount()));

    std::vector<octi::LiveRoomRow> remaining;
    std::string err;
    CHECK_TRUE(store->listLiveRooms(remaining, err));
    CHECK_EQUAL(0u, remaining.size());
}

TEST(GameRoomManagerTest, CorruptJsonInLiveRoomsAndMetaIgnoredSafely)
{
    /* Create one valid room */
    RoomResult rValid = mgr->createTable(p[0], cfg());
    EXPECT_OK(rValid);
    CHECK_TRUE(mgr->checkpointToStore());
    delete mgr;
    mgr = nullptr;

    /* Inject corrupt entries into database */
    std::string err;
    CHECK_TRUE(store->upsertLiveRoom("corrupt-raw", "{not valid json", err));
    CHECK_TRUE(store->upsertLiveRoom("corrupt-types", "{\"id\":\"corrupt-types\",\"config\":123}", err));
    CHECK_TRUE(store->setLiveMeta("lobby_state", "invalid meta json", err));

    /* Reconstruct manager: must not crash or throw */
    mgr = new GameRoomManager(*store);
    mgr->setClock([this]() -> int64_t { return now; });
    mgr->setWallClock([this]() -> int64_t { return wall; });

    CHECK_EQUAL(1, static_cast<int>(mgr->roomCount()));
    RoomView v = view(rValid.roomId);
    STRCMP_EQUAL(rValid.roomId.c_str(), v.roomId.c_str());

    /* Corrupt rows should have been cleaned up */
    std::vector<octi::LiveRoomRow> liveRows;
    CHECK_TRUE(store->listLiveRooms(liveRows, err));
    CHECK_EQUAL(1u, liveRows.size());
    STRCMP_EQUAL(rValid.roomId.c_str(), liveRows[0].roomId.c_str());
}

TEST(GameRoomManagerTest, RapidCheckMoveSequenceRoundTripsAcrossManagerRestart)
{
    bool passed = rc::check("random legal moves round-trip identically across restart", [&]() {
        /* Setup a clean temporary sub-store */
        TempDir tempDir;
        AccountStore localStore;
        std::string err;
        RC_ASSERT(localStore.openOrCreate(tempDir.path() + "/test.db", err));

        int64_t tNow = 1000000;
        int64_t tWall = 2000000;
        localStore.setClock([&tWall]() -> int64_t { return tWall; });

        std::vector<int64_t> pids;
        for (int i = 0; i < 2; ++i) {
            LoginResult lr = localStore.loginWithSso(
                makeClaims("rc-sub-" + std::to_string(i), "rc-" + std::to_string(i)), "test");
            RC_ASSERT(lr.ok);
            pids.push_back(lr.profile.playerId);
        }

        std::unique_ptr<GameRoomManager> m1(new GameRoomManager(localStore));
        m1->setClock([&tNow]() -> int64_t { return tNow; });
        m1->setWallClock([&tWall]() -> int64_t { return tWall; });

        GameTableConfig tableCfg;
        tableCfg.mode = GAME_MODE_2;
        tableCfg.timeControl.enabled = true;
        tableCfg.timeControl.initialMs = 120000;
        tableCfg.timeControl.incrementMs = 2000;

        RoomResult cr = m1->createTable(pids[0], tableCfg);
        RC_ASSERT(cr.ok());
        RoomResult jr = m1->joinTable(cr.roomId, pids[1], 0, "");
        RC_ASSERT(jr.ok());

        /* Play a random prefix of kTwoPlayerWin */
        size_t steps = *rc::gen::inRange<size_t>(0, kTwoPlayerWin.size());
        for (size_t s = 0; s < steps; ++s) {
            int seat = (s % 2 == 0) ? 1 : 2;
            int delay = *rc::gen::inRange<int>(100, 3000);
            tNow += delay;
            RoomResult mr = m1->submitMove(cr.roomId, pids[seat - 1], kTwoPlayerWin[s]);
            RC_ASSERT(mr.ok());
            if (mr.gameOver) {
                break;
            }
        }

        RoomView preView;
        RC_ASSERT(m1->getRoomView(cr.roomId, 0, preView).ok());

        /* Stop m1 and advance downtime clock */
        m1->checkpointToStore();
        m1.reset();

        int stopDuration = *rc::gen::inRange<int>(1000, 50000);
        tNow += stopDuration;
        tWall += (stopDuration / 1000);

        /* Reconstruct m2 on the same database */
        std::unique_ptr<GameRoomManager> m2(new GameRoomManager(localStore));
        m2->setClock([&tNow]() -> int64_t { return tNow; });
        m2->setWallClock([&tWall]() -> int64_t { return tWall; });

        RoomView postView;
        RC_ASSERT(m2->getRoomView(cr.roomId, 0, postView).ok());

        RC_ASSERT(preView.roomId == postView.roomId);
        RC_ASSERT(preView.phase == postView.phase);
        RC_ASSERT(preView.fen == postView.fen);
        RC_ASSERT(preView.moves.size() == postView.moves.size());
        RC_ASSERT(preView.turnSeat == postView.turnSeat);
        RC_ASSERT(preView.plyCount == postView.plyCount);
        RC_ASSERT(preView.result == postView.result);
        RC_ASSERT(preView.winnerSeat == postView.winnerSeat);
        for (size_t i = 0; i < preView.seats.size(); ++i) {
            RC_ASSERT(preView.seats[i].clockMs == postView.seats[i].clockMs);
        }
    });
    CHECK_TRUE(passed);
}

int main(int argc, char **argv)
{
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
