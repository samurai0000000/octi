/*
 * OctiDialogs.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_DIALOGS_HXX
#define OCTI_DESKTOP_OCTI_DIALOGS_HXX

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QWheelEvent>
#include "ClientConfigStore.hxx"
#include "OctiBoard.hxx"
#include "OctiBoardGeometry.hxx"
#include "OctiBotEngine.hxx"
#include "OctiDefs.hxx"
#include "OctiMoveComposer.hxx"
#include "OctiNetworkClient.hxx"
#include "OctiTheme.hxx"
#include "QtCompat.hxx"
#include "selfupdate/QtSelfUpdateBridge.hxx"
#include "selfupdate/SelfUpdateController.hxx"
#include <QRadioButton>

namespace octi {

class OctiMainWindow;

enum class LocalPlayType {
    PASS_AND_PLAY = 0,
    VS_AI_BOT     = 1,
};

struct LocalGameConfig {
    LocalPlayType playType = LocalPlayType::VS_AI_BOT;
    BotDifficulty botDifficulty = BotDifficulty::TACTICAL;
    OctiPlayer humanSeat = PLAYER_A;
    OctiGameMode mode = GAME_MODE_2;
    OctiGameBases bases = GAME_BASES_3;
    bool useSuperProngs = false;
    bool useEdgelessBoard = false;
    bool timeControlEnabled = false;
    long initialTimeMs = 600000;
    long incrementMs = 5000;
};

/*
 * 1. SsoOnboardingDialog:
 *    - Connects to hard-coded canonical endpoint with zero host/port displayed
 *      in the GUI and zero editable host/port inputs.
 *    - 5 SSO providers (Google, Apple, Microsoft, Facebook, LINE).
 *    - Browser loopback OAuth listener + manual token paste fallback.
 *    - Step-2 first-login handle setup when HTTP 201 Created (isNewAccount).
 */
class SsoOnboardingDialog : public QDialog {
public:
    explicit SsoOnboardingDialog(OctiNetworkClient *netClient,
                                 ClientConfigStore *configStore = nullptr,
                                 ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                 QWidget *parent = nullptr);
    ~SsoOnboardingDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    QString serverBadgeText() const;
    bool hasEditableServerOrPortField() const;

    bool startBrowserLoopbackFlow(const std::string &provider,
                                  bool openSystemBrowser = false);
    void stopBrowserLoopbackFlow();
    int loopbackBoundPort() const;
    std::string loopbackCallbackUrl() const;
    bool pollLoopbackCallback(int timeoutMs = 200);

    ApiResult<SsoLoginData> signInWithToken(
        const std::string &provider,
        const std::string &token,
        const std::string &desiredHandle = "",
        bool isAccessToken = false);

    bool isOnboardingStep2Active() const;
    ApiResult<PlayerProfileInfo> finalizeOnboardingHandle(
        const std::string &newHandle);

    bool isSignedIn() const;
    const PlayerProfileInfo &loggedInProfile() const;
    QString statusText() const;

    void setOnSignedInCallback(
        std::function<void(const PlayerProfileInfo &)> cb);

private:
    void persistLoginToConfig(const std::string &provider,
                              const std::string &sessionToken,
                              const PlayerProfileInfo &profile);

    OctiNetworkClient *_netClient;
    ClientConfigStore *_configStore;
    ThemeId _theme;
    UiDispatcher _dispatcher;
    OAuthLoopbackListener _loopbackListener;

    QLabel *_serverBadgeLabel;
    QLabel *_statusLabel;
    QComboBox *_providerCombo;
    QLineEdit *_tokenEdit;
    QLineEdit *_handleEdit;
    QPushButton *_signInBtn;
    QWidget *_step2Container;
    QLineEdit *_onboardingHandleEdit;
    QPushButton *_completeOnboardingBtn;

    bool _signedIn;
    bool _step2Active;
    std::string _lastProvider;
    PlayerProfileInfo _loggedInProfile;
    std::function<void(const PlayerProfileInfo &)> _onSignedInCallback;
};

/*
 * 2. AccountLinkDialog:
 *    - Lists linked SSO identities from fetchMyProfile().
 *    - Links additional providers (linkIdentity), handling 409 Conflict
 *      with an explicit "Switch to Existing Account" action.
 *    - Unlinks providers (unlinkIdentity), surfacing 400 last-identity guard.
 */
class AccountLinkDialog : public QDialog {
public:
    explicit AccountLinkDialog(OctiNetworkClient *netClient,
                               ClientConfigStore *configStore = nullptr,
                               ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                               QWidget *parent = nullptr);
    ~AccountLinkDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    bool refreshProfile();
    const PlayerProfileInfo &currentProfile() const;
    const std::vector<LinkedIdentityInfo> &linkedIdentities() const;

    ApiResult<LinkIdentityData> linkProviderIdentity(
        const std::string &provider,
        const std::string &token,
        bool isAccessToken = false);

    bool hasPendingConflict() const;
    const PlayerProfileInfo &conflictExistingProfile() const;
    ApiResult<SsoLoginData> confirmSwitchToConflictAccount(
        const std::string &provider = "",
        const std::string &token = "");
    void cancelConflictSwitch();

    ApiResult<UnlinkIdentityData> unlinkProviderIdentity(
        const std::string &provider);

    QString errorBannerText() const;
    QString conflictBannerText() const;

    void setOnAccountSwitchedCallback(
        std::function<void(const PlayerProfileInfo &)> cb);

private:
    void rebuildIdentitiesTable();

    OctiNetworkClient *_netClient;
    ClientConfigStore *_configStore;
    ThemeId _theme;

    QTableWidget *_identitiesTable;
    QComboBox *_providerCombo;
    QLineEdit *_tokenEdit;
    QPushButton *_linkBtn;
    QPushButton *_unlinkBtn;
    QLabel *_errorBannerLabel;
    QWidget *_conflictBox;
    QLabel *_conflictBannerLabel;
    QPushButton *_switchConflictBtn;
    QPushButton *_cancelConflictBtn;

    PlayerProfileInfo _currentProfile;
    std::vector<LinkedIdentityInfo> _identities;
    bool _hasConflict;
    std::string _conflictProvider;
    std::string _conflictToken;
    bool _conflictIsAccessToken;
    PlayerProfileInfo _conflictProfile;
    std::function<void(const PlayerProfileInfo &)> _onAccountSwitchedCallback;
};

/*
 * 3. CreateTableDialog & PasscodeDialog
 */
class CreateTableDialog : public QDialog {
public:
    explicit CreateTableDialog(OctiNetworkClient *netClient = nullptr,
                               ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                               QWidget *parent = nullptr);
    ~CreateTableDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    void setRequest(const CreateTableRequest &req);
    CreateTableRequest buildRequest() const;

    ApiResult<CreateTableResponse> submitCreateTable();
    const CreateTableResponse &createdTable() const;
    QString errorText() const;

    static std::string normalizeObserverPolicy(const std::string &policy);
    static std::string normalizeVoicePolicy(const std::string &policy);

private:
    OctiNetworkClient *_netClient;
    ThemeId _theme;

    QComboBox *_modeCombo;
    QComboBox *_basesCombo;
    QCheckBox *_superProngsCheck;
    QCheckBox *_edgelessCheck;
    QCheckBox *_ratedCheck;
    QCheckBox *_takebackCheck;
    QCheckBox *_timeControlCheck;
    QSpinBox *_initialSecSpin;
    QSpinBox *_incrementSecSpin;
    QComboBox *_observerPolicyCombo;
    QComboBox *_voicePolicyCombo;
    QLineEdit *_passcodeEdit;
    QLabel *_errorLabel;
    QPushButton *_createBtn;

    CreateTableResponse _createdTable;
};

class PasscodeDialog : public QDialog {
public:
    explicit PasscodeDialog(const std::string &roomId = "",
                            ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                            QWidget *parent = nullptr);
    ~PasscodeDialog() override;

    void setPasscode(const std::string &code);
    std::string passcode() const;

private:
    ThemeId _theme;
    QLineEdit *_passcodeEdit;
};

/*
 * 4. MatchmakingDialog
 */
class MatchmakingDialog : public QDialog {
public:
    explicit MatchmakingDialog(OctiNetworkClient *netClient,
                               ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                               QWidget *parent = nullptr);
    ~MatchmakingDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    void setRequest(const MatchmakingRequest &req);
    MatchmakingRequest buildRequest() const;

    ApiResult<bool> startMatchmaking();
    ApiResult<MatchmakingStatusData> pollMatchmakingStatus();
    ApiResult<bool> cancelMatchmaking();

    const MatchmakingStatusData &lastStatus() const;
    QString statusText() const;

    void setOnMatchedCallback(std::function<void(const std::string &roomId)> cb);

private:
    OctiNetworkClient *_netClient;
    ThemeId _theme;

    QComboBox *_modeCombo;
    QComboBox *_basesCombo;
    QCheckBox *_superProngsCheck;
    QCheckBox *_edgelessCheck;
    QCheckBox *_timeControlCheck;
    QSpinBox *_initialSecSpin;
    QSpinBox *_incrementSecSpin;
    QLabel *_statusLabel;
    QPushButton *_startBtn;
    QPushButton *_cancelBtn;
    QTimer _pollTimer;

    MatchmakingStatusData _lastStatus;
    std::function<void(const std::string &roomId)> _onMatchedCallback;
};

/*
 * 5. RoomModerationDialog
 */
class RoomModerationDialog : public QDialog {
public:
    explicit RoomModerationDialog(OctiNetworkClient *netClient,
                                  const std::string &roomId,
                                  ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                  QWidget *parent = nullptr);
    ~RoomModerationDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    bool refreshRoomView();
    void populateFromGameView(const GameViewData &view);
    const GameViewData &currentGameView() const;

    ApiResult<RoomSettingsData> applyPolicySettings(
        const std::string &observerPolicy,
        const std::string &voicePolicy);

    ApiResult<bool> muteParticipant(
        const std::string &targetUuid,
        bool muteChat,
        bool muteVoice);

    ApiResult<bool> kickAndBanObserver(
        const std::string &targetUuid,
        const std::string &reason = "");

    ApiResult<bool> kickWaitingSeat(int seat);
    ApiResult<bool> kickWaitingSeatByUuid(const std::string &targetUuid);

    QString statusText() const;

private:
    OctiNetworkClient *_netClient;
    std::string _roomId;
    ThemeId _theme;

    QComboBox *_observerPolicyCombo;
    QComboBox *_voicePolicyCombo;
    QPushButton *_applyPoliciesBtn;
    QTableWidget *_participantsTable;
    QLineEdit *_targetUuidEdit;
    QLineEdit *_banReasonEdit;
    QCheckBox *_muteChatCheck;
    QCheckBox *_muteVoiceCheck;
    QPushButton *_muteBtn;
    QPushButton *_banBtn;
    QPushButton *_kickSeatBtn;
    QLabel *_statusLabel;

    GameViewData _gameView;
};

/*
 * 6. ArchiveBrowserDialog & LeaderboardDialog
 */
class ArchiveBrowserDialog : public QDialog {
public:
    explicit ArchiveBrowserDialog(OctiNetworkClient *netClient,
                                  ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                  QWidget *parent = nullptr);
    ~ArchiveBrowserDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    ApiResult<ArchivedGamesPage> refreshArchiveList(
        const std::string &mode = "",
        const std::string &playerUuid = "",
        int limit = 20,
        int offset = 0);

    ApiResult<ArchivedGameSummary> inspectArchivedGame(
        const std::string &gameId);

    ApiResult<std::string> fetchTranscriptForGame(
        const std::string &gameId);

    bool loadGameIntoReplay(const std::string &gameId);

    const std::vector<ArchivedGameSummary> &archivedGames() const;
    const ArchivedGameSummary &inspectedGame() const;
    const std::string &lastDownloadedTranscript() const;

    void setOnLoadReplayCallback(
        std::function<void(const std::string &gameId,
                           const std::string &octiTranscript,
                           const ArchivedGameSummary &summary)> cb);

private:
    void rebuildGamesTable();

    OctiNetworkClient *_netClient;
    ThemeId _theme;

    QComboBox *_modeFilterCombo;
    QLineEdit *_playerFilterEdit;
    QPushButton *_refreshBtn;
    QTableWidget *_gamesTable;
    QPlainTextEdit *_transcriptPreview;
    QPushButton *_downloadBtn;
    QPushButton *_loadReplayBtn;
    QLabel *_statusLabel;

    std::vector<ArchivedGameSummary> _games;
    ArchivedGameSummary _inspectedGame;
    std::string _lastTranscript;
    std::function<void(const std::string &gameId,
                       const std::string &octiTranscript,
                       const ArchivedGameSummary &summary)> _onLoadReplayCallback;
};

class LeaderboardDialog : public QDialog {
public:
    explicit LeaderboardDialog(OctiNetworkClient *netClient,
                               ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                               QWidget *parent = nullptr);
    ~LeaderboardDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    ApiResult<std::vector<LeaderboardRow>> refreshLeaderboard(
        const std::string &mode = "2P",
        int limit = 50);

    const std::vector<LeaderboardRow> &rows() const;
    std::string selectedMode() const;

private:
    void rebuildTable();

    OctiNetworkClient *_netClient;
    ThemeId _theme;

    QComboBox *_modeCombo;
    QPushButton *_refreshBtn;
    QTableWidget *_table;
    std::vector<LeaderboardRow> _rows;
};

/*
 * 7. NewLocalGameDialog & GameOverDialog
 */
class NewLocalGameDialog : public QDialog {
public:
    explicit NewLocalGameDialog(ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                QWidget *parent = nullptr);
    ~NewLocalGameDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    void setConfig(const LocalGameConfig &cfg);
    LocalGameConfig config() const;

private:
    ThemeId _theme;

    QComboBox *_playTypeCombo;
    QComboBox *_difficultyCombo;
    QComboBox *_humanSeatCombo;
    QComboBox *_modeCombo;
    QComboBox *_basesCombo;
    QCheckBox *_superProngsCheck;
    QCheckBox *_edgelessCheck;
    QCheckBox *_timeControlCheck;
    QSpinBox *_initialSecSpin;
    QSpinBox *_incrementSecSpin;
};

class GameOverDialog : public QDialog {
public:
    explicit GameOverDialog(const std::string &winnerBanner,
                            const std::string &terminationReason,
                            int moveCount,
                            const std::string &octiTranscript = "",
                            ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                            QWidget *parent = nullptr);
    explicit GameOverDialog(const OctiBoard *board,
                            ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                            QWidget *parent = nullptr);
    ~GameOverDialog() override;

    static QString describeWinnerBanner(const OctiBoard *board);
    static QString describeTerminationReason(const OctiBoard *board);

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    QString winnerBannerText() const;
    QString terminationText() const;
    int moveCount() const;
    const std::string &octiTranscript() const;

    bool exportTranscriptToFile(const std::string &filePath) const;
    void triggerRematch();
    bool wasRematchRequested() const;
    bool newGameRequested() const;

    void setOnRematchCallback(std::function<void()> cb);

    QImage renderToImage(int width = 480, int height = 280);
    QRectF newGameBtnRect() const { return _newGameBtnRect; }
    QRectF reviewBoardBtnRect() const { return _reviewBoardBtnRect; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    ThemeId _theme;
    QString _winnerBanner;
    QString _terminationReason;
    int _moveCount;
    std::string _transcript;
    bool _rematchRequested;
    bool _newGameRequested;
    std::function<void()> _onRematchCallback;

    QRectF _newGameBtnRect;
    QRectF _reviewBoardBtnRect;
};

class SelfUpdateDialog : public QDialog {
public:
    explicit SelfUpdateDialog(
        std::shared_ptr<selfupdate::SelfUpdateController> controller,
        ThemeId theme = ThemeId::OBSIDIAN_BRASS,
        QWidget *parent = nullptr);
    ~SelfUpdateDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    void applyUpdateState(const selfupdate::UpdateState &state);
    const selfupdate::UpdateState &currentUpdateState() const;

    void triggerCheckForUpdates();
    void triggerDownloadAndInstall();
    void triggerCancelDownload();

    QString phaseBadgeText() const;
    QString versionSummaryText() const;
    QString releaseNotesText() const;
    QString metadataSummaryText() const;
    QString statusText() const;
    int progressPercent() const;
    bool isDownloadInstallButtonEnabled() const;
    QString downloadInstallButtonText() const;
    bool isCancelButtonEnabled() const;

private:
    std::shared_ptr<selfupdate::SelfUpdateController> _controller;
    OctiMainWindow *_ownerWindow;
    bool _boundStandalone;
    ThemeId _theme;
    selfupdate::UpdateState _currentState;

    QLabel *_phaseBadgeLabel;
    QLabel *_versionSummaryLabel;
    QLabel *_metadataLabel;
    QPlainTextEdit *_releaseNotesView;
    QProgressBar *_progressBar;
    QLabel *_statusLabel;
    QPushButton *_checkBtn;
    QPushButton *_downloadInstallBtn;
    QPushButton *_cancelBtn;
    QPushButton *_closeBtn;
};

/*
 * 12. OctiSplashDialog:
 *     10.0-second (10000 ms) luxury procedural animated splash screen.
 *     Obsidian & Brass tabletop, rotating brass ring, animated OCTI pod & superprong snapping,
 *     game credits, authorship, copyright, version 2.1.1, progress bar,
 *     and early dismissal via ESC or mouse click.
 */
class OctiSplashDialog : public QDialog {
public:
    explicit OctiSplashDialog(QWidget *parent = nullptr,
                              ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                              bool loopIndefinitely = false);
    ~OctiSplashDialog() override;

    bool isLoopingIndefinitely() const { return _loopIndefinitely; }
    static QImage renderSplashFrame(int width, int height, double progress);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    ThemeId _theme;
    bool _loopIndefinitely;
    QTimer _timer;
    QElapsedTimer _elapsed;
    double _progress;
};

/*
 * 13. ZoomedSquareDialog:
 *     Zoomed close-up square view opened on clicking any non-empty square
 *     (or eligible friendly home square for Reserve Pod / Liberation).
 *     Provides pod stack strip for >= 2 pods, move all matching stack toggle,
 *     interactive 320x320 zoomed octagonal pod canvas with 8 directional sockets,
 *     prong dispenser tray for dragging / repositioning prongs,
 *     and "Move / Jump This Pod" action button.
 */
class ZoomedSquareDialog : public QDialog {
public:
    explicit ZoomedSquareDialog(OctiBoard *board,
                                OctiMoveComposer *composer,
                                int squareX, int squareY,
                                BoardRotation rotation = BoardRotation::ROT_0,
                                ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                QWidget *parent = nullptr);
    ~ZoomedSquareDialog() override;

    int selectedPodIndex() const;
    bool shouldMovePod() const;
    bool moveAllMatchingStack() const;
    bool actionCompleted() const;
    void cycleSelectedPod(int delta);
    void setHoverProngDirForTest(int dir);
    OctiDir hoverProngDir() const { return _hoverProngDir; }
    const std::map<OctiDir, QPointF> &socketCenters() const { return _socketCenters; }
    QRectF movePodBtnRect() const { return _movePodBtnRect; }
    QRectF placeReserveBtnRect() const { return _placeReserveBtnRect; }
    QRectF liberateBtnRect() const { return _liberateBtnRect; }
    QRectF liberateButtonRect() const { return _liberateBtnRect; }
    const std::vector<QRectF> &stackPodTabRects() const { return _stackPodTabRects; }

    QImage renderToImage(int width = 560, int height = 530);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void updatePodSelection(int idx);
    void setupUi();
    OctiDir hitTestSocketOrBladeAt(const QPointF &pos,
                                   const OctiPod &pod,
                                   const QPointF &pc,
                                   double podR,
                                   double socketDist) const;

    OctiBoard *_board;
    OctiMoveComposer *_composer;
    int _squareX;
    int _squareY;
    BoardRotation _rotation;
    ThemeId _theme;

    int _selectedPodIndex;
    bool _shouldMovePod;
    bool _moveAllMatchingStack;
    bool _actionCompleted;

    QRectF _podCanvasRect;
    QRectF _stackStripRect;
    std::vector<QRectF> _stackPodTabRects;
    QRectF _moveStackCheckRect;
    QRectF _normalProngTrayRect;
    QRectF _superProngTrayRect;
    QRectF _movePodBtnRect;
    QRectF _placeReserveBtnRect;
    QRectF _liberateBtnRect;

    std::map<OctiDir, QPointF> _socketCenters;
    double _socketRadius;

    enum class SelectedProngType {
        NORMAL,
        SUPER
    };
    SelectedProngType _selectedProngType;
    OctiDir _selectedProngSocketForReposition;

    enum class ZoomDragKind {
        NONE,
        ADD_NORMAL_PRONG,
        ADD_SUPERPRONG,
        REPOSITION_PRONG
    };
    ZoomDragKind _dragKind;
    OctiDir _dragSrcProngDir;
    OctiDir _hoverProngDir;
    QPointF _dragCursorPos;
};

/*
 * 14. CapturePromptDialog:
 *     Prompts "Capture jumped pod(s) at Square (X, Y)?" with [Capture] and [Leave on Board]
 *     buttons, and an interactive mini-canvas where dragging the jumped pod off-board
 *     also triggers capture.
 */
class CapturePromptDialog : public QDialog {
public:
    explicit CapturePromptDialog(int jumpedX, int jumpedY,
                                 OctiPlayer jumpedPlayer,
                                 ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                 QWidget *parent = nullptr);
    ~CapturePromptDialog() override;

    bool captureChosen() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int _jumpedX;
    int _jumpedY;
    OctiPlayer _jumpedPlayer;
    ThemeId _theme;
    bool _captureChosen;

    QRectF _miniCanvasRect;
    QRectF _podIconRect;
    QRectF _captureBtnRect;
    QRectF _leaveBtnRect;

    bool _draggingPod;
    QPointF _dragPos;
};

/*
 * 15. SuperProngBonusDialog:
 *     Prompts "8th-Prong Superprong Bonus Move Available!"
 *     with [Move / Jump Now] and [Skip Bonus Move] buttons.
 */
class SuperProngBonusDialog : public QDialog {
public:
    explicit SuperProngBonusDialog(ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                   QWidget *parent = nullptr);
    ~SuperProngBonusDialog() override;

    bool moveNowChosen() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ThemeId _theme;
    bool _moveNowChosen;
    QPushButton *_moveNowBtn;
    QPushButton *_skipBtn;
};

/*
 * 16. CompositePodMoveDialog:
 *     Prompts "Another pod on Square (X, Y) can also move this turn."
 *     with [Select Next Pod] and [Proceed to Confirm Move] buttons.
 */
class CompositePodMoveDialog : public QDialog {
public:
    explicit CompositePodMoveDialog(int x, int y,
                                    ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                    QWidget *parent = nullptr);
    ~CompositePodMoveDialog() override;

    bool selectNextPodChosen() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int _x;
    int _y;
    ThemeId _theme;
    bool _selectNextPodChosen;
    QPushButton *_selectNextBtn;
    QPushButton *_confirmBtn;
};

/*
 * 17. PlyConfirmDialog (GUI2-05):
 *     Borderless, pure-rendering ply confirmation dialog.
 *     Presents Commit [Enter], Cancel [Esc], and optional More Moves [M] buttons
 *     while the live staged board preview remains clearly visible.
 */
class PlyConfirmDialog : public QDialog {
public:
    enum class ResultAction {
        COMMIT,
        CANCEL
    };

    explicit PlyConfirmDialog(OctiMoveComposer *composer,
                              ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                              QWidget *parent = nullptr);
    ~PlyConfirmDialog() override;

    ResultAction chosenAction() const;
    QString stagedMoveText() const;

    QImage renderToImage(int width = 420, int height = 150);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    OctiMoveComposer *_composer;
    ThemeId _theme;
    ResultAction _chosenAction;
    QString _notation;

    QRectF _commitBtnRect;
    QRectF _cancelBtnRect;
};

/*
 * Modal Borderless Non-Obstructing "AI Thinking" Progress Dialog (EMF-D02)
 */
class OctiThinkingDialog : public QDialog {
public:
    explicit OctiThinkingDialog(BotDifficulty difficulty = BotDifficulty::TACTICAL,
                                int budgetMs = 8000,
                                ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                QWidget *parent = nullptr);
    ~OctiThinkingDialog() override;

    void updateProgress(int elapsedMs, int totalBudgetMs,
                        int depthReached = 0,
                        std::uint64_t nodesVisited = 0,
                        int bestScore = 0);
    int sliderValue() const;
    QString statusText() const;
    QString progressBarText() const;
    bool isFrameless() const;
    void positionBelowBoard(QWidget *mainWindow, QWidget *canvas = nullptr);

private:
    BotDifficulty _difficulty;
    int _budgetMs;
    ThemeId _theme;
    QLabel *_statusLabel;
    QWidget *_progressBar;
    int _sliderVal;
    QString _progressBarText;
};

/*
 * Non-Dismissible Startup Telemetry Consent & CC-BY 4.0 Disclosure Modal
 */
class OctiTelemetryConsentDialog : public QDialog {
public:
    explicit OctiTelemetryConsentDialog(ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                        QWidget *parent = nullptr);
    ~OctiTelemetryConsentDialog() override;

    bool userConsented() const { return _consented; }
    void reject() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    bool _consented;
    bool _choiceMade;
    ThemeId _theme;
    QPlainTextEdit *_sampleJsonEdit;
    QPushButton *_togglePreviewBtn;
};

/*
 * Settings Dialog for Telemetry & Privacy (Tier 1 Consent Toggle, Install ID Reset)
 */
class OctiTelemetrySettingsDialog : public QDialog {
public:
    explicit OctiTelemetrySettingsDialog(class ClientConfigStore *configStore,
                                        class OctiNetworkClient *netClient = nullptr,
                                        ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                        QWidget *parent = nullptr);
    ~OctiTelemetrySettingsDialog() override;

private:
    class ClientConfigStore *_configStore;
    class OctiNetworkClient *_netClient;
    ThemeId _theme;
    QRadioButton *_shareRadio;
    QRadioButton *_declineRadio;
    QLabel *_installIdLabel;
};

/*
 * Modal Borderless "Start Match" Dialog Centered Over Board
 */
class OctiStartMatchDialog : public QDialog {
public:
    explicit OctiStartMatchDialog(OctiPlayer humanSeat = PLAYER_A,
                                  bool vsBot = true,
                                  ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                                  QWidget *parent = nullptr);
    ~OctiStartMatchDialog() override;

    void positionCenterOfBoard(QWidget *mainWin, QWidget *boardCanvas = nullptr);
    void reject() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    OctiPlayer _humanSeat;
    bool _vsBot;
    ThemeId _theme;
};

/*
 * Procedural Vector Menu Icon Generator (GUI-04)
 */
enum class MenuIconKind {
    NEW_GAME,
    BOARD_VIEW,
    LOBBY_VIEW,
    OFFER_DRAW,
    PAUSE_RESUME,
    UNDO_MOVE,
    CLAIM_FORFEIT,
    RESIGN,
    QUIT,
    ROTATE_BOARD,
    SWITCH_THEME,
    TOGGLE_SFX,
    VCR_FIRST,
    VCR_PREV,
    VCR_NEXT,
    VCR_LAST,
    SIGN_IN,
    ACCOUNT_LINK,
    GAME_ARCHIVE,
    LEADERBOARD,
    MODERATION,
    CHECK_UPDATE,
    SHOW_SPLASH,
};

QIcon createMenuIcon(MenuIconKind kind, ThemeId theme = ThemeId::OBSIDIAN_BRASS);

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_DIALOGS_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
