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
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
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
#include "P2pVoiceEngine.hxx"
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
    P2pVoiceEngine *voiceEngine() const;
    OctiSoundSynth *soundSynth();
    ClientConfigStore *configStore();

    /* ---- Canonical Server & Profile Status ---- */
    QString serverStatusText() const;
    bool hasEditableServerOrPortField() const;
    QString profileBadgeText() const;
    bool isAuthenticated() const;
    const PlayerProfileInfo &currentProfile() const;
    void applyAuthenticatedProfile(const PlayerProfileInfo &profile);

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

    int menuActionCountWithIconsAndShortcuts() const;
    void showSplashAboutDialog();

    void setSfxMuted(bool muted);
    bool isSfxMuted() const;

    /* ---- Offline Local / Bot Play ---- */
    void startNewLocalGame(const LocalGameConfig &cfg);
    bool isLocalBotGame() const;
    const LocalGameConfig &localGameConfig() const;
    bool executeBotTurnIfNeeded();
    bool isAwaitingMatchStart() const { return _awaitingMatchStart; }
    void beginAwaitingMatchStart();
    void confirmMatchStart();
    void promptStartMatchIfVisible();
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
    void leaveOnlineRoom();

    bool isInOnlineGame() const;
    const std::string &onlineRoomId() const;
    int onlineViewerSeat() const;
    const GameViewData &currentOnlineGameView() const;

    bool refreshOnlineGameView();
    void applyGameViewData(const GameViewData &view);
    void handleGameSseEvent(const SseEvent &ev);
    void pumpUiEvents();

    /* ---- Board Chat & Mutual Proposals ---- */
    bool sendGameChatMessage(const std::string &scope,
                             const std::string &text);
    const std::vector<std::string> &gameChatLines() const;

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

    /* ---- Push-to-Talk Voice Control ---- */
    bool joinOnlineVoice();
    void leaveOnlineVoice();
    bool isInVoiceRoom() const;
    bool setPushToTalkActive(bool talking);
    bool isPushToTalkActive() const;
    void simulatePushToTalkKeyPress();
    void simulatePushToTalkKeyRelease();

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
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void handleLocalOrOnlineMoveCommitted(const std::string &moveNotation);
    void updateTopBarLabels();
    void refreshMoveHistoryAndFenUi();
    void updateProposalBannerUi();
    void updateVoiceControlUi();
    void updateMatchActionsVisibility();
    void checkAndPresentGameOver();
    void bindSelfUpdateController();
    void handleSelfUpdateStateChanged(const selfupdate::UpdateState &state);
    void handleSelfUpdateQuitRequested();
    void updateSelfUpdateBannerUi();
    void showProtocolUpgradeRequired(const std::string &message = "");
    void setupMenuBar();

    ClientConfigStore _configStore;
    std::unique_ptr<OctiNetworkClient> _netClient;
    std::unique_ptr<P2pVoiceEngine> _voiceEngine;
    OctiSoundSynth _soundSynth;
    OctiBoard _board;
    OctiMoveComposer _composer;
    OctiBotEngine _botEngine;
    UiDispatcher _dispatcher;

    ThemeId _theme;
    bool _offlineMode;
    bool _authenticated;
    PlayerProfileInfo _profile;

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
    int _onlineViewerSeat;
    bool _onlineAutoOriented;
    GameViewData _onlineGameView;
    std::vector<std::string> _appliedOnlineMoves;
    std::vector<std::string> _gameChatLines;
    int _vcrTotalPlies;

    /* ---- Top Navigation Bar Widgets ---- */
    QLabel *_brandLabel;
    QLabel *_serverStatusLabel;
    QLabel *_profileBadgeLabel;
    QPushButton *_signInBtn;
    QPushButton *_accountLinkBtn;
    QPushButton *_boardViewBtn;
    QPushButton *_lobbyViewBtn;
    QPushButton *_rotateBoardBtn;
    QCheckBox *_confirmAllCheck;
    QPushButton *_themeToggleBtn;
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

    QWidget *_proposalBannerWidget;
    QLabel *_proposalBannerLabel;
    QPushButton *_acceptProposalBtn;
    QPushButton *_declineProposalBtn;

    QPushButton *_offerDrawBtn;
    QPushButton *_pauseResumeBtn;
    QPushButton *_undoRequestBtn;
    QPushButton *_claimForfeitBtn;
    QPushButton *_resignBtn;
    QPushButton *_moderationBtn;

    QWidget *_chatBox;
    QComboBox *_chatScopeCombo;
    QPlainTextEdit *_gameChatTranscript;
    QLineEdit *_gameChatInput;
    QPushButton *_sendGameChatBtn;

    QWidget *_voiceBox;
    QPushButton *_joinLeaveVoiceBtn;
    QPushButton *_holdToTalkBtn;
    QLabel *_voiceStatusLabel;
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
