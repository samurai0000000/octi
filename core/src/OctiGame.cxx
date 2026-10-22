/*
 * OctiGame.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiGame.hxx"
#include "OctiBoard.hxx"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <sstream>

namespace octi {

// Ghost (vacated) pods exist only on the moving stack's own starting square.
// Every other square is judged by plain emptiness so that an own pod with
// identical prongs elsewhere on the board is never mistaken for a vacated pod.
static inline bool isEmptyForJump(const OctiSquare &sq, int x, int y,
                                  int startX, int startY,
                                  const std::vector<OctiPod> &ghostPods)
{
    if (x == startX && y == startY) {
        return sq.isEmptyWithout(ghostPods);
    }
    return sq.isEmpty();
}

static inline bool findStepDelta(int cx, int cy, int tx, int ty,
                                 bool edgeless, int width, int height,
                                 int &outDx, int &outDy)
{
    if (!edgeless) {
        outDx = tx - cx;
        outDy = ty - cy;
        return true;
    }

    bool foundX = false;
    int bestDx = 0;
    for (int d = -2; d <= 2; d++) {
        if (((cx + d) % width + width) % width == tx) {
            bestDx = d;
            foundX = true;
            break;
        }
    }
    if (!foundX) {
        return false;
    }

    bool foundY = false;
    int bestDy = 0;
    for (int d = -2; d <= 2; d++) {
        if (((cy + d) % height + height) % height == ty) {
            bestDy = d;
            foundY = true;
            break;
        }
    }
    if (!foundY) {
        return false;
    }

    outDx = bestDx;
    outDy = bestDy;
    return true;
}

OctiGame::OctiGame(OctiBoard *board,
                   OctiGameMode mode,
                   bool useSuperProngs,
                   bool useEdgelessBoard,
                   OctiGameBases bases)
    : _board(board)
    , _mode(mode)
    , _bases(bases)
    , _useSuperProngs(mode == GAME_MODE_OFK ? false : useSuperProngs)
    , _useEdgelessBoard(useEdgelessBoard)
    , _gameStarted(false)
    , _gameOver(false)
    , _gameResigned(false)
    , _startPlayer(PLAYER_A)
    , _turn(PLAYER_UNDEF)
    , _winner(PLAYER_UNDEF)
    , _hasPreparedMove(false)
{
}

OctiGame::~OctiGame() = default;

void OctiGame::addGameListener(OctiGameListener *listener)
{
    if (listener) {
        auto it = std::find(_listeners.begin(), _listeners.end(), listener);
        if (it == _listeners.end()) {
            _listeners.push_back(listener);
        }
    }
}

void OctiGame::removeGameListener(OctiGameListener *listener)
{
    auto it = std::find(_listeners.begin(), _listeners.end(), listener);
    if (it != _listeners.end()) {
        _listeners.erase(it);
    }
}

void OctiGame::notifyGameUpdated()
{
    for (auto *l : _listeners) {
        l->gameUpdated(this);
    }
}

void OctiGame::notifyUndoRedoStatusChanged()
{
    for (auto *l : _listeners) {
        l->UndoRedoStatusChanged(this, canUndo(), canRedo());
    }
}

OctiBoard *OctiGame::getBoard()
{
    return _board;
}

const OctiBoard *OctiGame::getBoard() const
{
    return _board;
}

OctiGameMode OctiGame::getMode() const
{
    return _mode;
}

OctiGameBases OctiGame::getBases() const
{
    return _bases;
}

bool OctiGame::useSuperProngs() const
{
    return _useSuperProngs;
}

bool OctiGame::useEdgelessBoard() const
{
    return _useEdgelessBoard;
}

bool OctiGame::isGameStarted() const
{
    return _gameStarted;
}

bool OctiGame::isGameOver() const
{
    return _gameOver;
}

bool OctiGame::isGameResigned() const
{
    return _gameResigned;
}

OctiPlayer OctiGame::getTurn() const
{
    return _turn;
}

OctiPlayer OctiGame::getStartPlayer() const
{
    return _startPlayer;
}

OctiPlayer OctiGame::getWinner() const
{
    return _winner;
}

bool OctiGame::isPlayerEliminated(OctiPlayer p) const
{
    if (p < PLAYER_A || p > PLAYER_D) {
        return false;
    }
    return _players[p].eliminated;
}

int OctiGame::getFreePods(OctiPlayer p) const
{
    if (p < PLAYER_A || p > PLAYER_D) return 0;
    return _players[p].freePods;
}

int OctiGame::getCapturedPods(OctiPlayer p) const
{
    if (p < PLAYER_A || p > PLAYER_D) return 0;
    return _players[p].capturedPods;
}

int OctiGame::getFreeProngs(OctiPlayer p) const
{
    if (p < PLAYER_A || p > PLAYER_D) return 0;
    return _players[p].freeProngs;
}

int OctiGame::getFreeSuperProngs(OctiPlayer p) const
{
    if (p < PLAYER_A || p > PLAYER_D) return 0;
    return _players[p].freeSuperProngs;
}

long OctiGame::getTimeRemaining(OctiPlayer p) const
{
    if (p < PLAYER_A || p > PLAYER_D) return 0;
    return _players[p].timeRemainingMs;
}

void OctiGame::setTimeRemaining(OctiPlayer p, long ms)
{
    if (p >= PLAYER_A && p <= PLAYER_D) {
        _players[p].timeRemainingMs = ms;
    }
}

int OctiGame::getNumMoves() const
{
    return static_cast<int>(_history.size());
}

void OctiGame::getMoves(std::vector<std::pair<OctiPlayer, OctiMove>> &history) const
{
    history = _history;
}

bool OctiGame::canUndo() const
{
    return !_undoStack.empty();
}

bool OctiGame::canRedo() const
{
    return !_redoStack.empty();
}

void OctiGame::reset()
{
    _undoStack.clear();
    _redoStack.clear();
    _history.clear();

    _gameStarted = false;
    _gameOver = false;
    _gameResigned = false;
    _turn = PLAYER_UNDEF;
    _winner = PLAYER_UNDEF;
    _hasPreparedMove = false;
    _prepOps.clear();

    for (int p = 0; p < 5; p++) {
        _players[p] = OctiPlayerState();
    }

    if (_board) {
        _board->clear();

        if (_mode == GAME_MODE_2) {
            _board->squareAt(2, 2).addPod(OctiPod(A_POD));
            _board->squareAt(4, 2).addPod(OctiPod(A_POD));
            _board->squareAt(6, 2).addPod(OctiPod(A_POD));

            _board->squareAt(2, 6).addPod(OctiPod(B_POD));
            _board->squareAt(4, 6).addPod(OctiPod(B_POD));
            _board->squareAt(6, 6).addPod(OctiPod(B_POD));

            for (int p : {PLAYER_A, PLAYER_B}) {
                _players[p].freePods = 4;
                _players[p].capturedPods = 0;
                if (_useSuperProngs) {
                    _players[p].freeProngs = 24;
                    _players[p].freeSuperProngs = 1;
                } else {
                    _players[p].freeProngs = 25;
                    _players[p].freeSuperProngs = 0;
                }
            }
        } else if (_mode == GAME_MODE_4) {
            _board->squareAt(2, 2).addPod(OctiPod(A_POD));
            _board->squareAt(2, 6).addPod(OctiPod(B_POD));
            _board->squareAt(6, 6).addPod(OctiPod(C_POD));
            _board->squareAt(6, 2).addPod(OctiPod(D_POD));

            for (int p : {PLAYER_A, PLAYER_B, PLAYER_C, PLAYER_D}) {
                _players[p].freePods = 3;
                _players[p].capturedPods = 0;
                if (_useSuperProngs) {
                    _players[p].freeProngs = 11;
                    _players[p].freeSuperProngs = 1;
                } else {
                    _players[p].freeProngs = 12;
                    _players[p].freeSuperProngs = 0;
                }
                _players[p].eliminated = false;
            }
        } else if (_mode == GAME_MODE_OFK) {
            _board->squareAt(1, 1).addPod(OctiPod(A_POD));
            _board->squareAt(2, 1).addPod(OctiPod(A_POD));
            _board->squareAt(3, 1).addPod(OctiPod(A_POD));
            _board->squareAt(4, 1).addPod(OctiPod(A_POD));

            _board->squareAt(1, 5).addPod(OctiPod(B_POD));
            _board->squareAt(2, 5).addPod(OctiPod(B_POD));
            _board->squareAt(3, 5).addPod(OctiPod(B_POD));
            _board->squareAt(4, 5).addPod(OctiPod(B_POD));

            for (int p : {PLAYER_A, PLAYER_B}) {
                _players[p].freePods = 0;
                _players[p].capturedPods = 0;
                _players[p].freeProngs = 12;
                _players[p].freeSuperProngs = 0;
                _players[p].eliminated = false;
            }
        }
    }

    notifyGameUpdated();
    notifyUndoRedoStatusChanged();
}

void OctiGame::start(OctiPlayer startPlayer)
{
    _startPlayer = startPlayer;
    _turn = startPlayer;
    _gameStarted = true;
    _gameOver = false;
    _gameResigned = false;
    _winner = PLAYER_UNDEF;

    notifyGameUpdated();
    notifyUndoRedoStatusChanged();
}

void OctiGame::randomize()
{
    if (_board && _mode != GAME_MODE_2) {
        _board->setGameMode(GAME_MODE_2);
    }
    reset();
    start(PLAYER_A);

    const char *moves[] = {
        "33+b",
        "37+e",
        "33",
        "37",
        "33+c",
        "37+d",
        "53+a",
        "57+f",
        "33b+g",
        "37d+f",
        "73+h",
        "37df-46,37e-36",
        "53a+f",
        "57f-35",
        "33c-43,33bg-44",
        "36e+c",
        "53af-54",
        "35f+d"
    };

    for (const char *m : moves) {
        move(m);
    }
}

void OctiGame::resign(OctiPlayer player)
{
    if (!_gameStarted || _gameOver) {
        throw IllegalMoveException("game is not in progress");
    }

    _gameResigned = true;
    _gameOver = true;

    if (_mode == GAME_MODE_2 || _mode == GAME_MODE_OFK) {
        _winner = (player == PLAYER_A) ? PLAYER_B : PLAYER_A;
    } else if (_mode == GAME_MODE_4) {
        if (player == PLAYER_A || player == PLAYER_C) {
            _winner = PLAYER_B;
        } else {
            _winner = PLAYER_A;
        }
    }

    notifyGameUpdated();
    notifyUndoRedoStatusChanged();
}

void OctiGame::move(const std::string &moveStr)
{
    move(OctiMove(moveStr));
}

void OctiGame::move(const char *moveStr)
{
    move(OctiMove(moveStr));
}

void OctiGame::move(const std::vector<OctiMoveOp> &ops)
{
    move(OctiMove(ops));
}

void OctiGame::move(OctiPlayer player, const OctiMove &m)
{
    if (player != _turn) {
        throw IllegalMoveException("not player's turn");
    }
    move(m);
}

void OctiGame::move(OctiPlayer player, const std::string &moveStr)
{
    move(player, OctiMove(moveStr));
}

void OctiGame::move(const OctiMove &m)
{
    executeMoveInternal(m, false);
}

void OctiGame::executeMoveInternal(const OctiMove &m, bool isRedo)
{
    if (!_gameStarted) {
        throw IllegalMoveException("game has not started");
    }
    if (_gameOver) {
        throw IllegalMoveException("game is already over");
    }
    if (_turn == PLAYER_UNDEF) {
        throw IllegalMoveException("turn is undefined");
    }
    if (m.isEmpty()) {
        throw IllegalMoveException("move is empty");
    }

    OctiGameSnapshot snapBefore = captureSnapshot();

    try {
        applyMoveOps(m.getOps());
        check4PlayerElimination();
    } catch (...) {
        restoreSnapshot(snapBefore);
        throw;
    }

    if (!isRedo) {
        snapBefore.lastMove = m;
        snapBefore.lastPlayer = snapBefore.turn;
        _undoStack.push_back(snapBefore);
        _redoStack.clear();
        _history.push_back({snapBefore.turn, m});
    }

    updateGameOverAndWinner();
    if (!_gameOver) {
        advanceTurn();
    }

    if (_board) {
        _board->notifyBoardTurnUpdate(&m);
    }
    notifyGameUpdated();
    notifyUndoRedoStatusChanged();
}

void OctiGame::applyMoveOps(const std::vector<OctiMoveOp> &ops)
{
    if (ops.empty()) {
        throw IllegalMoveException("no operations in move");
    }

    // Separate bonus ops and regular ops
    std::vector<OctiMoveOp> bonusOps;
    std::vector<OctiMoveOp> regularOps;
    for (const auto &op : ops) {
        if (op.isSuperProngBonusMove) {
            bonusOps.push_back(op);
        } else {
            regularOps.push_back(op);
        }
    }

    if (!bonusOps.empty()) {
        if (!_useSuperProngs) {
            throw IllegalMoveException("superprongs not enabled");
        }
        if (bonusOps.size() > 2) {
            throw IllegalMoveException("exceeded maximum superprong bonus moves");
        }

        // Execute bonus ops
        std::set<std::pair<int, int>> bonusCaptures;
        for (const auto &bOp : bonusOps) {
            if (bOp.hops.empty()) {
                throw IllegalMoveException("superprong bonus move must contain hops");
            }
            if (!_board->isValidCoord(bOp.x0, bOp.y0)) {
                throw IllegalMoveException("superprong bonus starting square out of bounds");
            }
            OctiSquare &sq = _board->squareAt(bOp.x0, bOp.y0);
            int podIdx = -1;
            for (int i = 0; i < sq.getNumPods(); i++) {
                if (sq.getPodAt(i).getType() == playerToPodType(_turn) &&
                    (sq.getPodAt(i).rawConfig() & 0xFFFFu) == (bOp.pod.rawConfig() & 0xFFFFu)) {
                    podIdx = i;
                    break;
                }
            }
            if (podIdx == -1) {
                throw IllegalMoveException("pod for superprong bonus move not found");
            }
            OctiPod pod = sq.getPodAt(podIdx);
            sq.removePod(pod);

            int cx = bOp.x0;
            int cy = bOp.y0;
            std::set<std::pair<int, int>> jumpedSquares;

            for (const auto &hop : bOp.hops) {
                int dx = 0, dy = 0;
                if (!findStepDelta(cx, cy, hop.x, hop.y, _useEdgelessBoard,
                                   _board->getBoardWidth(), _board->getBoardHeight(),
                                   dx, dy)) {
                    throw IllegalMoveException("invalid displacement in bonus move");
                }
                bool isDist1 = (std::max(std::abs(dx), std::abs(dy)) == 1 &&
                                (std::abs(dx) == 1 || std::abs(dy) == 1));
                if (isDist1) {
                    if (bOp.hops.size() != 1 || hop.capture) {
                        throw IllegalMoveException("invalid distance-1 bonus hop");
                    }
                    OctiDir dir = deltaToDir(dx, dy);
                    if (dir == DIR_UNDEF || pod.prongAt(dir) != SUPER_PRONG) {
                        throw IllegalMoveException("bonus move must follow superprong direction");
                    }
                    OctiSquare &destSq = _board->squareAt(hop.x, hop.y);
                    if (destSq.hasEnemyOf(_turn)) {
                        throw IllegalMoveException("cannot move onto enemy square in bonus move");
                    }
                    cx = hop.x;
                    cy = hop.y;
                } else {
                    bool validJump = (std::abs(dx) == 0 || std::abs(dx) == 2) &&
                                     (std::abs(dy) == 0 || std::abs(dy) == 2) &&
                                     (std::abs(dx) == 2 || std::abs(dy) == 2) &&
                                     (std::abs(dx) == 0 || std::abs(dy) == 0 || std::abs(dx) == std::abs(dy));
                    if (!validJump) {
                        throw IllegalMoveException("invalid jump geometry in bonus move");
                    }
                    OctiDir dir = deltaToDir(dx / 2, dy / 2);
                    if (dir == DIR_UNDEF || pod.prongAt(dir) != SUPER_PRONG) {
                        throw IllegalMoveException("bonus jump must follow superprong direction");
                    }
                    int jx = _useEdgelessBoard ? (((cx + dx / 2) % _board->getBoardWidth() + _board->getBoardWidth()) % _board->getBoardWidth()) : (cx + dx / 2);
                    int jy = _useEdgelessBoard ? (((cy + dy / 2) % _board->getBoardHeight() + _board->getBoardHeight()) % _board->getBoardHeight()) : (cy + dy / 2);
                    std::pair<int, int> jCoord = {jx, jy};
                    if (jumpedSquares.count(jCoord) > 0) {
                        throw IllegalMoveException("cannot jump same square twice in bonus move");
                    }
                    jumpedSquares.insert(jCoord);

                    OctiSquare &jumpedSq = _board->squareAt(jx, jy);
                    if (jumpedSq.isEmpty()) {
                        throw IllegalMoveException("cannot jump empty square in bonus move");
                    }
                    if (hop.capture) {
                        if (!jumpedSq.hasEnemyOf(_turn)) {
                            throw IllegalMoveException("cannot capture non-enemy square in bonus move");
                        }
                        bonusCaptures.insert(jCoord);
                    }
                    OctiSquare &landingSq = _board->squareAt(hop.x, hop.y);
                    if (!landingSq.isEmpty()) {
                        throw IllegalMoveException("landing square must be empty in bonus move");
                    }
                    cx = hop.x;
                    cy = hop.y;
                }
            }
            _board->squareAt(cx, cy).addPod(pod);
        }

        // Process bonus captures
        for (const auto &coord : bonusCaptures) {
            OctiSquare &sq = _board->squareAt(coord.first, coord.second);
            std::vector<OctiPod> keep;
            for (int i = 0; i < sq.getNumPods(); i++) {
                const OctiPod &sqPod = sq.getPodAt(i);
                OctiPlayer owner = podTypeToPlayer(sqPod.getType());
                if (areAlliedPlayers(_mode, _turn, owner)) {
                    keep.push_back(sqPod);
                } else {
                    _players[_turn].freeProngs += sqPod.getNumNormalProngs();
                    _players[_turn].freeSuperProngs += sqPod.getNumSuperProngs();
                    _players[owner].capturedPods++;
                }
            }
            sq.clear();
            for (const auto &kp : keep) {
                sq.addPod(kp);
            }
        }
    }

    if (regularOps.empty()) {
        throw IllegalMoveException("move must include a regular turn action");
    }

    // Option V: Pod placement / liberation
    if (regularOps[0].addPod) {
        if (regularOps.size() != 1) {
            throw IllegalMoveException("pod placement cannot be combined with other actions");
        }
        if (_mode == GAME_MODE_OFK) {
            throw IllegalMoveException("pod placement forbidden in OCTI for Kids");
        }
        const auto &op = regularOps[0];
        if (!_board->isValidCoord(op.x0, op.y0)) {
            throw IllegalMoveException("pod placement square out of bounds");
        }
        OctiSquare &sq = _board->squareAt(op.x0, op.y0);
        if (sq.getType() == NEUTRAL_SQUARE) {
            throw IllegalMoveException("pod placement only allowed on OCTI squares");
        }
        if (sq.isEmpty() || sq.hasEnemyOf(_turn)) {
            throw IllegalMoveException("pod placement square must be occupied by friendly pod");
        }

        if (_mode == GAME_MODE_2) {
            bool isFriendlyBase = (sq.getType() == (_turn == PLAYER_A ? A_HOME_SQUARE : B_HOME_SQUARE));
            if (isFriendlyBase) {
                if (op.liberate) {
                    if (!_board->canLiberate(_turn) || _players[_turn].capturedPods <= 0) {
                        throw IllegalMoveException("liberation condition not met on friendly base");
                    }
                    _players[_turn].capturedPods--;
                    sq.addPod(OctiPod(playerToPodType(_turn)));
                } else {
                    if (_players[_turn].freePods > 0) {
                        _players[_turn].freePods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else if (_board->canLiberate(_turn) && _players[_turn].capturedPods > 0) {
                        _players[_turn].capturedPods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else {
                        throw IllegalMoveException("no free or capturable pods available to place");
                    }
                }
            } else {
                // Enemy base
                if (op.liberate) {
                    if (_players[_turn].capturedPods <= 0) {
                        throw IllegalMoveException("no captured pods to liberate");
                    }
                    _players[_turn].capturedPods--;
                    sq.addPod(OctiPod(playerToPodType(_turn)));
                } else {
                    if (_players[_turn].capturedPods > 0) {
                        _players[_turn].capturedPods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else if (_players[_turn].freePods > 0) {
                        _players[_turn].freePods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else {
                        throw IllegalMoveException("no free or capturable pods available to place");
                    }
                }
            }
        } else if (_mode == GAME_MODE_4) {
            OctiSquareType homeType = (_turn == PLAYER_A ? A_HOME_SQUARE :
                                       _turn == PLAYER_B ? B_HOME_SQUARE :
                                       _turn == PLAYER_C ? C_HOME_SQUARE : D_HOME_SQUARE);
            OctiPlayer partner = (_turn == PLAYER_A ? PLAYER_C :
                                  _turn == PLAYER_C ? PLAYER_A :
                                  _turn == PLAYER_B ? PLAYER_D : PLAYER_B);
            OctiSquareType partnerHome = (partner == PLAYER_A ? A_HOME_SQUARE :
                                          partner == PLAYER_B ? B_HOME_SQUARE :
                                          partner == PLAYER_C ? C_HOME_SQUARE : D_HOME_SQUARE);

            if (sq.getType() == homeType) {
                if (op.liberate || _players[_turn].freePods <= 0) {
                    throw IllegalMoveException("cannot place reserve pod on own base");
                }
                _players[_turn].freePods--;
                sq.addPod(OctiPod(playerToPodType(_turn)));
            } else if (sq.getType() == partnerHome) {
                if (_players[partner].eliminated) {
                    if (_players[partner].capturedPods > 0) {
                        _players[partner].eliminated = false;
                        _players[partner].capturedPods--;
                        sq.addPod(OctiPod(playerToPodType(partner)));
                    } else if (!op.liberate && _players[_turn].freePods > 0) {
                        _players[_turn].freePods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else {
                        throw IllegalMoveException("cannot liberate partner: no captured pods");
                    }
                } else {
                    if (!op.liberate && _players[_turn].freePods > 0) {
                        _players[_turn].freePods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else {
                        throw IllegalMoveException("cannot place pod on partner base");
                    }
                }
            } else {
                // Enemy base or neutral OCTI square
                if (op.liberate) {
                    if (_players[_turn].capturedPods <= 0) {
                        throw IllegalMoveException("no captured pods to liberate");
                    }
                    _players[_turn].capturedPods--;
                    sq.addPod(OctiPod(playerToPodType(_turn)));
                } else {
                    if (_players[_turn].capturedPods > 0) {
                        _players[_turn].capturedPods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else if (_players[_turn].freePods > 0) {
                        _players[_turn].freePods--;
                        sq.addPod(OctiPod(playerToPodType(_turn)));
                    } else {
                        throw IllegalMoveException("no free or capturable pods available");
                    }
                }
            }
        }
        return;
    }

    // Option I: Add Prong
    if (regularOps[0].addProngDir != DIR_UNDEF && regularOps[0].rmProngDir == DIR_UNDEF) {
        if (regularOps.size() != 1) {
            throw IllegalMoveException("prong addition cannot be combined with other actions");
        }
        const auto &op = regularOps[0];
        if (!_board->isValidCoord(op.x0, op.y0)) {
            throw IllegalMoveException("prong addition square out of bounds");
        }
        OctiSquare &sq = _board->squareAt(op.x0, op.y0);
        int podIdx = -1;
        for (int i = 0; i < sq.getNumPods(); i++) {
            if (sq.getPodAt(i).getType() == playerToPodType(_turn) &&
                (sq.getPodAt(i).rawConfig() & 0xFFFFu) == (op.pod.rawConfig() & 0xFFFFu)) {
                podIdx = i;
                break;
            }
        }
        if (podIdx == -1) {
            throw IllegalMoveException("matching pod for prong addition not found on square");
        }
        if (sq.podAt(podIdx).prongAt(op.addProngDir) != NO_PRONG) {
            throw IllegalMoveException("slot already has a prong");
        }
        if (op.superProng) {
            if (!_useSuperProngs || _players[_turn].freeSuperProngs <= 0) {
                throw IllegalMoveException("no free superprongs available");
            }
            _players[_turn].freeSuperProngs--;
            sq.podAt(podIdx).addProng(op.addProngDir, true);
        } else {
            if (_players[_turn].freeProngs <= 0) {
                throw IllegalMoveException("no free normal prongs available");
            }
            _players[_turn].freeProngs--;
            sq.podAt(podIdx).addProng(op.addProngDir, false);
        }
        return;
    }

    // Option II: Reposition Prong
    if (regularOps[0].addProngDir != DIR_UNDEF && regularOps[0].rmProngDir != DIR_UNDEF) {
        if (regularOps.size() != 1) {
            throw IllegalMoveException("prong repositioning cannot be combined with other actions");
        }
        if (_mode == GAME_MODE_OFK) {
            throw IllegalMoveException("prong repositioning forbidden in OCTI for Kids");
        }
        const auto &op = regularOps[0];
        if (op.superProng) {
            throw IllegalMoveException("cannot add superprong during repositioning");
        }
        if (!_board->isValidCoord(op.x0, op.y0)) {
            throw IllegalMoveException("prong repositioning square out of bounds");
        }
        OctiSquare &sq = _board->squareAt(op.x0, op.y0);
        int podIdx = -1;
        for (int i = 0; i < sq.getNumPods(); i++) {
            if (sq.getPodAt(i).getType() == playerToPodType(_turn) &&
                (sq.getPodAt(i).rawConfig() & 0xFFFFu) == (op.pod.rawConfig() & 0xFFFFu)) {
                podIdx = i;
                break;
            }
        }
        if (podIdx == -1) {
            throw IllegalMoveException("matching pod for prong repositioning not found on square");
        }
        if (sq.podAt(podIdx).prongAt(op.rmProngDir) != NORMAL_PRONG) {
            throw IllegalMoveException("can only reposition normal prongs");
        }
        if (sq.podAt(podIdx).prongAt(op.addProngDir) != NO_PRONG) {
            throw IllegalMoveException("target prong slot must be empty");
        }
        sq.podAt(podIdx).removeProng(op.rmProngDir);
        sq.podAt(podIdx).addProng(op.addProngDir, false);
        return;
    }

    // Options III & IV: Movement and Jumps (single or composite stack move)
    int sx = regularOps[0].x0;
    int sy = regularOps[0].y0;
    if (!_board->isValidCoord(sx, sy)) {
        throw IllegalMoveException("starting square out of bounds");
    }

    for (const auto &op : regularOps) {
        if (op.x0 != sx || op.y0 != sy) {
            throw IllegalMoveException("composite move operations must share the same starting square");
        }
        if (op.hops.empty()) {
            throw IllegalMoveException("movement operation must contain at least one hop");
        }
    }

    OctiSquare &startSq = _board->squareAt(sx, sy);
    std::vector<OctiPod> ghostPods = startSq.pods();
    std::vector<bool> claimed(ghostPods.size(), false);
    std::vector<OctiPod> movingPods;

    for (const auto &op : regularOps) {
        int foundIdx = -1;
        for (std::size_t i = 0; i < ghostPods.size(); i++) {
            if (!claimed[i] &&
                ghostPods[i].getType() == playerToPodType(_turn) &&
                (ghostPods[i].rawConfig() & 0xFFFFu) == (op.pod.rawConfig() & 0xFFFFu)) {
                foundIdx = static_cast<int>(i);
                break;
            }
        }
        if (foundIdx == -1) {
            throw IllegalMoveException("matching pod not found in starting stack");
        }
        claimed[static_cast<std::size_t>(foundIdx)] = true;
        movingPods.push_back(ghostPods[static_cast<std::size_t>(foundIdx)]);
    }

    // Remove moving pods from start square
    startSq.clear();
    for (std::size_t i = 0; i < ghostPods.size(); i++) {
        if (!claimed[i]) {
            startSq.addPod(ghostPods[i]);
        }
    }

    struct LandedPod {
        OctiPod pod;
        int finalX = -1;
        int finalY = -1;
    };

    std::vector<LandedPod> landedPods;
    std::vector<std::pair<int, int>> capturedSquaresEndOfTurn;

    for (std::size_t k = 0; k < regularOps.size(); k++) {
        const auto &op = regularOps[k];
        const auto &pod = movingPods[k];
        int cx = sx;
        int cy = sy;
        std::vector<std::pair<int, int>> jumpedSquaresThisPod;

        int dx = 0, dy = 0;
        if (!findStepDelta(cx, cy, op.hops[0].x, op.hops[0].y, _useEdgelessBoard,
                           _board->getBoardWidth(), _board->getBoardHeight(),
                           dx, dy)) {
            throw IllegalMoveException("invalid displacement for first hop");
        }

        bool isDist1 = (std::max(std::abs(dx), std::abs(dy)) == 1 &&
                        (std::abs(dx) == 1 || std::abs(dy) == 1));

        if (isDist1) {
            if (op.hops.size() != 1) {
                throw IllegalMoveException("chaining distance-1 moves is forbidden");
            }
            if (op.hops[0].capture) {
                throw IllegalMoveException("capture not allowed on distance-1 move");
            }
            OctiDir dir = deltaToDir(dx, dy);
            if (dir == DIR_UNDEF || pod.prongAt(dir) == NO_PRONG) {
                throw IllegalMoveException("pod lacks prong in distance-1 move direction");
            }
            OctiSquare &destSq = _board->squareAt(op.hops[0].x, op.hops[0].y);
            if (_mode == GAME_MODE_OFK && !destSq.isEmpty()) {
                throw IllegalMoveException("pod stacking forbidden in OCTI for Kids");
            }
            if (_mode != GAME_MODE_OFK && destSq.hasEnemyOf(_turn)) {
                throw IllegalMoveException("cannot move onto enemy square");
            }
            landedPods.push_back({pod, op.hops[0].x, op.hops[0].y});
        } else {
            // Distance-2 jumps
            for (const auto &hop : op.hops) {
                int jdx = 0, jdy = 0;
                if (!findStepDelta(cx, cy, hop.x, hop.y, _useEdgelessBoard,
                                   _board->getBoardWidth(), _board->getBoardHeight(),
                                   jdx, jdy)) {
                    throw IllegalMoveException("invalid displacement for jump hop");
                }
                bool validJump = (std::abs(jdx) == 0 || std::abs(jdx) == 2) &&
                                 (std::abs(jdy) == 0 || std::abs(jdy) == 2) &&
                                 (std::abs(jdx) == 2 || std::abs(jdy) == 2) &&
                                 (std::abs(jdx) == 0 || std::abs(jdy) == 0 || std::abs(jdx) == std::abs(jdy));
                if (!validJump) {
                    throw IllegalMoveException("invalid jump geometry");
                }
                OctiDir dir = deltaToDir(jdx / 2, jdy / 2);
                if (dir == DIR_UNDEF || pod.prongAt(dir) == NO_PRONG) {
                    throw IllegalMoveException("pod lacks prong in jump direction");
                }
                int jx = _useEdgelessBoard ? (((cx + jdx / 2) % _board->getBoardWidth() + _board->getBoardWidth()) % _board->getBoardWidth()) : (cx + jdx / 2);
                int jy = _useEdgelessBoard ? (((cy + jdy / 2) % _board->getBoardHeight() + _board->getBoardHeight()) % _board->getBoardHeight()) : (cy + jdy / 2);
                std::pair<int, int> jCoord = {jx, jy};

                if (std::find(jumpedSquaresThisPod.begin(), jumpedSquaresThisPod.end(), jCoord) != jumpedSquaresThisPod.end()) {
                    throw IllegalMoveException("cannot jump same square twice in single pod chain");
                }
                jumpedSquaresThisPod.push_back(jCoord);

                OctiSquare &jumpedSq = _board->squareAt(jx, jy);
                if (isEmptyForJump(jumpedSq, jx, jy, sx, sy, ghostPods)) {
                    throw IllegalMoveException("cannot jump square without stationary pods");
                }

                if (hop.capture) {
                    if (!jumpedSq.hasEnemyOf(_turn)) {
                        throw IllegalMoveException("cannot capture non-enemy square");
                    }
                    if (std::find(capturedSquaresEndOfTurn.begin(), capturedSquaresEndOfTurn.end(), jCoord) == capturedSquaresEndOfTurn.end()) {
                        capturedSquaresEndOfTurn.push_back(jCoord);
                    }
                }

                OctiSquare &landingSq = _board->squareAt(hop.x, hop.y);
                if (!isEmptyForJump(landingSq, hop.x, hop.y, sx, sy, ghostPods)) {
                    throw IllegalMoveException("landing square must be empty of stationary pods");
                }

                cx = hop.x;
                cy = hop.y;
            }
            landedPods.push_back({pod, cx, cy});
        }
    }

    // Place all landed pods on destination squares
    for (const auto &lp : landedPods) {
        OctiSquare &destSq = _board->squareAt(lp.finalX, lp.finalY);
        if (_mode == GAME_MODE_OFK && !destSq.isEmpty()) {
            throw IllegalMoveException("pod stacking forbidden in OCTI for Kids");
        }
        destSq.addPod(lp.pod);
    }

    // Process end-of-turn captured pods
    for (const auto &coord : capturedSquaresEndOfTurn) {
        OctiSquare &sq = _board->squareAt(coord.first, coord.second);
        std::vector<OctiPod> remaining;
        for (int pIdx = 0; pIdx < sq.getNumPods(); pIdx++) {
            const OctiPod &sqPod = sq.getPodAt(pIdx);
            OctiPlayer owner = podTypeToPlayer(sqPod.getType());
            if (areAlliedPlayers(_mode, _turn, owner)) {
                remaining.push_back(sqPod);
            } else {
                _players[_turn].freeProngs += sqPod.getNumNormalProngs();
                _players[_turn].freeSuperProngs += sqPod.getNumSuperProngs();
                _players[owner].capturedPods++;
            }
        }
        sq.clear();
        for (const auto &rp : remaining) {
            sq.addPod(rp);
        }
    }
}

void OctiGame::check4PlayerElimination()
{
    if (_mode != GAME_MODE_4) {
        return;
    }

    const std::pair<int, int> homeCoords[5] = {
        {-1, -1}, {2, 2}, {2, 6}, {6, 6}, {6, 2}
    };
    for (int p = 1; p <= 4; p++) {
        OctiPlayer player = static_cast<OctiPlayer>(p);
        if (!_players[p].eliminated) {
            const OctiSquare &hsq = _board->squareAt(homeCoords[p].first, homeCoords[p].second);
            if (hsq.isCaptured()) {
                _players[p].eliminated = true;
                int bw = _board->getBoardWidth();
                int bh = _board->getBoardHeight();
                for (int x = 0; x < bw; x++) {
                    for (int y = 0; y < bh; y++) {
                        OctiSquare &s = _board->squareAt(x, y);
                        std::vector<OctiPod> keep;
                        for (int pi = 0; pi < s.getNumPods(); pi++) {
                            const OctiPod &pod = s.getPodAt(pi);
                            if (podTypeToPlayer(pod.getType()) == player) {
                                _players[_turn].freeProngs += pod.getNumNormalProngs();
                                _players[_turn].freeSuperProngs += pod.getNumSuperProngs();
                                _players[p].capturedPods++;
                            } else {
                                keep.push_back(pod);
                            }
                        }
                        s.clear();
                        for (const auto &kp : keep) {
                            s.addPod(kp);
                        }
                    }
                }
            }
        }
    }
}

void OctiGame::updateGameOverAndWinner()
{
    if (_mode == GAME_MODE_2) {
        int aOccupiesB = 0;
        int bOccupiesA = 0;

        if (_board->squareAt(2, 6).isCapturedBy(PLAYER_A)) aOccupiesB++;
        if (_board->squareAt(4, 6).isCapturedBy(PLAYER_A)) aOccupiesB++;
        if (_board->squareAt(6, 6).isCapturedBy(PLAYER_A)) aOccupiesB++;

        if (_board->squareAt(2, 2).isCapturedBy(PLAYER_B)) bOccupiesA++;
        if (_board->squareAt(4, 2).isCapturedBy(PLAYER_B)) bOccupiesA++;
        if (_board->squareAt(6, 2).isCapturedBy(PLAYER_B)) bOccupiesA++;

        int neededBases = (_bases == GAME_BASES_1) ? 1 : 3;
        if (aOccupiesB >= neededBases) {
            _gameOver = true;
            _winner = PLAYER_A;
            return;
        }
        if (bOccupiesA >= neededBases) {
            _gameOver = true;
            _winner = PLAYER_B;
            return;
        }

        // Pod count check
        int podsA = 0, podsB = 0;
        int bw = _board->getBoardWidth();
        int bh = _board->getBoardHeight();
        for (int x = 0; x < bw; x++) {
            for (int y = 0; y < bh; y++) {
                const OctiSquare &sq = _board->squareAt(x, y);
                for (int i = 0; i < sq.getNumPods(); i++) {
                    if (sq.getPodAt(i).getType() == A_POD) podsA++;
                    else if (sq.getPodAt(i).getType() == B_POD) podsB++;
                }
            }
        }

        if (podsB == 0) {
            _gameOver = true;
            _winner = PLAYER_A;
            return;
        }
        if (podsA == 0) {
            _gameOver = true;
            _winner = PLAYER_B;
            return;
        }
    } else if (_mode == GAME_MODE_OFK) {
        // Base occupation (any 1 of 4)
        for (int x = 1; x <= 4; x++) {
            if (_board->squareAt(x, 5).isCapturedBy(PLAYER_A)) {
                _gameOver = true;
                _winner = PLAYER_A;
                return;
            }
            if (_board->squareAt(x, 1).isCapturedBy(PLAYER_B)) {
                _gameOver = true;
                _winner = PLAYER_B;
                return;
            }
        }

        // Total pod capture check
        int podsA = 0, podsB = 0;
        for (int x = 0; x < _board->getBoardWidth(); x++) {
            for (int y = 0; y < _board->getBoardHeight(); y++) {
                const OctiSquare &sq = _board->squareAt(x, y);
                for (int i = 0; i < sq.getNumPods(); i++) {
                    if (sq.getPodAt(i).getType() == A_POD) podsA++;
                    else if (sq.getPodAt(i).getType() == B_POD) podsB++;
                }
            }
        }

        if (podsB == 0) {
            _gameOver = true;
            _winner = PLAYER_A;
            return;
        }
        if (podsA == 0) {
            _gameOver = true;
            _winner = PLAYER_B;
            return;
        }

        // No legal moves check for next player
        OctiPlayer nextP = (_turn == PLAYER_A) ? PLAYER_B : PLAYER_A;
        OctiPlayer savedTurn = _turn;
        _turn = nextP;
        std::vector<OctiMove> moves = generateLegalMoves();
        _turn = savedTurn;
        if (moves.empty()) {
            _gameOver = true;
            _winner = _turn; // Current player wins because opponent has no moves
            return;
        }
    } else if (_mode == GAME_MODE_4) {
        // Team A/C wins if B's home (2,6) and D's home (6,2) captured by Team A/C
        bool acBases = _board->squareAt(2, 6).isCapturedByTeamOf(PLAYER_A) &&
                       _board->squareAt(6, 2).isCapturedByTeamOf(PLAYER_A);
        if (acBases) {
            _gameOver = true;
            _winner = PLAYER_A;
            return;
        }

        // Team B/D wins if A's home (2,2) and C's home (6,6) captured by Team B/D
        bool bdBases = _board->squareAt(2, 2).isCapturedByTeamOf(PLAYER_B) &&
                       _board->squareAt(6, 6).isCapturedByTeamOf(PLAYER_B);
        if (bdBases) {
            _gameOver = true;
            _winner = PLAYER_B;
            return;
        }

        // Pods count
        int pods[5] = {0};
        int bw4 = _board->getBoardWidth();
        int bh4 = _board->getBoardHeight();
        for (int x = 0; x < bw4; x++) {
            for (int y = 0; y < bh4; y++) {
                const OctiSquare &sq = _board->squareAt(x, y);
                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPlayer p = podTypeToPlayer(sq.getPodAt(i).getType());
                    if (p >= 1 && p <= 4) pods[p]++;
                }
            }
        }

        bool bdElim = (_players[PLAYER_B].eliminated || pods[PLAYER_B] == 0) &&
                      (_players[PLAYER_D].eliminated || pods[PLAYER_D] == 0);
        if (bdElim) {
            _gameOver = true;
            _winner = PLAYER_A;
            return;
        }

        bool acElim = (_players[PLAYER_A].eliminated || pods[PLAYER_A] == 0) &&
                      (_players[PLAYER_C].eliminated || pods[PLAYER_C] == 0);
        if (acElim) {
            _gameOver = true;
            _winner = PLAYER_B;
            return;
        }
    }
}

void OctiGame::advanceTurn()
{
    if (_mode == GAME_MODE_2 || _mode == GAME_MODE_OFK) {
        _turn = (_turn == PLAYER_A) ? PLAYER_B : PLAYER_A;
    } else if (_mode == GAME_MODE_4) {
        int current = static_cast<int>(_turn);
        for (int step = 1; step <= 4; step++) {
            int nextP = (current - 1 + step) % 4 + 1;
            if (!_players[nextP].eliminated) {
                _turn = static_cast<OctiPlayer>(nextP);
                break;
            }
        }
    }
}

void OctiGame::undo()
{
    if (_undoStack.empty()) {
        return;
    }

    OctiGameSnapshot currentSnap = captureSnapshot();
    OctiGameSnapshot prevSnap = _undoStack.back();
    _undoStack.pop_back();
    _redoStack.push_back(currentSnap);

    if (!_history.empty()) {
        _history.pop_back();
    }

    restoreSnapshot(prevSnap);

    if (_board) {
        _board->notifyBoardTurnUpdate(nullptr);
    }
    notifyGameUpdated();
    notifyUndoRedoStatusChanged();
}

void OctiGame::redo()
{
    if (_redoStack.empty()) {
        return;
    }

    OctiGameSnapshot targetSnap = _redoStack.back();
    _redoStack.pop_back();
    OctiGameSnapshot currentSnap = captureSnapshot();
    _undoStack.push_back(currentSnap);

    if (targetSnap.lastPlayer != PLAYER_UNDEF && !targetSnap.lastMove.isEmpty()) {
        _history.push_back({targetSnap.lastPlayer, targetSnap.lastMove});
    }

    restoreSnapshot(targetSnap);

    if (_board) {
        _board->notifyBoardTurnUpdate(nullptr);
    }
    notifyGameUpdated();
    notifyUndoRedoStatusChanged();
}

void OctiGame::prepareAddPod(int x, int y)
{
    if (!_hasPreparedMove) {
        _prepSnapshot = captureSnapshot();
        _prepOps.clear();
        _hasPreparedMove = true;
    }

    OctiMoveOp op;
    op.x0 = x;
    op.y0 = y;
    op.addPod = true;
    op.liberate = false;
    _prepOps.push_back(op);
}

void OctiGame::prepareAddProng(int x, int y, OctiPod pod, OctiDir dir, bool superProng)
{
    if (!_hasPreparedMove) {
        _prepSnapshot = captureSnapshot();
        _prepOps.clear();
        _hasPreparedMove = true;
    }

    OctiMoveOp op;
    op.x0 = x;
    op.y0 = y;
    op.pod = pod;
    op.addProngDir = dir;
    op.superProng = superProng;
    _prepOps.push_back(op);
}

void OctiGame::prepareReposition(int x, int y, OctiPod pod, OctiDir srcDir, OctiDir tgtDir)
{
    if (!_hasPreparedMove) {
        _prepSnapshot = captureSnapshot();
        _prepOps.clear();
        _hasPreparedMove = true;
    }

    OctiMoveOp op;
    op.x0 = x;
    op.y0 = y;
    op.pod = pod;
    op.rmProngDir = srcDir;
    op.addProngDir = tgtDir;
    _prepOps.push_back(op);
}

void OctiGame::prepareHops(int x, int y, OctiPod pod, const std::vector<OctiHop> &hops)
{
    if (!_hasPreparedMove) {
        _prepSnapshot = captureSnapshot();
        _prepOps.clear();
        _hasPreparedMove = true;
    }

    OctiMoveOp op;
    op.x0 = x;
    op.y0 = y;
    op.pod = pod;
    op.hops = hops;
    _prepOps.push_back(op);
}

void OctiGame::prepareCapture(int x, int y)
{
    if (!_prepOps.empty() && !_prepOps.back().hops.empty()) {
        for (auto &h : _prepOps.back().hops) {
            if (h.x == x && h.y == y) {
                h.capture = true;
                break;
            }
        }
    }
}

void OctiGame::commitPreparedMove()
{
    if (!_hasPreparedMove) {
        throw IllegalMoveException("no prepared move to commit");
    }

    std::vector<OctiMoveOp> ops = _prepOps;
    rollbackPreparedMove();

    OctiMove m(ops);
    move(m);
}

void OctiGame::rollbackPreparedMove()
{
    if (_hasPreparedMove) {
        restoreSnapshot(_prepSnapshot);
        _hasPreparedMove = false;
        _prepOps.clear();
        notifyGameUpdated();
    }
}

bool OctiGame::hasPreparedMove() const
{
    return _hasPreparedMove;
}

std::string OctiGame::getPreparedMoveString() const
{
    if (!_hasPreparedMove || _prepOps.empty()) {
        return "";
    }
    return OctiMove::formatOps(_prepOps);
}

static void dfsJumpChains(const OctiBoard *board,
                          bool edgeless,
                          OctiPlayer turn,
                          int cx, int cy,
                          const OctiPod &pod,
                          const std::vector<OctiPod> &ghostPods,
                          std::vector<std::pair<int, int>> &jumpedSquares,
                          std::vector<OctiHop> &currentHops,
                          int sx, int sy,
                          std::vector<OctiMove> &outMoves)
{
    int w = board->getBoardWidth();
    int h = board->getBoardHeight();

    for (int d = 0; d < DIR_MAX; d++) {
        OctiDir dir = static_cast<OctiDir>(d);
        if (pod.prongAt(dir) == NO_PRONG) {
            continue;
        }

        int dx = dirDeltaX(dir) * 2;
        int dy = dirDeltaY(dir) * 2;
        int jdx = dirDeltaX(dir);
        int jdy = dirDeltaY(dir);

        int jx = edgeless ? (((cx + jdx) % w + w) % w) : (cx + jdx);
        int jy = edgeless ? (((cy + jdy) % h + h) % h) : (cy + jdy);

        int lx = edgeless ? (((cx + dx) % w + w) % w) : (cx + dx);
        int ly = edgeless ? (((cy + dy) % h + h) % h) : (cy + dy);

        if (!edgeless) {
            if (jx < 0 || jx >= w || jy < 0 || jy >= h) continue;
            if (lx < 0 || lx >= w || ly < 0 || ly >= h) continue;
        }

        std::pair<int, int> jCoord = {jx, jy};
        if (std::find(jumpedSquares.begin(), jumpedSquares.end(), jCoord) != jumpedSquares.end()) {
            continue;
        }

        const OctiSquare &jumpedSq = board->squareAt(jx, jy);
        if (isEmptyForJump(jumpedSq, jx, jy, sx, sy, ghostPods)) {
            continue;
        }

        const OctiSquare &landSq = board->squareAt(lx, ly);
        if (!isEmptyForJump(landSq, lx, ly, sx, sy, ghostPods)) {
            continue;
        }

        // Branch 1: Without capture
        {
            OctiHop hop;
            hop.x = lx;
            hop.y = ly;
            hop.capture = false;

            currentHops.push_back(hop);
            jumpedSquares.push_back(jCoord);

            OctiMoveOp op;
            op.x0 = sx;
            op.y0 = sy;
            op.pod = pod;
            op.hops = currentHops;
            outMoves.emplace_back(std::vector<OctiMoveOp>{op});

            // Recurse further along jump chain
            dfsJumpChains(board, edgeless, turn, lx, ly, pod, ghostPods,
                          jumpedSquares, currentHops, sx, sy, outMoves);

            jumpedSquares.pop_back();
            currentHops.pop_back();
        }

        // Branch 2: With capture (if jumped square has enemy)
        if (jumpedSq.hasEnemyOf(turn)) {
            OctiHop hop;
            hop.x = lx;
            hop.y = ly;
            hop.capture = true;

            currentHops.push_back(hop);
            jumpedSquares.push_back(jCoord);

            OctiMoveOp op;
            op.x0 = sx;
            op.y0 = sy;
            op.pod = pod;
            op.hops = currentHops;
            outMoves.emplace_back(std::vector<OctiMoveOp>{op});

            dfsJumpChains(board, edgeless, turn, lx, ly, pod, ghostPods,
                          jumpedSquares, currentHops, sx, sy, outMoves);

            jumpedSquares.pop_back();
            currentHops.pop_back();
        }
    }
}

void OctiGame::generateLegalMoves(std::vector<OctiMove> &outMoves) const
{
    outMoves.clear();
    if (!_gameStarted || _gameOver || _turn == PLAYER_UNDEF || !_board) {
        return;
    }

    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();

    // 1. Pod placements & liberations
    if (_mode != GAME_MODE_OFK) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const OctiSquare &sq = _board->squareAt(x, y);
                if (sq.getType() == NEUTRAL_SQUARE || sq.isEmpty() || sq.hasEnemyOf(_turn)) {
                    continue;
                }

                if (_mode == GAME_MODE_2) {
                    bool isFriendlyBase = (sq.getType() == (_turn == PLAYER_A ? A_HOME_SQUARE : B_HOME_SQUARE));
                    if (isFriendlyBase) {
                        if (_players[_turn].freePods > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        }
                        if (_board->canLiberate(_turn) && _players[_turn].capturedPods > 0) {
                            if (_players[_turn].freePods > 0) {
                                OctiMoveOp op;
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = true;
                                outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                            } else {
                                OctiMoveOp op;
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                            }
                        }
                    } else {
                        // Enemy base
                        if (_players[_turn].capturedPods > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        } else if (_players[_turn].freePods > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        }
                    }
                } else if (_mode == GAME_MODE_4) {
                    OctiSquareType homeType = (_turn == PLAYER_A ? A_HOME_SQUARE :
                                               _turn == PLAYER_B ? B_HOME_SQUARE :
                                               _turn == PLAYER_C ? C_HOME_SQUARE : D_HOME_SQUARE);
                    OctiPlayer partner = (_turn == PLAYER_A ? PLAYER_C :
                                          _turn == PLAYER_C ? PLAYER_A :
                                          _turn == PLAYER_B ? PLAYER_D : PLAYER_B);
                    OctiSquareType partnerHome = (partner == PLAYER_A ? A_HOME_SQUARE :
                                                  partner == PLAYER_B ? B_HOME_SQUARE :
                                                  partner == PLAYER_C ? C_HOME_SQUARE : D_HOME_SQUARE);

                    if (sq.getType() == homeType) {
                        if (_players[_turn].freePods > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        }
                    } else if (sq.getType() == partnerHome) {
                        if (_players[partner].eliminated) {
                            if (_players[partner].capturedPods > 0 || _players[_turn].freePods > 0) {
                                OctiMoveOp op;
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                            }
                        } else {
                            if (_players[_turn].freePods > 0) {
                                OctiMoveOp op;
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                            }
                        }
                    } else {
                        // Enemy base or neutral OCTI square
                        if (_players[_turn].capturedPods > 0 || _players[_turn].freePods > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        }
                    }
                }
            }
        }
    }

    // 2. Prong Additions, 3. Prong Repositions, 4. Distance-1 Moves, 5. Jumps
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const OctiSquare &sq = _board->squareAt(x, y);
            std::set<std::uint32_t> seenConfigs;

            for (int i = 0; i < sq.getNumPods(); i++) {
                const OctiPod &pod = sq.getPodAt(i);
                if (pod.getType() != playerToPodType(_turn)) {
                    continue;
                }
                if (seenConfigs.count(pod.rawConfig() & 0xFFFFu) > 0) {
                    continue;
                }
                seenConfigs.insert(pod.rawConfig() & 0xFFFFu);

                // 2. Prong Additions
                for (int d = 0; d < DIR_MAX; d++) {
                    OctiDir dir = static_cast<OctiDir>(d);
                    if (pod.prongAt(dir) == NO_PRONG) {
                        if (_players[_turn].freeProngs > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y;
                            op.pod = pod;
                            op.addProngDir = dir;
                            op.superProng = false;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        }
                        if (_useSuperProngs && _players[_turn].freeSuperProngs > 0) {
                            OctiMoveOp op;
                            op.x0 = x; op.y0 = y;
                            op.pod = pod;
                            op.addProngDir = dir;
                            op.superProng = true;
                            outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                        }
                    }
                }

                // 3. Prong Repositions
                if (_mode != GAME_MODE_OFK) {
                    for (int rm = 0; rm < DIR_MAX; rm++) {
                        OctiDir rmDir = static_cast<OctiDir>(rm);
                        if (pod.prongAt(rmDir) == NORMAL_PRONG) {
                            for (int add = 0; add < DIR_MAX; add++) {
                                OctiDir addDir = static_cast<OctiDir>(add);
                                if (addDir != rmDir && pod.prongAt(addDir) == NO_PRONG) {
                                    OctiMoveOp op;
                                    op.x0 = x; op.y0 = y;
                                    op.pod = pod;
                                    op.rmProngDir = rmDir;
                                    op.addProngDir = addDir;
                                    op.superProng = false;
                                    outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                                }
                            }
                        }
                    }
                }

                // 4. Distance-1 Moves
                for (int d = 0; d < DIR_MAX; d++) {
                    OctiDir dir = static_cast<OctiDir>(d);
                    if (pod.prongAt(dir) != NO_PRONG) {
                        int dx = dirDeltaX(dir);
                        int dy = dirDeltaY(dir);
                        int nx = _useEdgelessBoard ? (((x + dx) % w + w) % w) : (x + dx);
                        int ny = _useEdgelessBoard ? (((y + dy) % h + h) % h) : (y + dy);

                        if (!_useEdgelessBoard) {
                            if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                                continue;
                            }
                        }

                        const OctiSquare &destSq = _board->squareAt(nx, ny);
                        if (_mode == GAME_MODE_OFK) {
                            if (!destSq.isEmpty()) continue;
                        } else {
                            if (destSq.hasEnemyOf(_turn)) continue;
                        }

                        OctiHop hop;
                        hop.x = nx;
                        hop.y = ny;
                        hop.capture = false;

                        OctiMoveOp op;
                        op.x0 = x; op.y0 = y;
                        op.pod = pod;
                        op.hops.push_back(hop);
                        outMoves.emplace_back(std::vector<OctiMoveOp>{op});
                    }
                }

                // 5. Distance-2 Jumps
                std::vector<OctiPod> ghostPods = sq.pods();
                std::vector<std::pair<int, int>> jumpedSquares;
                std::vector<OctiHop> currentHops;
                dfsJumpChains(_board, _useEdgelessBoard, _turn, x, y, pod, ghostPods,
                              jumpedSquares, currentHops, x, y, outMoves);
            }
        }
    }
}

std::vector<OctiMove> OctiGame::generateLegalMoves() const
{
    std::vector<OctiMove> moves;
    generateLegalMoves(moves);
    return moves;
}

void OctiGame::serialize(std::ostream &out) const
{
    out << "; OCTI Game Transcript v2.0\n";

    if (_mode == GAME_MODE_2) {
        out << "GAME_MODE: 2\n";
    } else if (_mode == GAME_MODE_4) {
        out << "GAME_MODE: 4\n";
    } else if (_mode == GAME_MODE_OFK) {
        out << "GAME_MODE: OFK\n";
    }

    out << "GAME_BASES: " << static_cast<int>(_bases) << "\n";
    out << "USE_SUPER_PRONGS: " << (_useSuperProngs ? "Yes" : "No") << "\n";
    out << "USE_EDGELESS_BOARD: " << (_useEdgelessBoard ? "Yes" : "No") << "\n";

    out << "START_PLAYER: ";
    switch (_startPlayer) {
    case PLAYER_A: out << "A\n"; break;
    case PLAYER_B: out << "B\n"; break;
    case PLAYER_C: out << "C\n"; break;
    case PLAYER_D: out << "D\n"; break;
    default:       out << "A\n"; break;
    }

    out << "STATUS: ";
    if (!_gameStarted) {
        out << "NOT_STARTED\n";
    } else if (_gameResigned) {
        out << "RESIGNED\n";
    } else if (_gameOver) {
        out << "GAME_OVER\n";
    } else {
        out << "IN_PROGRESS\n";
    }

    out << "WINNER: ";
    switch (_winner) {
    case PLAYER_A: out << "A\n"; break;
    case PLAYER_B: out << "B\n"; break;
    case PLAYER_C: out << "C\n"; break;
    case PLAYER_D: out << "D\n"; break;
    default:       out << "NONE\n"; break;
    }

    out << "MOVES: " << _history.size() << "\n";
    for (const auto &entry : _history) {
        out << entry.second.asString() << "\n";
    }
}

std::string OctiGame::serialize() const
{
    std::ostringstream ss;
    serialize(ss);
    return ss.str();
}

static inline std::string trimString(const std::string &s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }
    if (start == s.size()) return "";
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }
    return s.substr(start, end - start);
}

void OctiGame::deserialize(std::istream &in)
{
    if (_board) {
        _board->deserializeGame(in);
        return;
    }
}

void OctiGame::deserialize(const std::string &transcript)
{
    std::istringstream ss(transcript);
    deserialize(ss);
}

OctiGameSnapshot OctiGame::captureSnapshot() const
{
    OctiGameSnapshot snap;
    if (_board) {
        int w = _board->getBoardWidth();
        int h = _board->getBoardHeight();
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                snap.squares[x][y] = _board->squareAt(x, y).pods();
            }
        }
    }
    for (int p = 0; p < 5; p++) {
        snap.players[p] = _players[p];
    }
    snap.turn = _turn;
    snap.winner = _winner;
    snap.gameStarted = _gameStarted;
    snap.gameOver = _gameOver;
    snap.gameResigned = _gameResigned;
    return snap;
}

void OctiGame::restoreSnapshot(const OctiGameSnapshot &snap)
{
    if (_board) {
        int w = _board->getBoardWidth();
        int h = _board->getBoardHeight();
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                OctiSquare &sq = _board->squareAt(x, y);
                sq.clear();
                for (const auto &pod : snap.squares[x][y]) {
                    sq.addPod(pod);
                }
            }
        }
    }
    for (int p = 0; p < 5; p++) {
        _players[p] = snap.players[p];
    }
    _turn = snap.turn;
    _winner = snap.winner;
    _gameStarted = snap.gameStarted;
    _gameOver = snap.gameOver;
    _gameResigned = snap.gameResigned;
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
