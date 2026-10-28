/*
 * OctiGame.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_GAME_HXX
#define OCTI_GAME_HXX

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "OctiDefs.hxx"
#include "OctiExceptions.hxx"
#include "OctiMove.hxx"
#include "OctiPod.hxx"

namespace octi {

class OctiBoard;
class OctiGame;

class OctiGameListener {
public:
    virtual ~OctiGameListener() = default;
    virtual void gameUpdated(OctiGame *game) = 0;
    virtual void UndoRedoStatusChanged(OctiGame *game, bool canUndo, bool canRedo) = 0;
};

struct OctiPlayerState {
    int freePods = 0;
    int capturedPods = 0;
    int freeProngs = 0;
    int freeSuperProngs = 0;
    long timeRemainingMs = 0;
    bool eliminated = false;
};

struct OctiGameSnapshot {
    std::vector<OctiPod> squares[9][9];
    OctiPlayerState players[5];
    OctiPlayer turn = PLAYER_UNDEF;
    OctiPlayer winner = PLAYER_UNDEF;
    bool gameStarted = false;
    bool gameOver = false;
    bool gameResigned = false;
    OctiMove lastMove;
    OctiPlayer lastPlayer = PLAYER_UNDEF;
    int searchPlyOffset = 0;
};

class OctiGame {
public:
    explicit OctiGame(OctiBoard *board,
                      OctiGameMode mode = GAME_MODE_2,
                      bool useSuperProngs = false,
                      bool useEdgelessBoard = false,
                      OctiGameBases bases = GAME_BASES_3);
    ~OctiGame();

    OctiGame(const OctiGame &) = delete;
    OctiGame &operator=(const OctiGame &) = delete;

    void addGameListener(OctiGameListener *listener);
    void removeGameListener(OctiGameListener *listener);

    OctiBoard *getBoard();
    const OctiBoard *getBoard() const;

    OctiGameMode getMode() const;
    OctiGameBases getBases() const;
    bool useSuperProngs() const;
    bool useEdgelessBoard() const;

    bool isGameStarted() const;
    bool isGameOver() const;
    bool isGameResigned() const;
    OctiPlayer getTurn() const;
    OctiPlayer getStartPlayer() const;
    OctiPlayer getWinner() const;

    bool isPlayerEliminated(OctiPlayer p) const;
    int getFreePods(OctiPlayer p) const;
    int getCapturedPods(OctiPlayer p) const;
    int getFreeProngs(OctiPlayer p) const;
    int getFreeSuperProngs(OctiPlayer p) const;

    long getTimeRemaining(OctiPlayer p) const;
    void setTimeRemaining(OctiPlayer p, long ms);

    int getNumMoves() const;
    void getMoves(std::vector<std::pair<OctiPlayer, OctiMove>> &history) const;

    bool canUndo() const;
    bool canRedo() const;

    void reset();
    void start(OctiPlayer startPlayer = PLAYER_A);
    void randomize();
    void resign(OctiPlayer player);

    void move(const std::string &moveStr);
    void move(const char *moveStr);
    void move(const OctiMove &m);
    void move(const std::vector<OctiMoveOp> &ops);
    void move(OctiPlayer player, const OctiMove &m);
    void move(OctiPlayer player, const std::string &moveStr);

    void undo();
    void redo();

    // Preparatory Move Workflow (GUI staging)
    void prepareAddPod(int x, int y, bool liberate = false);
    void prepareAddProng(int x, int y, OctiPod pod, OctiDir dir, bool superProng = false);
    void prepareReposition(int x, int y, OctiPod pod, OctiDir srcDir, OctiDir tgtDir);
    void prepareHops(int x, int y, OctiPod pod, const std::vector<OctiHop> &hops);
    void prepareCapture(int x, int y);
    void commitPreparedMove();
    void rollbackPreparedMove();
    bool hasPreparedMove() const;
    std::string getPreparedMoveString() const;

    // Legal move generation
    void generateLegalMoves(std::vector<OctiMove> &outMoves) const;
    std::vector<OctiMove> generateLegalMoves() const;

    // Transcript serialization and deserialization
    void serialize(std::ostream &out) const;
    std::string serialize() const;
    void deserialize(std::istream &in);
    void deserialize(const std::string &transcript);

    // Snapshot / state restore
    OctiGameSnapshot captureSnapshot() const;
    void restoreSnapshot(const OctiGameSnapshot &snap);
    void setSearchMode(bool searchMode);
    bool isSearchMode() const;

private:
    void notifyGameUpdated();
    void notifyUndoRedoStatusChanged();

    void executeMoveInternal(const OctiMove &m, bool isRedo = false);
    void applyMoveOps(const std::vector<OctiMoveOp> &ops);
    void check4PlayerElimination();
    void advanceTurn();
    void updateGameOverAndWinner();

    OctiBoard *_board;
    OctiGameMode _mode;
    OctiGameBases _bases;
    bool _useSuperProngs;
    bool _useEdgelessBoard;

    bool _gameStarted;
    bool _gameOver;
    bool _gameResigned;
    OctiPlayer _startPlayer;
    OctiPlayer _turn;
    OctiPlayer _winner;

    OctiPlayerState _players[5];

    std::vector<std::pair<OctiPlayer, OctiMove>> _history;
    std::vector<OctiGameSnapshot> _undoStack;
    std::vector<OctiGameSnapshot> _redoStack;

    // Preparatory state
    bool _hasPreparedMove;
    OctiGameSnapshot _prepSnapshot;
    std::vector<OctiMoveOp> _prepOps;

    std::vector<OctiGameListener *> _listeners;
    bool _searchMode = false;
    int _searchPlyOffset = 0;
};

} // namespace octi

#endif // OCTI_GAME_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
