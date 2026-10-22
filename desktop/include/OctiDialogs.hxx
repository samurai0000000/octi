/*
 * OctiDialogs.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_DIALOGS_HXX
#define OCTI_DESKTOP_OCTI_DIALOGS_HXX

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include "ClientConfigStore.hxx"
#include "OctiBotEngine.hxx"
#include "OctiDefs.hxx"
#include "OctiNetworkClient.hxx"
#include "OctiTheme.hxx"
#include "QtCompat.hxx"

namespace octi {

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
 *    - Displays hard-coded read-only server badge ("Server: trumpet.selfso.com:6874")
 *      with zero editable host/port inputs.
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
    ~GameOverDialog() override;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    QString winnerBannerText() const;
    QString terminationText() const;
    int moveCount() const;
    const std::string &octiTranscript() const;

    bool exportTranscriptToFile(const std::string &filePath) const;
    void triggerRematch();
    bool wasRematchRequested() const;

    void setOnRematchCallback(std::function<void()> cb);

private:
    ThemeId _theme;
    QLabel *_winnerLabel;
    QLabel *_terminationLabel;
    QLabel *_movesLabel;
    QPushButton *_exportBtn;
    QPushButton *_rematchBtn;
    int _moveCount;
    std::string _transcript;
    bool _rematchRequested;
    std::function<void()> _onRematchCallback;
};

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
