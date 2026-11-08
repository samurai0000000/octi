/*
 * OctiMainWindow.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_MAIN_WINDOW_HXX
#define OCTI_DESKTOP_OCTI_MAIN_WINDOW_HXX

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>
#include "ClientConfigStore.hxx"
#include "LobbyView.hxx"
#include "OctiBoard.hxx"
#include "OctiBoardCanvas.hxx"
#include "OctiBotEngine.hxx"
#include "OctiDialogs.hxx"
#include "OctiMoveComposer.hxx"
#include "OctiNetworkClient.hxx"
#include "OctiReservePanel.hxx"
#include "OctiSoundSynth.hxx"
#include "OctiTheme.hxx"
#include "QtCompat.hxx"
#include "selfupdate/QtSelfUpdateBridge.hxx"
#include "selfupdate/SelfUpdateController.hxx"

namespace octi {

struct DesktopCliArgs {
    bool showHelp = false;
    bool showVersion = false;
    bool startOffline = false;
    std::string configPath;

    static bool parse(int argc,
                      const char * const argv[],
                      DesktopCliArgs &outArgs,
                      std::string &errMsg);
    static std::string versionBanner();
    static std::string usageBanner(const char *progName);
};

class OctiMainWindow : public QMainWindow {
public:
    explicit OctiMainWindow(
        std::unique_ptr<OctiNetworkClient> netClient = nullptr,
        const std::string &configPath = "",
        bool startOffline = false,
        QWidget *parent = nullptr);
    ~OctiMainWindow() override;

    /* ---- Component Accessors ---- */
    OctiBoard *board();
    const OctiBoard *board() const;
    OctiBoardCanvas *boardCanvas() const;
    OctiReservePanel *reservePanel() const;
    OctiMoveComposer *moveComposer();
    LobbyView *lobbyView() const;
    OctiNetworkClient *netClient() const;
    OctiSoundSynth *soundSynth();
    ClientConfigStore *configStore();

    /* ---- Canonical Server & Profile Status ---- */
    QString serverStatusText() const;
    bool hasEditableServerOrPortField() const;
    QString profileBadgeText() const;
    QLabel *profileAvatarLabel() const { return _profileAvatarLabel; }
    bool isAuthenticated() const;
    const PlayerProfileInfo &currentProfile() const;
    void applyAuthenticatedProfile(const PlayerProfileInfo &profile);
    void refreshAuthenticatedProfileFromServer();

    /* ---- View Mode & Tabletop Controls ---- */
    void showBoardView();
    void showLobbyView();
    bool isBoardViewVisible() const;
    bool isLobbyViewVisible() const;

    void rotateBoardClockwise();
    void setBoardRotation(BoardRotation rot);
    BoardRotation boardRotation() const;

    void setConfirmAllMoves(bool confirmAll);
    bool confirmAllMoves() const;

    void toggleTheme();
    void setTheme(ThemeId theme);
    ThemeId theme() const;
    QComboBox *themeSelectorCombo() const { return _themeCombo; }

    int menuActionCountWithIconsAndShortcuts() const;
    void showSplashAboutDialog();

    void setSfxMuted(bool muted);
    bool isSfxMuted() const;

    /* ---- Offline Local / Bot Play ---- */
    void startNewLocalGame(const LocalGameConfig &cfg);
    bool isLocalBotGame() const;
    const LocalGameConfig &localGameConfig() const;
    bool executeBotTurnIfNeeded();
    void showHint();
    bool isAwaitingMatchStart() const { return _awaitingMatchStart; }
    void beginAwaitingMatchStart();
    void confirmMatchStart();
    void promptStartMatchIfVisible();
    void reconfigurePendingMatch(const LocalGameConfig &cfg);
    void syncSidePanelsToBoardEdges();
    OctiStartMatchDialog *startMatchOverlay() const { return _startMatchOverlay; }
    OctiStartMatchDialog *startMatchOverlayForTest() const { return _startMatchOverlay; }
    QRect rightPanelTopWidgetGeometry() const;
    QRect rightPanelBottomWidgetGeometry() const;
    std::int64_t matchStartedEpochSec() const { return _matchStartedEpochSec; }
    std::int64_t currentTurnStartedEpochMs() const { return _currentTurnStartedEpochMs; }

    /* ---- Online Multiplayer Room Lifecycle ---- */
    ApiResult<CreateTableResponse> createAndEnterOnlineTable(
        const CreateTableRequest &req);
    ApiResult<JoinTableResponse> joinAndEnterOnlineTable(
        const std::string &roomId,
        int seat = 0,
        const std::string &passcode = "");
    ApiResult<bool> observeOnlineTable(
        const std::string &roomId,
        const std::string &passcode = "");
    ApiResult<bool> leaveActiveOnlineTable(
        const std::string &roomId = "");
    void leaveOnlineRoom();

    bool isInOnlineGame() const;
    const std::string &onlineRoomId() const;
    int onlineViewerSeat() const;
    bool isInteractiveOnlineTurn() const;
    std::string onlineRoomPhase() const { return _onlineGameView.phase; }
    const GameViewData &currentOnlineGameView() const;
    bool isWaitingRoomBannerVisible() const;
    QString waitingRoomBannerText() const;
    QPushButton *leaveTableButton() const { return _leaveTableBtn; }
    QPushButton *leaveWaitingTableButton() const { return _leaveWaitingTableBtn; }
    QPushButton *sitSeatButton() const { return _sitSeatBtn; }
    QPushButton *undoButton() const { return _undoRequestBtn; }
    QPushButton *undoRequestButton() const { return _undoRequestBtn; }
    QAction *undoAction() const { return _actUndo; }
    QLabel *matchTurnStatusLabel() const { return _matchTurnStatusLabel; }
    bool isGameOverDialogShownForCurrentMatch() const { return _gameOverDialogShownForCurrentMatch; }
    void tickLocalClock(int elapsedMs = 100);

    void updateMenuActionsState();
    void syncBoardLobbyChatTranscript();
    void sendBoardLobbyChatMessage();

    QWidget *leftMiddleContainer() const { return _leftMiddleContainer; }
    QGroupBox *statusBox() const { return _statusBox; }
    QGroupBox *historyBox() const { return _historyBox; }
    QGroupBox *boardLobbyChatBox() const { return _boardLobbyChatBox; }
    QPlainTextEdit *boardLobbyChatTranscript() const { return _boardLobbyChatTranscript; }
    QLineEdit *boardLobbyChatInput() const { return _boardLobbyChatInput; }
    QPushButton *boardLobbyChatSendButton() const { return _boardLobbyChatSendBtn; }
    QGroupBox *chatBox() const { return qobject_cast<QGroupBox *>(_chatBox); }
    QGroupBox *actionsBox() const { return _actionsBox; }
    QPushButton *tableChatLeaveButton() const { return _tableChatLeaveBtn; }
    QPushButton *tableChatPurgeButton() const { return _tableChatPurgeBtn; }
    QPushButton *zoomTableChatButton() const { return _zoomChatBtn; }
    QLabel *vcrPlyLabel() const { return _vcrPlyLabel; }

    QAction *actionNewLocal() const { return _actNewGame; }
    QAction *actionBoardView() const { return _actBoardView; }
    QAction *actionLobbyView() const { return _actLobbyView; }
    QAction *actionOfferDraw() const { return _actOfferDraw; }
    QAction *actionPauseResume() const { return _actPauseResume; }
    QAction *actionUndo() const { return _actUndo; }
    QAction *actionClaimForfeit() const { return _actClaimForfeit; }
    QAction *actionResign() const { return _actResign; }
    QAction *actionExportMoves() const { return _actExportMoves; }
    QAction *actionQuit() const { return _actQuit; }
    QAction *actionRotate() const { return _actRotate; }
    QAction *actionFlipPerspective() const { return _actFlipPerspective; }
    QAction *actionToggleCoords() const { return _actToggleCoords; }
    QAction *actionVcrFirst() const { return _actVcrFirst; }
    QAction *actionVcrPrev() const { return _actVcrPrev; }
    QAction *actionVcrNext() const { return _actVcrNext; }
    QAction *actionVcrLast() const { return _actVcrLast; }
    QAction *actionSignIn() const { return _actSignIn; }
    QAction *actionAccount() const { return _actAccount; }
    QAction *actionArchive() const { return _actArchive; }
    QAction *actionLeaderboard() const { return _actLeaderboard; }
    QAction *actionRobots() const { return _actRobots; }
    QAction *actionModeration() const { return _actModeration; }

    bool refreshOnlineGameView();
    void applyGameViewData(const GameViewData &view);
    void handleGameSseEvent(const SseEvent &ev);
    void pumpUiEvents();

    /* ---- Board Chat & Mutual Proposals ---- */
    void openBoardChatZoomDialog();
    bool sendGameChatMessage(const std::string &scope,
                             const std::string &text);
    const std::vector<std::string> &gameChatLines() const;
    QLineEdit *gameChatInput() const { return _gameChatInput; }
    QPushButton *sendGameChatButton() const { return _sendGameChatBtn; }

    bool hasActiveProposal() const;
    std::string activeProposalType() const;
    QString proposalBannerText() const;

    ApiResult<GameActionData> proposeDraw();
    ApiResult<GameActionData> proposePauseOrResume();
    ApiResult<GameActionData> proposeUndo();
    ApiResult<GameActionData> proposeRematch();
    ApiResult<GameActionData> claimOpponentForfeit();
    ApiResult<bool> resignCurrentGame();
    ApiResult<GameActionData> acceptActiveProposal();
    ApiResult<GameActionData> declineActiveProposal();

    /* ---- Move History, VCR Replay & .octi Transcript ---- */
    void vcrFirst();
    void vcrPrev();
    void vcrNext();
    void vcrLast();
    int vcrCurrentPly() const;
    int vcrTotalPlies() const;
    QString fenStatusText() const;
    std::vector<std::string> moveHistoryLines() const;
    QString exportMoveHistoryText() const;
    QListWidget *moveHistoryListWidget() const;

    bool loadOctiTranscriptIntoReplay(const std::string &octiTranscript);
    std::string exportCurrentOctiTranscript() const;
    std::string buildCurrentGameTranscriptJson(bool includeExtendedTelemetry = false) const;
    void setTelemetryConsentForTesting(const std::string &choice);
    bool uploadCompletedBotGameIfEligible(bool synchronousForTest = false);
    void waitForBackgroundUpload();
    void flushPendingTelemetry(bool async = true);
    void checkPromptTelemetryConsentAtStartup();
    void showTelemetrySettingsDialog();
    QString copyExportedNotationToClipboard() const;
    void showExportNotationDialog();
    bool saveTranscriptToFile(const std::string &filePath) const;
    bool loadTranscriptFromFile(const std::string &filePath);

    /* ---- Offscreen Screenshot Capture ---- */
    QImage renderWindowToImage(int width = 1440, int height = 900);

    /* ---- Self-Update Control & Status ---- */
    static selfupdate::CheckerConfig defaultSelfUpdateConfig();
    selfupdate::SelfUpdateController *selfUpdateController() const;
    std::shared_ptr<selfupdate::SelfUpdateController>
    selfUpdateControllerShared() const;
    void setSelfUpdateController(
        std::shared_ptr<selfupdate::SelfUpdateController> controller);

    void checkForAppUpdates(bool isManualCheck = false);
    void triggerAppUpdateDownloadAndInstall();
    void cancelAppUpdateDownload();
    void dismissAppUpdateBanner();

    const selfupdate::UpdateState &currentSelfUpdateState() const;
    bool isUpdateBannerVisible() const;
    QString updateBannerText() const;
    QString checkUpdateButtonText() const;
    bool wasQuitRequestedByUpdate() const;

    void registerUpdateDialog(SelfUpdateDialog *dlg);
    void unregisterUpdateDialog(SelfUpdateDialog *dlg);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void handleLocalOrOnlineMoveCommitted(const std::string &moveNotation);
    void updateTopBarLabels();
    void refreshMoveHistoryAndFenUi();
    void updateProposalBannerUi();
    void updateMatchActionsVisibility();
    void checkAndPresentGameOver();
    void bindSelfUpdateController();
    void handleSelfUpdateStateChanged(const selfupdate::UpdateState &state);
    void handleSelfUpdateQuitRequested();
    void updateSelfUpdateBannerUi();
    void showProtocolUpgradeRequired(const std::string &message = "");
    void setupMenuBar();
    void updateStartMatchOverlay();

    ClientConfigStore _configStore;
    std::unique_ptr<OctiNetworkClient> _netClient;
    OctiSoundSynth _soundSynth;
    OctiBoard _board;
    OctiMoveComposer _composer;
    OctiBotEngine _botEngine;
    UiDispatcher _dispatcher;

    ThemeId _theme;
    bool _offlineMode;
    bool _authenticated;
    PlayerProfileInfo _profile;
    std::string _cachedAvatarUrl;
    QImage _cachedAvatarImage;

    bool _isLocalBotGame;
    LocalGameConfig _localConfig;
    bool _awaitingMatchStart;
    qint64 _currentTurnStartedEpochMs;
    std::int64_t _matchStartedEpochSec;
    bool _transcriptUploadedForCurrentMatch;
    bool _heldTranscriptPendingConsent;
    std::thread _uploadThread;

    bool _inOnlineRoom;
    std::string _onlineRoomId;
    std::string _pendingOnlinePasscode;
    int _onlineViewerSeat;
    bool _onlineAutoOriented;
    int _lastOrientedViewerSeat = 0;
    GameViewData _onlineGameView;
    std::vector<std::string> _appliedOnlineMoves;
    std::vector<std::string> _gameChatLines;
    std::int64_t _lastGameChatSeq;
    std::int64_t _lastAppliedGameEventSeq = 0;
    int _vcrTotalPlies;

    /* ---- Top Navigation Bar Widgets ---- */
    QLabel *_brandLabel;
    QLabel *_serverStatusLabel;
    QLabel *_profileBadgeLabel;
    QLabel *_profileAvatarLabel;
    QPushButton *_signInBtn;
    QPushButton *_accountLinkBtn;
    QPushButton *_boardViewBtn;
    QPushButton *_lobbyViewBtn;
    QPushButton *_rotateBoardBtn;
    QCheckBox *_confirmAllCheck;
    QPushButton *_themeToggleBtn;
    QComboBox *_themeCombo;
    QPushButton *_sfxMuteBtn;
    QPushButton *_newLocalGameBtn;
    QPushButton *_archiveBtn;
    QPushButton *_leaderboardBtn;

    /* ---- Center Stacked Views ---- */
    QStackedWidget *_viewStack;
    QWidget *_boardPage;
    OctiReservePanel *_reservePanel;
    OctiBoardCanvas *_boardCanvas;
    LobbyView *_lobbyView;

    /* ---- Right Side Panel Widgets ---- */
    QLabel *_matchModeLabel;
    QLabel *_matchTurnStatusLabel;
    QPushButton *_sidebarNewGameBtn;
    QListWidget *_moveHistoryList;
    QPushButton *_vcrFirstBtn;
    QPushButton *_vcrPrevBtn;
    QPushButton *_vcrNextBtn;
    QPushButton *_vcrLastBtn;
    QPushButton *_exportNotationBtn;
    QLabel *_fenStatusLabel;

    QWidget *_waitingRoomBannerWidget = nullptr;
    QLabel *_waitingRoomBannerLabel = nullptr;
    QPushButton *_sitSeatBtn = nullptr;
    QPushButton *_deleteWaitingTableBtn = nullptr;
    QPushButton *_leaveWaitingTableBtn = nullptr;
    QAction *_actNewGame = nullptr;
    QAction *_actUndo = nullptr;
    QAction *_actBoardView = nullptr;
    QAction *_actLobbyView = nullptr;
    QAction *_actOfferDraw = nullptr;
    QAction *_actPauseResume = nullptr;
    QAction *_actClaimForfeit = nullptr;
    QAction *_actResign = nullptr;
    QAction *_actExportMoves = nullptr;
    QAction *_actQuit = nullptr;
    QAction *_actRotate = nullptr;
    QAction *_actFlipPerspective = nullptr;
    QAction *_actToggleCoords = nullptr;
    QAction *_actTheme = nullptr;
    QAction *_actSfx = nullptr;
    QAction *_actVcrFirst = nullptr;
    QAction *_actVcrPrev = nullptr;
    QAction *_actVcrNext = nullptr;
    QAction *_actVcrLast = nullptr;
    QAction *_actSignIn = nullptr;
    QAction *_actAccount = nullptr;
    QAction *_actArchive = nullptr;
    QAction *_actLeaderboard = nullptr;
    QAction *_actRobots = nullptr;
    QAction *_actModeration = nullptr;
    QAction *_actUpdate = nullptr;
    QAction *_actSplash = nullptr;
    QAction *_actTelemetry = nullptr;

    QWidget *_proposalBannerWidget;
    QLabel *_proposalBannerLabel;
    QPushButton *_acceptProposalBtn;
    QPushButton *_declineProposalBtn;

    QPushButton *_offerDrawBtn;
    QPushButton *_pauseResumeBtn;
    QPushButton *_undoRequestBtn;
    QPushButton *_claimForfeitBtn;
    QPushButton *_resignBtn;
    QPushButton *_leaveTableBtn = nullptr;
    QPushButton *_moderationBtn;

    QWidget *_rightTopHalfWidget = nullptr;
    QWidget *_chatBox;
    QPushButton *_zoomChatBtn = nullptr;
    QPointer<QDialog> _boardChatZoomDialog;
    QComboBox *_chatScopeCombo;
    QPlainTextEdit *_gameChatTranscript;
    QLineEdit *_gameChatInput;
    QPushButton *_sendGameChatBtn;

    bool _gameOverDialogShownForCurrentMatch;

    /* ---- Self-Update Subsystem ---- */
    std::shared_ptr<selfupdate::SelfUpdateController> _updateController;
    selfupdate::UpdateState _currentUpdateState;
    SelfUpdateDialog *_activeUpdateDialog;
    bool _updateBannerDismissed;
    bool _quitRequestedByUpdate;

    QPushButton *_checkUpdateBtn;
    QWidget *_updateBannerWidget;
    QLabel *_updateBannerLabel;
    QProgressBar *_updateBannerProgressBar;
    QPushButton *_updateActionBtn;
    QPushButton *_updateDetailsBtn;
    QPushButton *_updateDismissBtn;

    QWidget *_rightSide = nullptr;
    class QVBoxLayout *_rightLayout = nullptr;
    class QGroupBox *_statusBox = nullptr;
    class QGroupBox *_historyBox = nullptr;
    class QGroupBox *_actionsBox = nullptr;
    class QGroupBox *_boardLobbyChatBox = nullptr;
    QPlainTextEdit *_boardLobbyChatTranscript = nullptr;
    QLineEdit *_boardLobbyChatInput = nullptr;
    QPushButton *_boardLobbyChatSendBtn = nullptr;
    QPushButton *_tableChatLeaveBtn = nullptr;
    QPushButton *_tableChatPurgeBtn = nullptr;
    QLabel *_vcrPlyLabel = nullptr;
    QWidget *_leftMiddleContainer = nullptr;
    OctiStartMatchDialog *_startMatchOverlay = nullptr;
    QTimer *_onlineTtlTimer = nullptr;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_MAIN_WINDOW_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
