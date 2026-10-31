/*
 * OctiMainWindow.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <atomic>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <nlohmann/json.hpp>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QShortcut>
#include <QSysInfo>
#include <QVBoxLayout>
#include "OctiCore.hxx"
#include "OctiCrashReporter.hxx"
#include "OctiMainWindow.hxx"
#include "Version.hxx"
#include "selfupdate/UpdateInstaller.hxx"

namespace octi {

using Json = nlohmann::json;

/* ==================================================================== */
/* DesktopCliArgs                                                       */
/* ==================================================================== */

std::string DesktopCliArgs::versionBanner()
{
    return OctiCore::buildBanner("octi");
}

std::string DesktopCliArgs::usageBanner(const char *progName)
{
    const char *prog = (progName && progName[0] != '\0') ? progName : "octi";
    std::ostringstream oss;
    oss << "Usage: " << prog << " [options]\n"
        << "Canonical Server Endpoint: Server endpoint is permanently fixed and not configurable\n\n"
        << "Options:\n"
        << "  -h, --help           Show this usage help and exit\n"
        << "  -v, --version        Show version banner and exit\n"
        << "  --offline            Start directly in offline tabletop/bot mode\n"
        << "  --config <path>      Path to local client preferences JSON file\n";
    return oss.str();
}

bool DesktopCliArgs::parse(int argc,
                           const char * const argv[],
                           DesktopCliArgs &outArgs,
                           std::string &errMsg)
{
    outArgs = DesktopCliArgs();
    errMsg.clear();

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (!arg) {
            continue;
        }
        if (std::strcmp(arg, "--version") == 0 ||
            std::strcmp(arg, "-v") == 0) {
            outArgs.showVersion = true;
            continue;
        }
        if (std::strcmp(arg, "--help") == 0 ||
            std::strcmp(arg, "-h") == 0) {
            outArgs.showHelp = true;
            continue;
        }
        if (std::strcmp(arg, "--offline") == 0) {
            outArgs.startOffline = true;
            continue;
        }
        if (std::strcmp(arg, "--config") == 0) {
            if (i + 1 >= argc || !argv[i + 1] || argv[i + 1][0] == '\0') {
                errMsg = "--config requires a file path argument";
                return false;
            }
            outArgs.configPath = argv[++i];
            continue;
        }
        if (std::strncmp(arg, "--config=", 9) == 0) {
            outArgs.configPath = std::string(arg + 9);
            if (outArgs.configPath.empty()) {
                errMsg = "--config= requires a non-empty file path";
                return false;
            }
            continue;
        }

        /*
         * Strict Hard-Coded Endpoint Guard:
         * Reject --server, --host, --port, and -p unconditionally.
         */
        if (std::strcmp(arg, "--server") == 0 ||
            std::strncmp(arg, "--server=", 9) == 0 ||
            std::strcmp(arg, "--host") == 0 ||
            std::strncmp(arg, "--host=", 7) == 0 ||
            std::strcmp(arg, "--port") == 0 ||
            std::strncmp(arg, "--port=", 7) == 0 ||
            std::strcmp(arg, "-p") == 0) {
            errMsg = std::string("Rejected option '") + arg +
                     "': server endpoint is permanently fixed and not configurable";
            return false;
        }

        /* Allow standard Qt platform / style arguments */
        if (std::strcmp(arg, "-platform") == 0 ||
            std::strcmp(arg, "-style") == 0) {
            if (i + 1 < argc) {
                ++i;
            }
            continue;
        }

        errMsg = std::string("Unknown command-line option '") + arg +
                 "' (server endpoint is permanently fixed and not configurable)";
        return false;
    }
    return true;
}

/* ==================================================================== */
/* OctiMainWindow                                                       */
/* ==================================================================== */

OctiMainWindow::OctiMainWindow(
    std::unique_ptr<OctiNetworkClient> netClient,
    const std::string &configPath,
    bool startOffline,
    QWidget *parent)
    : QMainWindow(parent),
      _configStore(configPath.empty()
                       ? ClientConfigStore::defaultConfigFilePath()
                       : configPath),
      _netClient(netClient ? std::move(netClient)
                           : std::make_unique<OctiNetworkClient>()),
      _voiceEngine(std::make_unique<P2pVoiceEngine>(_netClient.get())),
      _soundSynth(),
      _board(GAME_MODE_2, false, false, GAME_BASES_3),
      _composer(&_board),
      _botEngine(BotDifficulty::TACTICAL),
      _dispatcher(this),
      _theme(ThemeId::OBSIDIAN_BRASS),
      _offlineMode(startOffline),
      _authenticated(false),
      _isLocalBotGame(true),
      _awaitingMatchStart(false),
      _currentTurnStartedEpochMs(0),
      _matchStartedEpochSec(0),
      _transcriptUploadedForCurrentMatch(false),
      _heldTranscriptPendingConsent(false),
      _inOnlineRoom(false),
      _onlineViewerSeat(0),
      _onlineAutoOriented(false),
      _vcrTotalPlies(0),
      _brandLabel(nullptr),
      _serverStatusLabel(nullptr),
      _profileBadgeLabel(nullptr),
      _signInBtn(nullptr),
      _accountLinkBtn(nullptr),
      _boardViewBtn(nullptr),
      _lobbyViewBtn(nullptr),
      _rotateBoardBtn(nullptr),
      _confirmAllCheck(nullptr),
      _themeToggleBtn(nullptr),
      _sfxMuteBtn(nullptr),
      _newLocalGameBtn(nullptr),
      _archiveBtn(nullptr),
      _leaderboardBtn(nullptr),
      _viewStack(nullptr),
      _boardPage(nullptr),
      _reservePanel(nullptr),
      _boardCanvas(nullptr),
      _lobbyView(nullptr),
      _matchModeLabel(nullptr),
      _matchTurnStatusLabel(nullptr),
      _sidebarNewGameBtn(nullptr),
      _moveHistoryList(nullptr),
      _vcrFirstBtn(nullptr),
      _vcrPrevBtn(nullptr),
      _vcrNextBtn(nullptr),
      _vcrLastBtn(nullptr),
      _exportNotationBtn(nullptr),
      _fenStatusLabel(nullptr),
      _proposalBannerWidget(nullptr),
      _proposalBannerLabel(nullptr),
      _acceptProposalBtn(nullptr),
      _declineProposalBtn(nullptr),
      _offerDrawBtn(nullptr),
      _pauseResumeBtn(nullptr),
      _undoRequestBtn(nullptr),
      _claimForfeitBtn(nullptr),
      _resignBtn(nullptr),
      _moderationBtn(nullptr),
      _chatBox(nullptr),
      _chatScopeCombo(nullptr),
      _gameChatTranscript(nullptr),
      _gameChatInput(nullptr),
      _sendGameChatBtn(nullptr),
      _voiceBox(nullptr),
      _joinLeaveVoiceBtn(nullptr),
      _holdToTalkBtn(nullptr),
      _voiceStatusLabel(nullptr),
      _gameOverDialogShownForCurrentMatch(false),
      _updateController(nullptr),
      _currentUpdateState(),
      _activeUpdateDialog(nullptr),
      _updateBannerDismissed(false),
      _quitRequestedByUpdate(false),
      _checkUpdateBtn(nullptr),
      _updateBannerWidget(nullptr),
      _updateBannerLabel(nullptr),
      _updateBannerProgressBar(nullptr),
      _updateActionBtn(nullptr),
      _updateDetailsBtn(nullptr),
      _updateDismissBtn(nullptr)
{
    selfupdate::UpdateInstaller::cleanupStaleBackup();
    _updateController = std::make_shared<selfupdate::SelfUpdateController>(
        defaultSelfUpdateConfig());
    bindSelfUpdateController();

    _voiceEngine->setHardwareAudioEnabled(false);

    std::string loadErr;
    if (_configStore.load(loadErr)) {
        const ClientPreferences &prefs = _configStore.preferences();
        _theme = prefs.theme;
        _soundSynth.setMuted(prefs.sfxMuted);
        if (!prefs.sessionToken.empty()) {
            _netClient->setSessionToken(prefs.sessionToken);
            _profile.uuid = prefs.playerUuid;
            _profile.handle = prefs.handle;
            _profile.displayName = prefs.displayName;
            _profile.avatarUrl = prefs.avatarUrl;
            _authenticated = !prefs.playerUuid.empty();
        }
    }
    _composer.setConfirmAllMoves(true);
    _configStore.preferences().confirmAllMoves = true;

    setWindowIcon(createOctiAppIcon());
    setWindowTitle(
        QString("OCTI Desktop Player v%1")
            .arg(QString::fromUtf8(OctiCore::versionString())));
    resize(1440, 900);

    QWidget *central = new QWidget(this);
    QVBoxLayout *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(10);
    setCentralWidget(central);

    /* ---- Top Navigation Bar (Decluttered Logo Header - GUI-04) ---- */
    QWidget *topBar = new QWidget(central);
    QHBoxLayout *topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(8, 4, 8, 4);
    topBarLayout->setSpacing(8);

    QLabel *crestLabel = new QLabel(topBar);
    crestLabel->setPixmap(QPixmap::fromImage(renderOctiIconImage(32)));
    crestLabel->setFixedSize(32, 32);
    crestLabel->setStyleSheet("border: none; background: transparent;");
    topBarLayout->addWidget(crestLabel);

    _brandLabel = new QLabel("OCTI", topBar);
    _brandLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    topBarLayout->addWidget(_brandLabel);

    topBarLayout->addStretch(1);
    rootLayout->addWidget(topBar);

    /* Legacy button/status pointers (hidden for header decluttering, but kept valid) */
    _serverStatusLabel = new QLabel(topBar);
    _serverStatusLabel->setObjectName("serverEndpointBadge");
    _serverStatusLabel->setVisible(false);

    _profileBadgeLabel = new QLabel("Guest (Offline Practice)", topBar);
    _profileBadgeLabel->setVisible(false);

    _signInBtn = new QPushButton("Sign In / SSO", topBar);
    _signInBtn->setVisible(false);
    _accountLinkBtn = new QPushButton("Account / Link SSO", topBar);
    _accountLinkBtn->setVisible(false);
    _boardViewBtn = new QPushButton("Board Table", topBar);
    _boardViewBtn->setVisible(false);
    _lobbyViewBtn = new QPushButton("Online Lobby", topBar);
    _lobbyViewBtn->setVisible(false);
    _newLocalGameBtn = new QPushButton("New Local / Bot Game", topBar);
    _newLocalGameBtn->setVisible(false);
    _rotateBoardBtn = new QPushButton("Rotate Board (0°)", topBar);
    _rotateBoardBtn->setVisible(false);
    _confirmAllCheck = new QCheckBox("Confirm All Moves", topBar);
    _confirmAllCheck->setChecked(true);
    _confirmAllCheck->setVisible(false);
    _themeToggleBtn = new QPushButton("Theme: Obsidian", topBar);
    _themeToggleBtn->setVisible(false);
    _sfxMuteBtn = new QPushButton("SFX: On", topBar);
    _sfxMuteBtn->setVisible(false);
    _archiveBtn = new QPushButton("Archive", topBar);
    _archiveBtn->setVisible(false);
    _leaderboardBtn = new QPushButton("Leaderboard", topBar);
    _leaderboardBtn->setVisible(false);
    _checkUpdateBtn = new QPushButton("Check for Updates", topBar);
    _checkUpdateBtn->setVisible(false);

    /* ---- Inline Self-Update Notification Banner ---- */
    _updateBannerWidget = new QWidget(central);
    QHBoxLayout *bannerLayout = new QHBoxLayout(_updateBannerWidget);
    bannerLayout->setContentsMargins(10, 4, 10, 4);
    bannerLayout->setSpacing(8);

    _updateBannerLabel = new QLabel(_updateBannerWidget);
    _updateBannerProgressBar = new QProgressBar(_updateBannerWidget);
    _updateBannerProgressBar->setFixedSize(140, 16);
    _updateBannerProgressBar->setVisible(false);

    _updateActionBtn = new QPushButton("Download && Update", _updateBannerWidget);
    _updateDetailsBtn = new QPushButton("Release Details...", _updateBannerWidget);
    _updateDismissBtn = new QPushButton("Dismiss", _updateBannerWidget);

    bannerLayout->addWidget(_updateBannerLabel);
    bannerLayout->addWidget(_updateBannerProgressBar);
    bannerLayout->addStretch(1);
    bannerLayout->addWidget(_updateActionBtn);
    bannerLayout->addWidget(_updateDetailsBtn);
    bannerLayout->addWidget(_updateDismissBtn);

    _updateBannerWidget->setVisible(false);
    rootLayout->addWidget(_updateBannerWidget);

    /* ---- Center View Stack (Board Page vs Lobby View) ---- */
    _viewStack = new QStackedWidget(central);

    _boardPage = new QWidget(_viewStack);
    QHBoxLayout *boardLayout = new QHBoxLayout(_boardPage);
    boardLayout->setContentsMargins(0, 0, 0, 0);
    boardLayout->setSpacing(12);

    _reservePanel = new OctiReservePanel(_boardPage);
    _reservePanel->setMinimumWidth(300);
    _reservePanel->setMaximumWidth(350);

    _boardCanvas = new OctiBoardCanvas(_boardPage);
    _boardCanvas->setMinimumSize(620, 620);

    _board.getGame()->start(PLAYER_A);
    _boardCanvas->attachBoard(&_board, &_composer, &_soundSynth);
    _reservePanel->attach(&_board, &_composer, _boardCanvas, &_soundSynth);

    _composer.setMoveCommittedCallback(
        [this](const std::string &moveNotation) {
            handleLocalOrOnlineMoveCommitted(moveNotation);
        });
    _reservePanel->setPushToTalkCallback([this](bool talking) {
        setPushToTalkActive(talking);
    });

    /* ---- Right Side Panel (Move History / VCR, Proposals, Chat, PTT Voice) ---- */
    QWidget *rightSide = new QWidget(_boardPage);
    rightSide->setMinimumWidth(330);
    rightSide->setMaximumWidth(390);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightSide);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    /* 0. Match Status Card (PCE-D06 / PCE-D09) */
    QGroupBox *statusBox = new QGroupBox("Match Status", rightSide);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusBox);
    _matchModeLabel = new QLabel("Local vs AI Bot", statusBox);
    _matchTurnStatusLabel = new QLabel("Active Turn: Player A", statusBox);
    _matchTurnStatusLabel->setStyleSheet("font-size: 13px; font-weight: bold;");
    _matchTurnStatusLabel->setWordWrap(true);
    statusLayout->addWidget(_matchModeLabel);
    statusLayout->addWidget(_matchTurnStatusLabel);
    rightLayout->addWidget(statusBox);

    /* 1. Move History & VCR Replay */
    QGroupBox *historyBox = new QGroupBox("Move History • VCR Replay", rightSide);
    QVBoxLayout *histLayout = new QVBoxLayout(historyBox);
    _moveHistoryList = new QListWidget(historyBox);
    _moveHistoryList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    histLayout->addWidget(_moveHistoryList, 1);

    QShortcut *copyHistoryShortcut = new QShortcut(QKeySequence::Copy, _moveHistoryList);
    QObject::connect(copyHistoryShortcut, &QShortcut::activated, this, [this]() {
        if (!_moveHistoryList) {
            return;
        }
        QStringList lines;
        const QList<QListWidgetItem *> selected = _moveHistoryList->selectedItems();
        if (!selected.isEmpty()) {
            for (const QListWidgetItem *item : selected) {
                if (item) {
                    lines.append(item->text());
                }
            }
        } else {
            for (int i = 0; i < _moveHistoryList->count(); ++i) {
                if (_moveHistoryList->item(i)) {
                    lines.append(_moveHistoryList->item(i)->text());
                }
            }
        }
        if (QClipboard *cb = QGuiApplication::clipboard()) {
            cb->setText(lines.join("\n"));
        }
    });

    QHBoxLayout *vcrRow = new QHBoxLayout();
    _vcrFirstBtn = new QPushButton("|< First", historyBox);
    _vcrPrevBtn = new QPushButton("< Prev", historyBox);
    _vcrNextBtn = new QPushButton("Next >", historyBox);
    _vcrLastBtn = new QPushButton("Last >|", historyBox);
    _exportNotationBtn = new QPushButton("Export...", historyBox);
    vcrRow->addWidget(_vcrFirstBtn);
    vcrRow->addWidget(_vcrPrevBtn);
    vcrRow->addWidget(_vcrNextBtn);
    vcrRow->addWidget(_vcrLastBtn);
    vcrRow->addWidget(_exportNotationBtn);
    histLayout->addLayout(vcrRow);

    _fenStatusLabel = new QLabel(historyBox);
    _fenStatusLabel->setWordWrap(true);
    _fenStatusLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    _fenStatusLabel->setVisible(false);
    rightLayout->addWidget(historyBox, 2);

    /* 2. Active Proposal Banner & Game Action Buttons */
    _proposalBannerWidget = new QGroupBox("Pending Proposal", rightSide);
    QVBoxLayout *propLayout = new QVBoxLayout(_proposalBannerWidget);
    _proposalBannerLabel = new QLabel("No active proposal.", _proposalBannerWidget);
    _proposalBannerLabel->setWordWrap(true);
    propLayout->addWidget(_proposalBannerLabel);
    QHBoxLayout *propRespRow = new QHBoxLayout();
    _acceptProposalBtn = new QPushButton("Accept", _proposalBannerWidget);
    _declineProposalBtn = new QPushButton("Decline", _proposalBannerWidget);
    propRespRow->addWidget(_acceptProposalBtn);
    propRespRow->addWidget(_declineProposalBtn);
    propLayout->addLayout(propRespRow);
    _proposalBannerWidget->setVisible(false);
    rightLayout->addWidget(_proposalBannerWidget);

    QGroupBox *actionsBox = new QGroupBox("Match Actions", rightSide);
    QVBoxLayout *actLayout = new QVBoxLayout(actionsBox);
    QHBoxLayout *actRow1 = new QHBoxLayout();
    _sidebarNewGameBtn = new QPushButton("New Game...", actionsBox);
    _offerDrawBtn = new QPushButton("Offer Draw", actionsBox);
    _pauseResumeBtn = new QPushButton("Pause", actionsBox);
    _undoRequestBtn = new QPushButton("Undo", actionsBox);
    actRow1->addWidget(_sidebarNewGameBtn);
    actRow1->addWidget(_offerDrawBtn);
    actRow1->addWidget(_pauseResumeBtn);
    actRow1->addWidget(_undoRequestBtn);
    actLayout->addLayout(actRow1);

    QHBoxLayout *actRow2 = new QHBoxLayout();
    _claimForfeitBtn = new QPushButton("Forfeit", actionsBox);
    _resignBtn = new QPushButton("Resign", actionsBox);
    _moderationBtn = new QPushButton("Moderation", actionsBox);
    actRow2->addWidget(_claimForfeitBtn);
    actRow2->addWidget(_resignBtn);
    actRow2->addWidget(_moderationBtn);
    actLayout->addLayout(actRow2);
    rightLayout->addWidget(actionsBox);

    /* 3. Board Chat Widget */
    QGroupBox *chatBox = new QGroupBox("Table Chat", rightSide);
    _chatBox = chatBox;
    QVBoxLayout *chatLayout = new QVBoxLayout(chatBox);
    _gameChatTranscript = new QPlainTextEdit(chatBox);
    _gameChatTranscript->setReadOnly(true);
    chatLayout->addWidget(_gameChatTranscript, 1);

    QHBoxLayout *chatInputRow = new QHBoxLayout();
    _chatScopeCombo = new QComboBox(chatBox);
    _chatScopeCombo->addItem("ALL", "ALL");
    _chatScopeCombo->addItem("TEAM", "TEAM");
    _gameChatInput = new QLineEdit(chatBox);
    _gameChatInput->setPlaceholderText("Send board message...");
    _sendGameChatBtn = new QPushButton("Send", chatBox);
    chatInputRow->addWidget(_chatScopeCombo);
    chatInputRow->addWidget(_gameChatInput, 1);
    chatInputRow->addWidget(_sendGameChatBtn);
    chatLayout->addLayout(chatInputRow);
    rightLayout->addWidget(chatBox, 2);

    /* 4. Push-to-Talk Voice Control Bar */
    QGroupBox *voiceBox = new QGroupBox("Push-to-Talk P2P Voice", rightSide);
    _voiceBox = voiceBox;
    QVBoxLayout *vLayout = new QVBoxLayout(voiceBox);
    _voiceStatusLabel = new QLabel("Voice: Disconnected (Strict PTT)", voiceBox);
    vLayout->addWidget(_voiceStatusLabel);
    QHBoxLayout *vBtnRow = new QHBoxLayout();
    _joinLeaveVoiceBtn = new QPushButton("Join Voice", voiceBox);
    _holdToTalkBtn = new QPushButton("Hold to Talk [V]", voiceBox);
    vBtnRow->addWidget(_joinLeaveVoiceBtn);
    vBtnRow->addWidget(_holdToTalkBtn);
    vLayout->addLayout(vBtnRow);
    rightLayout->addWidget(voiceBox);

    boardLayout->addWidget(_reservePanel);
    boardLayout->addWidget(_boardCanvas, 1);
    boardLayout->addWidget(rightSide);

    _lobbyView = new LobbyView(_netClient.get(), _theme, _viewStack);

    _viewStack->addWidget(_boardPage);
    _viewStack->addWidget(_lobbyView);
    rootLayout->addWidget(_viewStack, 1);

    /* ---- Connect Signals ---- */
    QObject::connect(_boardViewBtn, &QPushButton::clicked, [this]() {
        showBoardView();
    });
    QObject::connect(_lobbyViewBtn, &QPushButton::clicked, [this]() {
        showLobbyView();
    });
    QObject::connect(_rotateBoardBtn, &QPushButton::clicked, [this]() {
        rotateBoardClockwise();
    });
    QObject::connect(_confirmAllCheck, &QCheckBox::toggled, [this](bool checked) {
        setConfirmAllMoves(checked);
    });
    QObject::connect(_themeToggleBtn, &QPushButton::clicked, [this]() {
        toggleTheme();
    });
    QObject::connect(_sfxMuteBtn, &QPushButton::clicked, [this]() {
        setSfxMuted(!isSfxMuted());
    });
    QObject::connect(_signInBtn, &QPushButton::clicked, [this]() {
        SsoOnboardingDialog dlg(_netClient.get(), &_configStore, _theme, this);
        dlg.setOnSignedInCallback([this](const PlayerProfileInfo &prof) {
            applyAuthenticatedProfile(prof);
        });
        dlg.exec();
    });
    QObject::connect(_accountLinkBtn, &QPushButton::clicked, [this]() {
        AccountLinkDialog dlg(_netClient.get(), &_configStore, _theme, this);
        dlg.setOnAccountSwitchedCallback([this](const PlayerProfileInfo &prof) {
            applyAuthenticatedProfile(prof);
        });
        dlg.exec();
    });
    QObject::connect(_newLocalGameBtn, &QPushButton::clicked, [this]() {
        NewLocalGameDialog dlg(_theme, this);
        dlg.setConfig(_localConfig);
        if (dlg.exec() == QDialog::Accepted) {
            startNewLocalGame(dlg.config());
            beginAwaitingMatchStart();
            promptStartMatchIfVisible();
        }
    });
    QObject::connect(_sidebarNewGameBtn, &QPushButton::clicked, [this]() {
        NewLocalGameDialog dlg(_theme, this);
        dlg.setConfig(_localConfig);
        if (dlg.exec() == QDialog::Accepted) {
            startNewLocalGame(dlg.config());
            beginAwaitingMatchStart();
            promptStartMatchIfVisible();
        }
    });
    QObject::connect(_archiveBtn, &QPushButton::clicked, [this]() {
        ArchiveBrowserDialog dlg(_netClient.get(), _theme, this);
        dlg.setOnLoadReplayCallback(
            [this](const std::string & /*gameId*/,
                   const std::string &transcript,
                   const ArchivedGameSummary & /*summary*/) {
                loadOctiTranscriptIntoReplay(transcript);
                showBoardView();
            });
        dlg.refreshArchiveList();
        dlg.exec();
    });
    QObject::connect(_leaderboardBtn, &QPushButton::clicked, [this]() {
        LeaderboardDialog dlg(_netClient.get(), _theme, this);
        dlg.refreshLeaderboard("2P");
        dlg.exec();
    });

    QObject::connect(_vcrFirstBtn, &QPushButton::clicked, [this]() {
        vcrFirst();
    });
    QObject::connect(_vcrPrevBtn, &QPushButton::clicked, [this]() {
        vcrPrev();
    });
    QObject::connect(_vcrNextBtn, &QPushButton::clicked, [this]() {
        vcrNext();
    });
    QObject::connect(_vcrLastBtn, &QPushButton::clicked, [this]() {
        vcrLast();
    });
    QObject::connect(_exportNotationBtn, &QPushButton::clicked, [this]() {
        showExportNotationDialog();
    });

    QObject::connect(_acceptProposalBtn, &QPushButton::clicked, [this]() {
        acceptActiveProposal();
    });
    QObject::connect(_declineProposalBtn, &QPushButton::clicked, [this]() {
        declineActiveProposal();
    });
    QObject::connect(_offerDrawBtn, &QPushButton::clicked, [this]() {
        proposeDraw();
    });
    QObject::connect(_pauseResumeBtn, &QPushButton::clicked, [this]() {
        proposePauseOrResume();
    });
    QObject::connect(_undoRequestBtn, &QPushButton::clicked, [this]() {
        if (_inOnlineRoom && !_onlineRoomId.empty()) {
            proposeUndo();
        } else if (_board.getGame() && _board.getGame()->canUndo()) {
            _board.getGame()->undo();
            if (_isLocalBotGame && _board.getGame()->canUndo() &&
                _board.getGame()->getTurn() != _localConfig.humanSeat) {
                _board.getGame()->undo();
            }
            _composer.refreshLegalMoves();
            if (_boardCanvas) {
                _boardCanvas->clearLastMoveModifiedSquares();
                _boardCanvas->update();
            }
            if (_reservePanel) {
                _reservePanel->refresh();
            }
            refreshMoveHistoryAndFenUi();
        }
    });
    QObject::connect(_claimForfeitBtn, &QPushButton::clicked, [this]() {
        claimOpponentForfeit();
    });
    QObject::connect(_resignBtn, &QPushButton::clicked, [this]() {
        resignCurrentGame();
    });
    QObject::connect(_moderationBtn, &QPushButton::clicked, [this]() {
        if (!_onlineRoomId.empty()) {
            RoomModerationDialog dlg(
                _netClient.get(), _onlineRoomId, _theme, this);
            dlg.exec();
            refreshOnlineGameView();
        }
    });

    QObject::connect(_sendGameChatBtn, &QPushButton::clicked, [this]() {
        std::string txt = _gameChatInput->text().trimmed().toStdString();
        std::string scope =
            _chatScopeCombo->currentData().toString().toStdString();
        if (!txt.empty() && sendGameChatMessage(scope, txt)) {
            _gameChatInput->clear();
        }
    });
    QObject::connect(_gameChatInput, &QLineEdit::returnPressed, [this]() {
        std::string txt = _gameChatInput->text().trimmed().toStdString();
        std::string scope =
            _chatScopeCombo->currentData().toString().toStdString();
        if (!txt.empty() && sendGameChatMessage(scope, txt)) {
            _gameChatInput->clear();
        }
    });

    QObject::connect(_joinLeaveVoiceBtn, &QPushButton::clicked, [this]() {
        if (isInVoiceRoom()) {
            leaveOnlineVoice();
        } else {
            joinOnlineVoice();
        }
    });
    QObject::connect(_holdToTalkBtn, &QPushButton::pressed, [this]() {
        setPushToTalkActive(true);
    });
    QObject::connect(_holdToTalkBtn, &QPushButton::released, [this]() {
        setPushToTalkActive(false);
    });

    _lobbyView->setOnCreateTableRequested([this]() {
        CreateTableDialog dlg(_netClient.get(), _theme, this);
        if (dlg.exec() == QDialog::Accepted &&
            !dlg.createdTable().roomId.empty()) {
            _isLocalBotGame = false;
            _inOnlineRoom = true;
            _onlineRoomId = dlg.createdTable().roomId;
            _onlineViewerSeat = dlg.createdTable().seat;
            _onlineAutoOriented = false;
            _appliedOnlineMoves.clear();
            _netClient->startGameEventStream(
                _onlineRoomId, 0, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        handleGameSseEvent(ev);
                    });
                });
            refreshOnlineGameView();
            showBoardView();
        }
    });
    _lobbyView->setOnJoinTableRequested(
        [this](const TableSummaryInfo &table) {
            std::string code;
            if (table.hasPasscode) {
                PasscodeDialog pd(table.roomId, _theme, this);
                if (pd.exec() != QDialog::Accepted) {
                    return;
                }
                code = pd.passcode();
            }
            joinAndEnterOnlineTable(table.roomId, 0, code);
        });
    _lobbyView->setOnObserveTableRequested(
        [this](const TableSummaryInfo &table) {
            std::string code;
            if (table.hasPasscode) {
                PasscodeDialog pd(table.roomId, _theme, this);
                if (pd.exec() != QDialog::Accepted) {
                    return;
                }
                code = pd.passcode();
            }
            observeOnlineTable(table.roomId, code);
        });
    _lobbyView->setOnMatchmakingRequested([this]() {
        MatchmakingDialog dlg(_netClient.get(), _theme, this);
        dlg.setOnMatchedCallback([this](const std::string &roomId) {
            _isLocalBotGame = false;
            _inOnlineRoom = true;
            _onlineRoomId = roomId;
            _onlineAutoOriented = false;
            _appliedOnlineMoves.clear();
            _netClient->startGameEventStream(
                _onlineRoomId, 0, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        handleGameSseEvent(ev);
                    });
                });
            refreshOnlineGameView();
            showBoardView();
        });
        dlg.exec();
    });

    QObject::connect(_checkUpdateBtn, &QPushButton::clicked, [this]() {
        checkForAppUpdates(true);
        SelfUpdateDialog dlg(_updateController, _theme, this);
        dlg.exec();
    });
    QObject::connect(_updateActionBtn, &QPushButton::clicked, [this]() {
        triggerAppUpdateDownloadAndInstall();
    });
    QObject::connect(_updateDetailsBtn, &QPushButton::clicked, [this]() {
        SelfUpdateDialog dlg(_updateController, _theme, this);
        dlg.exec();
    });
    QObject::connect(_updateDismissBtn, &QPushButton::clicked, [this]() {
        dismissAppUpdateBanner();
    });

    installEventFilter(this);
    setTheme(_theme);
    updateTopBarLabels();
    refreshMoveHistoryAndFenUi();
    updateProposalBannerUi();
    updateVoiceControlUi();
    updateMatchActionsVisibility();
    setupMenuBar();
}

OctiMainWindow::~OctiMainWindow()
{
    if (_updateController) {
        _updateController->setStateCallback(nullptr);
        _updateController->setQuitCallback(nullptr);
        _updateController->setUiDispatcher(nullptr);
        _updateController->cancel();
        _updateController.reset();
    }
    leaveOnlineRoom();
    if (_uploadThread.joinable()) {
        _uploadThread.join();
    }
    if (_netClient) {
        _netClient->stopEventStreams();
    }
}

OctiBoard *OctiMainWindow::board()
{
    return &_board;
}

const OctiBoard *OctiMainWindow::board() const
{
    return &_board;
}

OctiBoardCanvas *OctiMainWindow::boardCanvas() const
{
    return _boardCanvas;
}

OctiReservePanel *OctiMainWindow::reservePanel() const
{
    return _reservePanel;
}

OctiMoveComposer *OctiMainWindow::moveComposer()
{
    return &_composer;
}

LobbyView *OctiMainWindow::lobbyView() const
{
    return _lobbyView;
}

OctiNetworkClient *OctiMainWindow::netClient() const
{
    return _netClient.get();
}

P2pVoiceEngine *OctiMainWindow::voiceEngine() const
{
    return _voiceEngine.get();
}

OctiSoundSynth *OctiMainWindow::soundSynth()
{
    return &_soundSynth;
}

ClientConfigStore *OctiMainWindow::configStore()
{
    return &_configStore;
}

QString OctiMainWindow::serverStatusText() const
{
    return _serverStatusLabel ? _serverStatusLabel->text() : QString();
}

bool OctiMainWindow::hasEditableServerOrPortField() const
{
    const QList<QLineEdit *> edits = findChildren<QLineEdit *>();
    for (int i = 0; i < edits.size(); ++i) {
        const QLineEdit *ed = edits[i];
        if (!ed) {
            continue;
        }
        const QString name = ed->objectName().toLower();
        const QString ph = ed->placeholderText().toLower();
        if (name.contains("server") || name.contains("host") ||
            name.contains("port") || ph.contains("server") ||
            ph.contains("port") || ph.contains("trumpet")) {
            return true;
        }
    }
    return false;
}

QString OctiMainWindow::profileBadgeText() const
{
    return _profileBadgeLabel ? _profileBadgeLabel->text() : QString();
}

bool OctiMainWindow::isAuthenticated() const
{
    return _authenticated;
}

const PlayerProfileInfo &OctiMainWindow::currentProfile() const
{
    return _profile;
}

void OctiMainWindow::applyAuthenticatedProfile(const PlayerProfileInfo &profile)
{
    _profile = profile;
    _authenticated = !profile.uuid.empty();
    _offlineMode = false;
    updateTopBarLabels();

    if (_authenticated && _netClient) {
        if (!_netClient->isLobbyEventStreamActive()) {
            _netClient->startLobbyEventStream(
                0, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        if (_lobbyView) {
                            _lobbyView->handleLobbySseEvent(ev);
                        }
                    });
                });
        }
        if (_lobbyView) {
            _lobbyView->refreshLobby();
        }
    }
}

void OctiMainWindow::showBoardView()
{
    if (_viewStack && _boardPage) {
        _viewStack->setCurrentWidget(_boardPage);
    }
}

void OctiMainWindow::showLobbyView()
{
    if (_viewStack && _lobbyView) {
        _viewStack->setCurrentWidget(_lobbyView);
        _lobbyView->refreshLobby();
    }
}

bool OctiMainWindow::isBoardViewVisible() const
{
    return _viewStack && _viewStack->currentWidget() == _boardPage;
}

bool OctiMainWindow::isLobbyViewVisible() const
{
    return _viewStack && _viewStack->currentWidget() == _lobbyView;
}

void OctiMainWindow::rotateBoardClockwise()
{
    if (!_boardCanvas) {
        return;
    }
    _boardCanvas->rotateClockwise();
    if (_rotateBoardBtn) {
        _rotateBoardBtn->setText(
            QString("Rotate Board (%1°)")
                .arg(_boardCanvas->geometry().rotationDegrees()));
    }
}

void OctiMainWindow::setBoardRotation(BoardRotation rot)
{
    if (!_boardCanvas) {
        return;
    }
    _boardCanvas->setBoardRotation(rot);
    if (_rotateBoardBtn) {
        _rotateBoardBtn->setText(
            QString("Rotate Board (%1°)")
                .arg(_boardCanvas->geometry().rotationDegrees()));
    }
}

BoardRotation OctiMainWindow::boardRotation() const
{
    return _boardCanvas ? _boardCanvas->boardRotation() : BoardRotation::ROT_0;
}

void OctiMainWindow::setConfirmAllMoves(bool confirmAll)
{
    _composer.setConfirmAllMoves(confirmAll);
    if (_confirmAllCheck && _confirmAllCheck->isChecked() != confirmAll) {
        _confirmAllCheck->blockSignals(true);
        _confirmAllCheck->setChecked(confirmAll);
        _confirmAllCheck->blockSignals(false);
    }
    _configStore.preferences().confirmAllMoves = confirmAll;
    std::string ignoredErr;
    _configStore.save(ignoredErr);
}

bool OctiMainWindow::confirmAllMoves() const
{
    return _composer.confirmAllMoves();
}

void OctiMainWindow::toggleTheme()
{
    setTheme(_theme == ThemeId::OBSIDIAN_BRASS
                 ? ThemeId::ROSEWOOD_IVORY
                 : ThemeId::OBSIDIAN_BRASS);
}

void OctiMainWindow::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
    if (_boardCanvas) {
        _boardCanvas->setTheme(_theme);
    }
    if (_reservePanel) {
        _reservePanel->setTheme(_theme);
    }
    if (_lobbyView) {
        _lobbyView->setTheme(_theme);
    }
    if (_themeToggleBtn) {
        _themeToggleBtn->setText(
            _theme == ThemeId::OBSIDIAN_BRASS ? "Theme: Obsidian"
                                              : "Theme: Rosewood");
    }
    _configStore.preferences().theme = _theme;
    std::string ignoredErr;
    _configStore.save(ignoredErr);
    refreshMoveHistoryAndFenUi();
    setupMenuBar();
}

ThemeId OctiMainWindow::theme() const
{
    return _theme;
}

void OctiMainWindow::showSplashAboutDialog()
{
    OctiSplashDialog splash(this, _theme, true);
    splash.exec();
}

int OctiMainWindow::menuActionCountWithIconsAndShortcuts() const
{
    int count = 0;
    if (menuBar()) {
        for (QAction *menuAct : menuBar()->actions()) {
            if (menuAct->menu()) {
                for (QAction *act : menuAct->menu()->actions()) {
                    if (!act->isSeparator() && !act->icon().isNull() && !act->shortcut().isEmpty()) {
                        ++count;
                    }
                }
            }
        }
    }
    return count;
}

void OctiMainWindow::setupMenuBar()
{
    QMenuBar *mb = menuBar();
    if (!mb) {
        return;
    }
    mb->clear();

    // 1. &Game Menu
    QMenu *gameMenu = mb->addMenu("&Game");

    QAction *actNewGame = gameMenu->addAction(createMenuIcon(MenuIconKind::NEW_GAME, _theme), "New Local / Bot Game...");
    actNewGame->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    QObject::connect(actNewGame, &QAction::triggered, this, [this]() {
        NewLocalGameDialog dlg(_theme, this);
        dlg.setConfig(_localConfig);
        if (dlg.exec() == QDialog::Accepted) {
            startNewLocalGame(dlg.config());
            beginAwaitingMatchStart();
            promptStartMatchIfVisible();
        }
    });

    QAction *actBoardView = gameMenu->addAction(createMenuIcon(MenuIconKind::BOARD_VIEW, _theme), "Tabletop Board View");
    actBoardView->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    QObject::connect(actBoardView, &QAction::triggered, this, [this]() {
        showBoardView();
    });

    QAction *actLobbyView = gameMenu->addAction(createMenuIcon(MenuIconKind::LOBBY_VIEW, _theme), "Online Lobby & Matchmaking");
    actLobbyView->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    QObject::connect(actLobbyView, &QAction::triggered, this, [this]() {
        showLobbyView();
    });

    gameMenu->addSeparator();

    QAction *actOfferDraw = gameMenu->addAction(createMenuIcon(MenuIconKind::OFFER_DRAW, _theme), "Offer Draw");
    actOfferDraw->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    QObject::connect(actOfferDraw, &QAction::triggered, this, [this]() {
        proposeDraw();
    });

    QAction *actPauseResume = gameMenu->addAction(createMenuIcon(MenuIconKind::PAUSE_RESUME, _theme), "Pause / Resume Game");
    actPauseResume->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    QObject::connect(actPauseResume, &QAction::triggered, this, [this]() {
        proposePauseOrResume();
    });

    QAction *actUndo = gameMenu->addAction(createMenuIcon(MenuIconKind::UNDO_MOVE, _theme), "Undo Move");
    actUndo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    QObject::connect(actUndo, &QAction::triggered, this, [this]() {
        if (_inOnlineRoom && !_onlineRoomId.empty()) {
            proposeUndo();
        } else if (_board.getGame() && _board.getGame()->canUndo()) {
            _board.getGame()->undo();
            if (_isLocalBotGame && _board.getGame()->canUndo() &&
                _board.getGame()->getTurn() != _localConfig.humanSeat) {
                _board.getGame()->undo();
            }
            refreshMoveHistoryAndFenUi();
            if (_boardCanvas) {
                _boardCanvas->clearLastMoveModifiedSquares();
                _boardCanvas->update();
            }
        }
    });

    QAction *actClaimForfeit = gameMenu->addAction(createMenuIcon(MenuIconKind::CLAIM_FORFEIT, _theme), "Claim Opponent Forfeit");
    actClaimForfeit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    QObject::connect(actClaimForfeit, &QAction::triggered, this, [this]() {
        claimOpponentForfeit();
    });

    QAction *actResign = gameMenu->addAction(createMenuIcon(MenuIconKind::RESIGN, _theme), "Resign Game");
    actResign->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    QObject::connect(actResign, &QAction::triggered, this, [this]() {
        resignCurrentGame();
    });

    gameMenu->addSeparator();

    QAction *actQuit = gameMenu->addAction(createMenuIcon(MenuIconKind::QUIT, _theme), "Quit OCTI");
    actQuit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    QObject::connect(actQuit, &QAction::triggered, this, [this]() {
        close();
    });

    // 2. &View Menu
    QMenu *viewMenu = mb->addMenu("&View");

    QAction *actRotate = viewMenu->addAction(createMenuIcon(MenuIconKind::ROTATE_BOARD, _theme), "Rotate Board 90° Clockwise");
    actRotate->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    QObject::connect(actRotate, &QAction::triggered, this, [this]() {
        rotateBoardClockwise();
    });

    QAction *actTheme = viewMenu->addAction(createMenuIcon(MenuIconKind::SWITCH_THEME, _theme), "Switch Tabletop Theme");
    actTheme->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    QObject::connect(actTheme, &QAction::triggered, this, [this]() {
        toggleTheme();
    });

    QAction *actSfx = viewMenu->addAction(createMenuIcon(MenuIconKind::TOGGLE_SFX, _theme), "Toggle Sound Effects (SFX)");
    actSfx->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    QObject::connect(actSfx, &QAction::triggered, this, [this]() {
        setSfxMuted(!isSfxMuted());
    });

    viewMenu->addSeparator();

    QAction *actVcrFirst = viewMenu->addAction(createMenuIcon(MenuIconKind::VCR_FIRST, _theme), "First Ply (|<)");
    actVcrFirst->setShortcut(QKeySequence(Qt::Key_Home));
    QObject::connect(actVcrFirst, &QAction::triggered, this, [this]() {
        vcrFirst();
    });

    QAction *actVcrPrev = viewMenu->addAction(createMenuIcon(MenuIconKind::VCR_PREV, _theme), "Previous Ply (<)");
    actVcrPrev->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left));
    QObject::connect(actVcrPrev, &QAction::triggered, this, [this]() {
        vcrPrev();
    });

    QAction *actVcrNext = viewMenu->addAction(createMenuIcon(MenuIconKind::VCR_NEXT, _theme), "Next Ply (>)");
    actVcrNext->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right));
    QObject::connect(actVcrNext, &QAction::triggered, this, [this]() {
        vcrNext();
    });

    QAction *actVcrLast = viewMenu->addAction(createMenuIcon(MenuIconKind::VCR_LAST, _theme), "Latest Ply (>|)");
    actVcrLast->setShortcut(QKeySequence(Qt::Key_End));
    QObject::connect(actVcrLast, &QAction::triggered, this, [this]() {
        vcrLast();
    });

    // 3. &Online Menu
    QMenu *onlineMenu = mb->addMenu("&Online");

    QAction *actSignIn = onlineMenu->addAction(createMenuIcon(MenuIconKind::SIGN_IN, _theme), "Sign In / SSO Onboarding...");
    actSignIn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    QObject::connect(actSignIn, &QAction::triggered, this, [this]() {
        SsoOnboardingDialog dlg(_netClient.get(), &_configStore, _theme, this);
        if (dlg.exec() == QDialog::Accepted) {
            _authenticated = true;
            updateTopBarLabels();
        }
    });

    QAction *actAccount = onlineMenu->addAction(createMenuIcon(MenuIconKind::ACCOUNT_LINK, _theme), "Account & Linked Identities...");
    actAccount->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    QObject::connect(actAccount, &QAction::triggered, this, [this]() {
        AccountLinkDialog dlg(_netClient.get(), &_configStore, _theme, this);
        dlg.setOnAccountSwitchedCallback([this](const PlayerProfileInfo &prof) {
            applyAuthenticatedProfile(prof);
        });
        dlg.exec();
    });

    QAction *actArchive = onlineMenu->addAction(createMenuIcon(MenuIconKind::GAME_ARCHIVE, _theme), "Game Archive & Replay Browser...");
    actArchive->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    QObject::connect(actArchive, &QAction::triggered, this, [this]() {
        ArchiveBrowserDialog dlg(_netClient.get(), _theme, this);
        dlg.setOnLoadReplayCallback(
            [this](const std::string &, const std::string &transcript, const ArchivedGameSummary &) {
                loadOctiTranscriptIntoReplay(transcript);
                showBoardView();
            });
        dlg.refreshArchiveList();
        dlg.exec();
    });

    QAction *actLeaderboard = onlineMenu->addAction(createMenuIcon(MenuIconKind::LEADERBOARD, _theme), "Elo Leaderboard...");
    actLeaderboard->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    QObject::connect(actLeaderboard, &QAction::triggered, this, [this]() {
        LeaderboardDialog dlg(_netClient.get(), _theme, this);
        dlg.refreshLeaderboard("2P");
        dlg.exec();
    });

    QAction *actModeration = onlineMenu->addAction(createMenuIcon(MenuIconKind::MODERATION, _theme), "Table Moderation & Policies...");
    actModeration->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    QObject::connect(actModeration, &QAction::triggered, this, [this]() {
        RoomModerationDialog dlg(_netClient.get(), _onlineRoomId, _theme, this);
        dlg.exec();
    });

    // 4. &Help Menu
    QMenu *helpMenu = mb->addMenu("&Help");

    QAction *actUpdate = helpMenu->addAction(createMenuIcon(MenuIconKind::CHECK_UPDATE, _theme), "Check for Updates...");
    actUpdate->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    QObject::connect(actUpdate, &QAction::triggered, this, [this]() {
        checkForAppUpdates(true);
        SelfUpdateDialog dlg(_updateController, _theme, this);
        dlg.exec();
    });

    QAction *actSplash = helpMenu->addAction(createMenuIcon(MenuIconKind::SHOW_SPLASH, _theme), "Show Splash / About OCTI");
    actSplash->setShortcut(QKeySequence(Qt::Key_F1));
    QObject::connect(actSplash, &QAction::triggered, this, [this]() {
        showSplashAboutDialog();
    });

    QAction *actTelemetry = helpMenu->addAction(createMenuIcon(MenuIconKind::MODERATION, _theme), "Telemetry & Privacy Preferences...");
    QObject::connect(actTelemetry, &QAction::triggered, this, [this]() {
        showTelemetrySettingsDialog();
    });
}

void OctiMainWindow::setSfxMuted(bool muted)
{
    _soundSynth.setMuted(muted);
    if (_sfxMuteBtn) {
        _sfxMuteBtn->setText(muted ? "SFX: Muted" : "SFX: On");
    }
    _configStore.preferences().sfxMuted = muted;
    std::string ignoredErr;
    _configStore.save(ignoredErr);
}

bool OctiMainWindow::isSfxMuted() const
{
    return _soundSynth.isMuted();
}

/* ---- Offline Local / Bot Play ---- */

void OctiMainWindow::startNewLocalGame(const LocalGameConfig &cfg)
{
    leaveOnlineRoom();
    _gameOverDialogShownForCurrentMatch = false;
    _transcriptUploadedForCurrentMatch = false;
    _heldTranscriptPendingConsent = false;
    _matchStartedEpochSec = _awaitingMatchStart ? 0 : QDateTime::currentSecsSinceEpoch();
    _currentTurnStartedEpochMs = _awaitingMatchStart ? 0 : QDateTime::currentMSecsSinceEpoch();
    _localConfig = cfg;
    _isLocalBotGame = (cfg.playType == LocalPlayType::VS_AI_BOT);
    _botEngine.setDifficulty(cfg.botDifficulty);
    if (!isVisible()) {
        _botEngine.setTimeBudgetMs(25);
    }

    _board.setGameMode(
        cfg.mode, cfg.useSuperProngs, cfg.useEdgelessBoard, cfg.bases);
    _board.getGame()->reset();
    _board.getGame()->start(PLAYER_A);

    if (cfg.timeControlEnabled && cfg.initialTimeMs > 0) {
        for (int p = 1; p <= 4; ++p) {
            _board.getGame()->setTimeRemaining(
                static_cast<OctiPlayer>(p), cfg.initialTimeMs);
        }
    }

    _composer.attachBoard(&_board);
    if (_boardCanvas) {
        _boardCanvas->clearLastMoveModifiedSquares();
        _boardCanvas->attachBoard(&_board, &_composer, &_soundSynth);
        _boardCanvas->autoOrientForSeat(cfg.humanSeat);
        if (_rotateBoardBtn) {
            _rotateBoardBtn->setText(
                QString("Rotate Board (%1°)")
                    .arg(_boardCanvas->geometry().rotationDegrees()));
        }
    }
    if (_reservePanel) {
        _reservePanel->attach(&_board, &_composer, _boardCanvas, &_soundSynth);
        _reservePanel->refresh();
    }

    showBoardView();
    refreshMoveHistoryAndFenUi();
    updateProposalBannerUi();
    updateMatchActionsVisibility();

    if (_isLocalBotGame && !_awaitingMatchStart) {
        executeBotTurnIfNeeded();
    }
}

void OctiMainWindow::beginAwaitingMatchStart()
{
    _awaitingMatchStart = true;
    _matchStartedEpochSec = 0;
    _currentTurnStartedEpochMs = 0;
    if (_boardCanvas) {
        _boardCanvas->setTurnIndicatorVisible(false);
    }
    if (_isLocalBotGame && _localConfig.humanSeat == PLAYER_B &&
        _board.getGame() && _board.getGame()->getNumMoves() == 1) {
        _board.getGame()->reset();
        _board.getGame()->start(PLAYER_A);
        if (_boardCanvas) {
            _boardCanvas->clearLastMoveModifiedSquares();
        }
        refreshMoveHistoryAndFenUi();
    }
    updateTopBarLabels();
}

void OctiMainWindow::confirmMatchStart()
{
    if (!_awaitingMatchStart) {
        return;
    }
    _awaitingMatchStart = false;
    _matchStartedEpochSec = QDateTime::currentSecsSinceEpoch();
    _currentTurnStartedEpochMs = QDateTime::currentMSecsSinceEpoch();
    if (_boardCanvas) {
        _boardCanvas->setTurnIndicatorVisible(true);
    }
    _soundSynth.triggerCue(SoundCue::POD_PLACE);
    updateTopBarLabels();
    if (_isLocalBotGame) {
        executeBotTurnIfNeeded();
    }
}

void OctiMainWindow::promptStartMatchIfVisible()
{
    if (!isVisible() || !_awaitingMatchStart) {
        return;
    }
    OctiStartMatchDialog dlg(_localConfig.humanSeat, _isLocalBotGame, _theme, this);
    dlg.positionCenterOfBoard(this, _boardCanvas);
    dlg.exec();
    confirmMatchStart();
}

bool OctiMainWindow::isLocalBotGame() const
{
    return _isLocalBotGame;
}

const LocalGameConfig &OctiMainWindow::localGameConfig() const
{
    return _localConfig;
}

bool OctiMainWindow::executeBotTurnIfNeeded()
{
    if (_awaitingMatchStart) {
        return false;
    }
    if (!_isLocalBotGame || _inOnlineRoom) {
        return false;
    }
    OctiGame *game = _board.getGame();
    if (!game || !game->isGameStarted() || game->isGameOver()) {
        return false;
    }

    if (!isVisible()) {
        _botEngine.setTimeBudgetMs(25);
    }

    int safetyCount = 0;
    bool movedAny = false;
    std::string lastBotMoveStr;
    BotSearchStats botStats;
    while (!game->isGameOver() &&
           game->getTurn() != _localConfig.humanSeat &&
           safetyCount < 8) {
        ++safetyCount;

        std::string botMove;
        QElapsedTimer timer;
        timer.start();
        int budgetMs = _botEngine.timeBudgetMs();
        if (!isVisible()) {
            botMove = _botEngine.chooseMove(_board, &botStats);
        } else {
            OctiThinkingDialog thinkingDlg(_botEngine.difficulty(), budgetMs, _theme, this);
            thinkingDlg.positionBelowBoard(this, _boardCanvas);

            std::atomic<bool> done{false};
            std::string chosen;
            std::thread worker([this, &done, &chosen, &botStats]() {
                try {
                    chosen = _botEngine.chooseMove(_board, &botStats);
                } catch (const std::exception &ex) {
                    octi::desktop::OctiCrashReporter::recordHandledException(
                        "OctiBotWorkerThread", typeid(ex).name(), ex.what(),
                        octi::desktop::OctiCrashReporter::getGameContextJson(),
                        _configStore.preferences().installId);
                    chosen.clear();
                } catch (...) {
                    octi::desktop::OctiCrashReporter::recordHandledException(
                        "OctiBotWorkerThread", "unknown_exception",
                        "Unknown exception in bot worker thread",
                        octi::desktop::OctiCrashReporter::getGameContextJson(),
                        _configStore.preferences().installId);
                    chosen.clear();
                }
                done.store(true);
            });

            QTimer tickTimer;
            QObject::connect(&tickTimer, &QTimer::timeout, &thinkingDlg, [&]() {
                int elapsed = static_cast<int>(timer.elapsed());
                int depth = _botEngine.liveDepthReached();
                std::uint64_t nodes = _botEngine.liveNodesVisited();
                int score = _botEngine.liveBestScore();
                thinkingDlg.updateProgress(elapsed, budgetMs, depth, nodes, score);
                if (done.load()) {
                    thinkingDlg.accept();
                }
            });
            tickTimer.start(50);

            thinkingDlg.exec();
            tickTimer.stop();
            if (worker.joinable()) {
                worker.join();
            }
            botMove = chosen;
        }

        if (botMove.empty()) {
            break;
        }
        try {
            game->move(botMove);
            lastBotMoveStr = botMove;
            movedAny = true;
            _soundSynth.triggerCue(SoundCue::POD_STEP);

            OctiPlyMetadata plyMeta;
            plyMeta.isAiMove = true;
            plyMeta.aiDifficulty = static_cast<int>(_botEngine.difficulty());
            plyMeta.aiBudgetMs = budgetMs;
            plyMeta.aiDepthReached = botStats.depthReached;
            plyMeta.aiNodesVisited = botStats.nodesVisited;
            plyMeta.aiBestScore = botStats.bestScore;
            plyMeta.elapsedMs = botStats.elapsedMs > 0 ? botStats.elapsedMs : static_cast<int>(timer.elapsed());
            game->setLastPlyMetadata(plyMeta);
            _currentTurnStartedEpochMs = QDateTime::currentMSecsSinceEpoch();
        } catch (...) {
            break;
        }
    }
    if (movedAny) {
        _composer.refreshLegalMoves();
        if (_boardCanvas) {
            if (!lastBotMoveStr.empty()) {
                try {
                    OctiMove parsedBotMove(lastBotMoveStr);
                    _boardCanvas->setLastMoveModifiedSquares(
                        OctiBoardCanvas::extractModifiedSquares(
                            parsedBotMove, game->useEdgelessBoard()));
                } catch (...) {
                }
            }
            _boardCanvas->update();
        }
        if (_reservePanel) {
            _reservePanel->refresh();
        }
        refreshMoveHistoryAndFenUi();
        checkAndPresentGameOver();
    }
    return movedAny;
}

/* ---- Online Multiplayer Room Lifecycle ---- */

ApiResult<CreateTableResponse> OctiMainWindow::createAndEnterOnlineTable(
    const CreateTableRequest &req)
{
    ApiResult<CreateTableResponse> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    CreateTableRequest normalized = req;
    normalized.observerPolicy =
        CreateTableDialog::normalizeObserverPolicy(req.observerPolicy);
    normalized.voicePolicy =
        CreateTableDialog::normalizeVoicePolicy(req.voicePolicy);
    res = _netClient->createTable(normalized);
    if (!res.ok) {
        if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
            showProtocolUpgradeRequired(res.errorMessage);
        }
        return res;
    }

    leaveOnlineRoom();
    _isLocalBotGame = false;
    _inOnlineRoom = true;
    _onlineRoomId = res.data.roomId;
    _onlineViewerSeat = res.data.seat;
    _onlineAutoOriented = false;
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    if (_gameChatTranscript) {
        _gameChatTranscript->clear();
    }

    _netClient->startGameEventStream(
        _onlineRoomId, 0, [this](const SseEvent &ev) {
            _dispatcher.post([this, ev]() {
                handleGameSseEvent(ev);
            });
        });

    refreshOnlineGameView();
    showBoardView();
    return res;
}

ApiResult<JoinTableResponse> OctiMainWindow::joinAndEnterOnlineTable(
    const std::string &roomId,
    int seat,
    const std::string &passcode)
{
    ApiResult<JoinTableResponse> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->joinTable(roomId, seat, passcode);
    if (!res.ok) {
        if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
            showProtocolUpgradeRequired(res.errorMessage);
        }
        return res;
    }

    leaveOnlineRoom();
    _isLocalBotGame = false;
    _inOnlineRoom = true;
    _onlineRoomId = res.data.roomId;
    _onlineViewerSeat = res.data.seat;
    _onlineAutoOriented = false;
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    if (_gameChatTranscript) {
        _gameChatTranscript->clear();
    }

    _netClient->startGameEventStream(
        _onlineRoomId, 0, [this](const SseEvent &ev) {
            _dispatcher.post([this, ev]() {
                handleGameSseEvent(ev);
            });
        });

    refreshOnlineGameView();
    showBoardView();
    return res;
}

ApiResult<bool> OctiMainWindow::observeOnlineTable(
    const std::string &roomId,
    const std::string &passcode)
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->observeTable(roomId, passcode);
    if (!res.ok) {
        if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
            showProtocolUpgradeRequired(res.errorMessage);
        }
        return res;
    }

    leaveOnlineRoom();
    _isLocalBotGame = false;
    _inOnlineRoom = true;
    _onlineRoomId = roomId;
    _onlineViewerSeat = 0;
    _onlineAutoOriented = false;
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    if (_gameChatTranscript) {
        _gameChatTranscript->clear();
    }

    _netClient->startGameEventStream(
        _onlineRoomId, 0, [this](const SseEvent &ev) {
            _dispatcher.post([this, ev]() {
                handleGameSseEvent(ev);
            });
        });

    refreshOnlineGameView();
    showBoardView();
    return res;
}

void OctiMainWindow::leaveOnlineRoom()
{
    if (_voiceEngine && _voiceEngine->isInVoiceRoom()) {
        _voiceEngine->leaveVoiceRoom();
    }
    if (_netClient) {
        _netClient->stopGameEventStream();
    }
    _inOnlineRoom = false;
    _onlineRoomId.clear();
    _onlineViewerSeat = 0;
    _onlineAutoOriented = false;
    _onlineGameView = GameViewData();
    _appliedOnlineMoves.clear();
    updateProposalBannerUi();
    updateVoiceControlUi();
    updateMatchActionsVisibility();
    setupMenuBar();
}

bool OctiMainWindow::isInOnlineGame() const
{
    return _inOnlineRoom && !_onlineRoomId.empty();
}

const std::string &OctiMainWindow::onlineRoomId() const
{
    return _onlineRoomId;
}

int OctiMainWindow::onlineViewerSeat() const
{
    return _onlineViewerSeat;
}

const GameViewData &OctiMainWindow::currentOnlineGameView() const
{
    return _onlineGameView;
}

bool OctiMainWindow::refreshOnlineGameView()
{
    if (!_netClient || _onlineRoomId.empty()) {
        return false;
    }
    auto res = _netClient->fetchGameView(_onlineRoomId);
    if (!res.ok) {
        if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
            showProtocolUpgradeRequired(res.errorMessage);
        }
        return false;
    }
    applyGameViewData(res.data);
    return true;
}

void OctiMainWindow::applyGameViewData(const GameViewData &view)
{
    _onlineGameView = view;
    if (view.viewerSeat > 0) {
        _onlineViewerSeat = view.viewerSeat;
    }

    OctiGameMode targetMode = GAME_MODE_2;
    if (view.mode == "4P") {
        targetMode = GAME_MODE_4;
    } else if (view.mode == "OFK") {
        targetMode = GAME_MODE_OFK;
    }
    OctiGameBases targetBases =
        (view.bases == 1) ? GAME_BASES_1 : GAME_BASES_3;

    if (!_onlineAutoOriented && _onlineViewerSeat >= 1 &&
        _onlineViewerSeat <= 4 && _boardCanvas) {
        _board.setGameMode(
            targetMode, view.superProngs, view.edgeless, targetBases);
        _boardCanvas->autoOrientForSeat(
            static_cast<OctiPlayer>(_onlineViewerSeat));
        if (_rotateBoardBtn) {
            _rotateBoardBtn->setText(
                QString("Rotate Board (%1°)")
                    .arg(_boardCanvas->geometry().rotationDegrees()));
        }
        _onlineAutoOriented = true;
    }

    OctiGame *g = _board.getGame();
    bool needFullRebuild =
        !g || _board.getGameMode() != targetMode ||
        g->getBases() != targetBases ||
        g->useSuperProngs() != view.superProngs ||
        g->useEdgelessBoard() != view.edgeless ||
        view.moves.size() < _appliedOnlineMoves.size();

    if (!needFullRebuild) {
        for (std::size_t i = 0; i < _appliedOnlineMoves.size(); ++i) {
            if (_appliedOnlineMoves[i] != view.moves[i]) {
                needFullRebuild = true;
                break;
            }
        }
    }

    if (needFullRebuild) {
        _board.setGameMode(
            targetMode, view.superProngs, view.edgeless, targetBases);
        _board.getGame()->reset();
        _board.getGame()->start(PLAYER_A);
        _appliedOnlineMoves.clear();
    } else if (!_board.getGame()->isGameStarted()) {
        _board.getGame()->start(PLAYER_A);
    }

    std::string latestOnlineMoveStr;
    for (std::size_t i = _appliedOnlineMoves.size();
         i < view.moves.size(); ++i) {
        try {
            _board.getGame()->move(view.moves[i]);
            _appliedOnlineMoves.push_back(view.moves[i]);
            latestOnlineMoveStr = view.moves[i];
        } catch (...) {
            if (!view.fen.empty()) {
                try {
                    _board.fromPositionString(view.fen);
                } catch (...) {
                }
            }
            break;
        }
    }

    for (std::size_t i = 0; i < view.seats.size(); ++i) {
        const GameSeatInfo &s = view.seats[i];
        if (s.seat >= 1 && s.seat <= 4 && s.clockMs >= 0) {
            _board.getGame()->setTimeRemaining(
                static_cast<OctiPlayer>(s.seat),
                static_cast<long>(s.clockMs));
        }
    }

    _composer.attachBoard(&_board);
    if (_boardCanvas) {
        _boardCanvas->attachBoard(&_board, &_composer, &_soundSynth);
        if (!latestOnlineMoveStr.empty()) {
            try {
                OctiMove parsedOnlineMove(latestOnlineMoveStr);
                _boardCanvas->setLastMoveModifiedSquares(
                    OctiBoardCanvas::extractModifiedSquares(
                        parsedOnlineMove, view.edgeless));
            } catch (...) {
            }
        }
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->attach(&_board, &_composer, _boardCanvas, &_soundSynth);
        _reservePanel->refresh();
    }

    refreshMoveHistoryAndFenUi();
    updateProposalBannerUi();
    updateVoiceControlUi();
    updateMatchActionsVisibility();
    checkAndPresentGameOver();
    setupMenuBar();
}

void OctiMainWindow::handleGameSseEvent(const SseEvent &ev)
{
    if (_voiceEngine) {
        _voiceEngine->handleGameSseEvent(ev);
    }

    Json j = Json::parse(ev.dataJson, nullptr, false);
    if (ev.event == "CHAT" && j.is_object()) {
        std::string scope = j.value("scope", "ALL");
        std::string handle = j.value("handle", "Player");
        std::string text = j.value("text", "");
        std::string line = "[" + scope + "] " + handle + ": " + text;
        _gameChatLines.push_back(line);
        if (_gameChatTranscript) {
            _gameChatTranscript->appendPlainText(
                QString::fromStdString(line));
        }
        return;
    }

    if (ev.event == "USER_MUTED") {
        if (_reservePanel && _voiceEngine) {
            _reservePanel->setVoiceMutedByRoom(
                _voiceEngine->isVoiceMutedByServer());
        }
        updateVoiceControlUi();
        return;
    }

    if (ev.event == "REMATCH_CREATED" && j.is_object()) {
        std::string newRoomId = j.value("room_id", "");
        if (!newRoomId.empty()) {
            _onlineGameView.rematchRoomId = newRoomId;
        }
    }

    std::string type = j.is_object() ? j.value("type", "") : "";
    if (ev.event == "MOVE" || ev.event == "GAME_STARTED" ||
        ev.event == "PROPOSAL_OFFERED" || ev.event == "PROPOSAL_ACCEPTED" ||
        ev.event == "PROPOSAL_DECLINED" || ev.event == "PROPOSAL_EXPIRED" ||
        ev.event == "GAME_PAUSED" || ev.event == "GAME_RESUMED" ||
        ev.event == "UNDO_APPLIED" || ev.event == "REMATCH_CREATED" ||
        ev.event == "GAME_OVER" || ev.event == "POLICY_UPDATED" ||
        ev.event == "SEAT_JOINED" || ev.event == "SEAT_LEFT" ||
        ev.event == "OBSERVER_BANNED" || ev.event == "VOICE_META" ||
        ev.event == "reconnect" || ev.event == "gap" ||
        ev.event == "PLAYER_DISCONNECTED" || ev.event == "PLAYER_RECONNECTED" ||
        type == "PLAYER_DISCONNECTED" || type == "PLAYER_RECONNECTED") {
        refreshOnlineGameView();
    }
}

void OctiMainWindow::pumpUiEvents()
{
    _dispatcher.drain();
    if (QCoreApplication::instance()) {
        QCoreApplication::processEvents();
    }
    _dispatcher.drain();
}

void OctiMainWindow::handleLocalOrOnlineMoveCommitted(
    const std::string &moveNotation)
{
    if (_boardCanvas && !moveNotation.empty()) {
        try {
            OctiMove parsedMove(moveNotation);
            bool edgeless = _board.getGame() && _board.getGame()->useEdgelessBoard();
            _boardCanvas->setLastMoveModifiedSquares(
                OctiBoardCanvas::extractModifiedSquares(parsedMove, edgeless));
        } catch (...) {
        }
    }

    if (_inOnlineRoom && !_onlineRoomId.empty() && _netClient) {
        _appliedOnlineMoves.push_back(moveNotation);
        auto res = _netClient->submitMove(_onlineRoomId, moveNotation);
        if (!res.ok) {
            _appliedOnlineMoves.pop_back();
            if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
                showProtocolUpgradeRequired(res.errorMessage);
            }
        }
        refreshOnlineGameView();
        return;
    }

    if (_boardCanvas) {
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    if (_board.getGame()) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        int elapsedMs = 0;
        if (_currentTurnStartedEpochMs > 0) {
            elapsedMs = static_cast<int>(std::max<qint64>(0, now - _currentTurnStartedEpochMs));
        }
        OctiPlyMetadata plyMeta;
        plyMeta.isAiMove = false;
        plyMeta.elapsedMs = elapsedMs;
        _board.getGame()->setLastPlyMetadata(plyMeta);
        _currentTurnStartedEpochMs = now;
    }
    refreshMoveHistoryAndFenUi();
    checkAndPresentGameOver();
    if (_isLocalBotGame && _board.getGame() && !_board.getGame()->isGameOver()) {
        executeBotTurnIfNeeded();
    }
}

/* ---- Board Chat & Mutual Proposals ---- */

bool OctiMainWindow::sendGameChatMessage(const std::string &scope,
                                         const std::string &text)
{
    if (text.empty()) {
        return false;
    }
    if (_inOnlineRoom && !_onlineRoomId.empty() && _netClient) {
        auto res = _netClient->postGameChat(_onlineRoomId, scope, text);
        if (!res.ok) {
            return false;
        }
        std::string handle =
            _profile.handle.empty() ? "Me" : _profile.handle;
        std::string line = "[" + scope + "] " + handle + ": " + text;
        if (_gameChatLines.empty() || _gameChatLines.back() != line) {
            _gameChatLines.push_back(line);
            if (_gameChatTranscript) {
                _gameChatTranscript->appendPlainText(
                    QString::fromStdString(line));
            }
        }
        return true;
    }

    std::string line = "[LOCAL] Player: " + text;
    _gameChatLines.push_back(line);
    if (_gameChatTranscript) {
        _gameChatTranscript->appendPlainText(QString::fromStdString(line));
    }
    return true;
}

const std::vector<std::string> &OctiMainWindow::gameChatLines() const
{
    return _gameChatLines;
}

bool OctiMainWindow::hasActiveProposal() const
{
    return !_onlineGameView.proposals.empty();
}

std::string OctiMainWindow::activeProposalType() const
{
    if (_onlineGameView.proposals.empty()) {
        return std::string();
    }
    return _onlineGameView.proposals.front().type;
}

QString OctiMainWindow::proposalBannerText() const
{
    return _proposalBannerLabel ? _proposalBannerLabel->text() : QString();
}

ApiResult<GameActionData> OctiMainWindow::proposeDraw()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty()) {
        res.errorMessage = "Not in an online room";
        return res;
    }
    res = _netClient->submitGameAction(_onlineRoomId, "offer_draw");
    refreshOnlineGameView();
    return res;
}

ApiResult<GameActionData> OctiMainWindow::proposePauseOrResume()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty()) {
        res.errorMessage = "Not in an online room";
        return res;
    }
    const char *act =
        _onlineGameView.paused ? "resume" : "request_pause";
    res = _netClient->submitGameAction(_onlineRoomId, act);
    refreshOnlineGameView();
    return res;
}

ApiResult<GameActionData> OctiMainWindow::proposeUndo()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty()) {
        res.errorMessage = "Not in an online room";
        return res;
    }
    res = _netClient->submitGameAction(_onlineRoomId, "request_undo");
    refreshOnlineGameView();
    return res;
}

ApiResult<GameActionData> OctiMainWindow::proposeRematch()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty()) {
        res.errorMessage = "Not in an online room";
        return res;
    }
    res = _netClient->submitGameAction(_onlineRoomId, "request_rematch");
    refreshOnlineGameView();
    return res;
}

ApiResult<GameActionData> OctiMainWindow::claimOpponentForfeit()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty()) {
        res.errorMessage = "Not in an online room";
        return res;
    }
    res = _netClient->submitGameAction(_onlineRoomId, "claim_forfeit");
    refreshOnlineGameView();
    return res;
}

ApiResult<bool> OctiMainWindow::resignCurrentGame()
{
    ApiResult<bool> res;
    if (_inOnlineRoom && !_onlineRoomId.empty() && _netClient) {
        res = _netClient->resignGame(_onlineRoomId);
        refreshOnlineGameView();
        return res;
    }
    if (_board.getGame() && _board.getGame()->isGameStarted() &&
        !_board.getGame()->isGameOver()) {
        _board.getGame()->resign(_board.getGame()->getTurn());
        _composer.refreshLegalMoves();
        if (_boardCanvas) {
            _boardCanvas->update();
        }
        if (_reservePanel) {
            _reservePanel->refresh();
        }
        refreshMoveHistoryAndFenUi();
        checkAndPresentGameOver();
        res.ok = true;
        res.data = true;
    }
    return res;
}

ApiResult<GameActionData> OctiMainWindow::acceptActiveProposal()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty() ||
        _onlineGameView.proposals.empty()) {
        res.errorMessage = "No active proposal";
        return res;
    }
    std::string pType = _onlineGameView.proposals.front().type;
    std::string action = "accept_draw";
    if (pType == "PAUSE_REQUEST") {
        action = "accept_pause";
    } else if (pType == "UNDO_REQUEST") {
        action = "accept_undo";
    } else if (pType == "REMATCH_REQUEST") {
        action = "accept_rematch";
    }
    res = _netClient->submitGameAction(_onlineRoomId, action);
    refreshOnlineGameView();
    return res;
}

ApiResult<GameActionData> OctiMainWindow::declineActiveProposal()
{
    ApiResult<GameActionData> res;
    if (!_netClient || _onlineRoomId.empty() ||
        _onlineGameView.proposals.empty()) {
        res.errorMessage = "No active proposal";
        return res;
    }
    std::string pType = _onlineGameView.proposals.front().type;
    std::string action = "decline_draw";
    if (pType == "PAUSE_REQUEST") {
        action = "decline_pause";
    } else if (pType == "UNDO_REQUEST") {
        action = "decline_undo";
    } else if (pType == "REMATCH_REQUEST") {
        action = "decline_rematch";
    }
    res = _netClient->submitGameAction(_onlineRoomId, action);
    refreshOnlineGameView();
    return res;
}

/* ---- Push-to-Talk Voice Control ---- */

bool OctiMainWindow::joinOnlineVoice()
{
    if (!_voiceEngine || _onlineRoomId.empty() || _profile.uuid.empty()) {
        return false;
    }
    std::string err;
    if (!_voiceEngine->joinVoiceRoom(_onlineRoomId, _profile.uuid, err)) {
        return false;
    }
    for (std::size_t i = 0; i < _onlineGameView.seats.size(); ++i) {
        const std::string &peerUuid = _onlineGameView.seats[i].playerUuid;
        if (!peerUuid.empty() && peerUuid != _profile.uuid) {
            _voiceEngine->initiatePeerOffer(peerUuid);
        }
    }
    updateVoiceControlUi();
    return true;
}

void OctiMainWindow::leaveOnlineVoice()
{
    if (_voiceEngine) {
        _voiceEngine->leaveVoiceRoom();
    }
    if (_reservePanel) {
        _reservePanel->setPushToTalkActive(false);
    }
    updateVoiceControlUi();
    setupMenuBar();
}

bool OctiMainWindow::isInVoiceRoom() const
{
    return _voiceEngine && _voiceEngine->isInVoiceRoom();
}

bool OctiMainWindow::setPushToTalkActive(bool talking)
{
    bool ok = false;
    if (_voiceEngine && _voiceEngine->isInVoiceRoom()) {
        ok = _voiceEngine->setPushToTalkActive(talking);
    } else {
        ok = talking;
    }
    if (_reservePanel) {
        _reservePanel->setPushToTalkActive(
            _voiceEngine && _voiceEngine->isInVoiceRoom()
                ? _voiceEngine->isPushToTalkActive()
                : talking);
    }
    updateVoiceControlUi();
    return ok;
}

bool OctiMainWindow::isPushToTalkActive() const
{
    if (_voiceEngine && _voiceEngine->isInVoiceRoom()) {
        return _voiceEngine->isPushToTalkActive();
    }
    return _reservePanel ? _reservePanel->isPushToTalkActive() : false;
}

void OctiMainWindow::simulatePushToTalkKeyPress()
{
    setPushToTalkActive(true);
}

void OctiMainWindow::simulatePushToTalkKeyRelease()
{
    setPushToTalkActive(false);
}

/* ---- Move History, VCR Replay & .octi Transcript ---- */

void OctiMainWindow::vcrFirst()
{
    OctiGame *g = _board.getGame();
    if (!g) {
        return;
    }
    while (g->canUndo()) {
        g->undo();
    }
    _composer.refreshLegalMoves();
    if (_boardCanvas) {
        _boardCanvas->clearLastMoveModifiedSquares();
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    refreshMoveHistoryAndFenUi();
}

void OctiMainWindow::vcrPrev()
{
    OctiGame *g = _board.getGame();
    if (!g || !g->canUndo()) {
        return;
    }
    g->undo();
    _composer.refreshLegalMoves();
    if (_boardCanvas) {
        _boardCanvas->clearLastMoveModifiedSquares();
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    refreshMoveHistoryAndFenUi();
}

void OctiMainWindow::vcrNext()
{
    OctiGame *g = _board.getGame();
    if (!g || !g->canRedo()) {
        return;
    }
    g->redo();
    _composer.refreshLegalMoves();
    if (_boardCanvas) {
        _boardCanvas->clearLastMoveModifiedSquares();
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    refreshMoveHistoryAndFenUi();
}

void OctiMainWindow::vcrLast()
{
    OctiGame *g = _board.getGame();
    if (!g) {
        return;
    }
    while (g->canRedo()) {
        g->redo();
    }
    _composer.refreshLegalMoves();
    if (_boardCanvas) {
        _boardCanvas->clearLastMoveModifiedSquares();
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    refreshMoveHistoryAndFenUi();
}

int OctiMainWindow::vcrCurrentPly() const
{
    const OctiGame *g = _board.getGame();
    return g ? g->getNumMoves() : 0;
}

int OctiMainWindow::vcrTotalPlies() const
{
    return std::max(_vcrTotalPlies, vcrCurrentPly());
}

QString OctiMainWindow::fenStatusText() const
{
    return _fenStatusLabel ? _fenStatusLabel->text() : QString();
}

std::vector<std::string> OctiMainWindow::moveHistoryLines() const
{
    std::vector<std::string> out;
    if (!_moveHistoryList) {
        return out;
    }
    for (int i = 0; i < _moveHistoryList->count(); ++i) {
        out.push_back(_moveHistoryList->item(i)->text().toStdString());
    }
    return out;
}

QString OctiMainWindow::exportMoveHistoryText() const
{
    QString out;
    const OctiGame *g = _board.getGame();
    if (!g) {
        return out;
    }
    std::vector<std::pair<OctiPlayer, OctiMove>> history;
    g->getMoves(history);
    for (std::size_t i = 0; i < history.size(); ++i) {
        char pCh = 'A';
        switch (history[i].first) {
        case PLAYER_A: pCh = 'A'; break;
        case PLAYER_B: pCh = 'B'; break;
        case PLAYER_C: pCh = 'C'; break;
        case PLAYER_D: pCh = 'D'; break;
        default:       pCh = '?'; break;
        }
        if (!out.isEmpty()) {
            out += "\n";
        }
        out += QString("%1. %2: %3")
            .arg(i + 1)
            .arg(QChar(pCh))
            .arg(QString::fromStdString(history[i].second.asString()));
    }
    return out;
}

QListWidget *OctiMainWindow::moveHistoryListWidget() const
{
    return _moveHistoryList;
}

bool OctiMainWindow::loadOctiTranscriptIntoReplay(
    const std::string &octiTranscript)
{
    if (octiTranscript.empty()) {
        return false;
    }
    leaveOnlineRoom();
    _gameOverDialogShownForCurrentMatch = false;
    _isLocalBotGame = false;
    try {
        _board.deserializeGame(octiTranscript);
    } catch (...) {
        return false;
    }
    _vcrTotalPlies = _board.getGame() ? _board.getGame()->getNumMoves() : 0;
    _composer.attachBoard(&_board);
    if (_boardCanvas) {
        _boardCanvas->clearLastMoveModifiedSquares();
        _boardCanvas->attachBoard(&_board, &_composer, &_soundSynth);
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->attach(&_board, &_composer, _boardCanvas, &_soundSynth);
        _reservePanel->refresh();
    }
    refreshMoveHistoryAndFenUi();
    return true;
}

std::string OctiMainWindow::buildCurrentGameTranscriptJson(
    bool includeExtendedTelemetry) const
{
    const OctiGame *game = _board.getGame();
    if (!game) {
        return "{}";
    }

    OctiGameMetadata meta;
    meta.source = _inOnlineRoom ? "ONLINE" : (_isLocalBotGame ? "LOCAL_BOT" : "LOCAL");
    if (game->isGameResigned()) {
        meta.status = "RESIGNED";
        meta.termination = "RESIGNATION";
    } else if (game->isGameOver()) {
        meta.status = "GAME_OVER";
        meta.termination = "BASE_OCCUPATION";
    } else if (game->isGameStarted()) {
        meta.status = "IN_PROGRESS";
    } else {
        meta.status = "NOT_STARTED";
    }

    meta.appVersion = OCTI_VERSION_STRING;
    meta.buildType = "release";
    meta.platform = "desktop-qt6";

    int numPlayers = (game->getMode() == GAME_MODE_4) ? 4 : 2;
    for (int i = 1; i <= numPlayers; ++i) {
        OctiPlayerMetadata pm;
        pm.seat = i;
        pm.player = static_cast<OctiPlayer>(i);
        if (_isLocalBotGame) {
            if (pm.player == _localConfig.humanSeat) {
                pm.type = "HUMAN";
                pm.handle = _profile.handle.empty() ? "Player" : _profile.handle;
            } else {
                pm.type = "AI";
                int lvl = static_cast<int>(_botEngine.difficulty());
                pm.handle = "AI_L" + std::to_string(lvl);
                pm.aiDifficulty = lvl;
                pm.aiBudgetMs = _botEngine.timeBudgetMs();
            }
        } else {
            pm.type = "HUMAN";
            pm.handle = "Player " + std::string(1, 'A' + i - 1);
        }
        meta.players.push_back(pm);
    }

    if (includeExtendedTelemetry) {
        meta.extended.present = true;
        meta.extended.startedAt = _matchStartedEpochSec > 0 ? _matchStartedEpochSec : QDateTime::currentSecsSinceEpoch();
        meta.extended.endedAt = QDateTime::currentSecsSinceEpoch();
        meta.extended.utcOffsetMin = static_cast<int>(QDateTime::currentDateTime().offsetFromUtc() / 60);
        meta.extended.locale = QLocale::system().name().toStdString();
        meta.extended.osVersion = QSysInfo::prettyProductName().toStdString();
        meta.extended.installId = _configStore.preferences().installId;
    } else {
        meta.extended.present = false;
    }

    return game->serializeJson(meta);
}

std::string OctiMainWindow::exportCurrentOctiTranscript() const
{
    return buildCurrentGameTranscriptJson(false);
}

QString OctiMainWindow::copyExportedNotationToClipboard() const
{
    QString text = QString::fromStdString(exportCurrentOctiTranscript());
    if (QClipboard *cb = QGuiApplication::clipboard()) {
        cb->setText(text);
    }
    return text;
}

void OctiMainWindow::showExportNotationDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Export OCTI Game Notation");
    dlg.setModal(true);
    dlg.resize(560, 440);
    dlg.setStyleSheet(buildAppStyleSheet(_theme));

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    QLabel *hdr = new QLabel(
        "Canonical OCTI Transcript (JSON v3.0):", &dlg);
    hdr->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(hdr);

    QPlainTextEdit *editor = new QPlainTextEdit(&dlg);
    editor->setReadOnly(true);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    editor->setFont(monoFont);
    std::string jsonStr = exportCurrentOctiTranscript();
    try {
        Json j = Json::parse(jsonStr, nullptr, false);
        if (!j.is_discarded()) {
            jsonStr = j.dump(2);
        }
    } catch (...) {}
    editor->setPlainText(QString::fromStdString(jsonStr));
    layout->addWidget(editor, 1);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    QPushButton *saveBtn = new QPushButton("Save to File...", &dlg);
    QPushButton *copyTranscriptBtn = new QPushButton("Copy Transcript", &dlg);
    QPushButton *closeBtn = new QPushButton("Close", &dlg);
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(copyTranscriptBtn);
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    QObject::connect(saveBtn, &QPushButton::clicked, &dlg, [this, &dlg, saveBtn]() {
        QString fileName = QFileDialog::getSaveFileName(
            &dlg, "Save OCTI Transcript", QString(), "OCTI JSON Transcript (*.json *.octi)");
        if (!fileName.isEmpty()) {
            if (saveTranscriptToFile(fileName.toStdString())) {
                saveBtn->setText("Saved!");
            }
        }
    });
    QObject::connect(copyTranscriptBtn, &QPushButton::clicked, &dlg, [this, copyTranscriptBtn]() {
        copyExportedNotationToClipboard();
        copyTranscriptBtn->setText("Copied Transcript!");
    });
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
}

bool OctiMainWindow::saveTranscriptToFile(const std::string &filePath) const
{
    if (filePath.empty()) {
        return false;
    }
    std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << exportCurrentOctiTranscript();
    return ofs.good();
}

bool OctiMainWindow::loadTranscriptFromFile(const std::string &filePath)
{
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return loadOctiTranscriptIntoReplay(ss.str());
}

QImage OctiMainWindow::renderWindowToImage(int width, int height)
{
    resize(width, height);
    pumpUiEvents();
    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(paletteForTheme(_theme).windowBg);
    render(&img);
    return img;
}

void OctiMainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event && event->key() == Qt::Key_V && !event->isAutoRepeat()) {
        QWidget *fw = QApplication::focusWidget();
        if (!dynamic_cast<QLineEdit *>(fw)) {
            setPushToTalkActive(true);
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void OctiMainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event && event->key() == Qt::Key_V && !event->isAutoRepeat()) {
        setPushToTalkActive(false);
        event->accept();
        return;
    }
    QMainWindow::keyReleaseEvent(event);
}

bool OctiMainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event && (event->type() == QEvent::KeyPress ||
                  event->type() == QEvent::KeyRelease)) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_V && !ke->isAutoRepeat()) {
            QWidget *fw = QApplication::focusWidget();
            if (!dynamic_cast<QLineEdit *>(fw)) {
                setPushToTalkActive(event->type() == QEvent::KeyPress);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void OctiMainWindow::updateTopBarLabels()
{
    if (_serverStatusLabel) {
        QString stateStr = _offlineMode
                               ? "Offline Mode"
                               : (_authenticated ? "Connected" : "Ready");
        _serverStatusLabel->setText(stateStr);
    }
    if (_profileBadgeLabel) {
        if (_authenticated) {
            _profileBadgeLabel->setText(
                QString("%1 • 2P: %2 • 4P: %3 • OFK: %4")
                    .arg(QString::fromStdString(_profile.handle))
                    .arg(_profile.elo2p)
                    .arg(_profile.elo4p)
                    .arg(_profile.eloOfk));
        } else {
            QString localMode = _isLocalBotGame
                ? "vs AI Bot"
                : "Pass-and-Play";
            _profileBadgeLabel->setText(
                QString("Not Signed In (Local Practice • %1)").arg(localMode));
        }
    }
}

void OctiMainWindow::refreshMoveHistoryAndFenUi()
{
    const OctiGame *g = _board.getGame();
    if (_moveHistoryList && g) {
        _moveHistoryList->clear();
        std::vector<std::pair<OctiPlayer, OctiMove>> history;
        g->getMoves(history);
        if (static_cast<int>(history.size()) > _vcrTotalPlies) {
            _vcrTotalPlies = static_cast<int>(history.size());
        }
        const OctiPalette &pal = paletteForTheme(_theme);
        for (std::size_t i = 0; i < history.size(); ++i) {
            char pCh = 'A';
            switch (history[i].first) {
            case PLAYER_A: pCh = 'A'; break;
            case PLAYER_B: pCh = 'B'; break;
            case PLAYER_C: pCh = 'C'; break;
            case PLAYER_D: pCh = 'D'; break;
            default:       pCh = '?'; break;
            }
            QString line = QString("%1. %2: %3")
                .arg(i + 1)
                .arg(QChar(pCh))
                .arg(QString::fromStdString(history[i].second.asString()));
            QListWidgetItem *item = new QListWidgetItem(line);
            const PodFactionPalette &fp = pal.factionForPlayer(history[i].first);
            item->setBackground(QBrush(fp.shadow));
            item->setForeground(QBrush(fp.highlight));
            _moveHistoryList->addItem(item);
        }
    }
    if (_fenStatusLabel) {
        _fenStatusLabel->setText(
            QString("FEN: %1")
                .arg(QString::fromStdString(_board.toPositionString())));
        _fenStatusLabel->setVisible(false);
    }
    updateMatchActionsVisibility();
}

void OctiMainWindow::updateMatchActionsVisibility()
{
    bool online = (_inOnlineRoom && !_onlineRoomId.empty());
    if (_offerDrawBtn) {
        _offerDrawBtn->setVisible(online);
    }
    if (_pauseResumeBtn) {
        _pauseResumeBtn->setVisible(online);
    }
    if (_claimForfeitBtn) {
        _claimForfeitBtn->setVisible(online);
    }
    if (_moderationBtn) {
        _moderationBtn->setVisible(online);
    }
    if (_chatBox) {
        _chatBox->setVisible(online);
    }
    if (_voiceBox) {
        _voiceBox->setVisible(online);
    }

    if (_matchModeLabel) {
        if (online) {
            _matchModeLabel->setText(
                QString("Online Room: %1")
                    .arg(QString::fromStdString(_onlineRoomId)));
        } else if (_isLocalBotGame) {
            _matchModeLabel->setText(
                QString::fromUtf8("Local vs AI Bot \xE2\x80\xA2 Offline Tabletop"));
        } else {
            _matchModeLabel->setText(
                QString::fromUtf8("Local Pass-and-Play \xE2\x80\xA2 Offline Tabletop"));
        }
    }

    if (_matchTurnStatusLabel) {
        if (_awaitingMatchStart) {
            _matchTurnStatusLabel->setText("Awaiting Match Start");
        } else {
            const OctiGame *g = _board.getGame();
            if (g && g->isGameOver()) {
                _matchTurnStatusLabel->setText(
                    QString::fromUtf8("%1 \xE2\x80\x94 %2")
                        .arg(GameOverDialog::describeWinnerBanner(&_board))
                        .arg(GameOverDialog::describeTerminationReason(&_board)));
            } else if (g) {
                int pIdx = static_cast<int>(g->getTurn());
                QChar pCh = (pIdx >= 1 && pIdx <= 4) ? QChar('A' + pIdx - 1) : QChar('A');
                _matchTurnStatusLabel->setText(
                    QString("Active Turn: Player %1").arg(pCh));
            } else {
                _matchTurnStatusLabel->setText("Match Idle");
            }
        }
    }
}

void OctiMainWindow::checkAndPresentGameOver()
{
    const OctiGame *g = _board.getGame();
    if (!g || !g->isGameOver()) {
        return;
    }
    if (_gameOverDialogShownForCurrentMatch) {
        return;
    }
    _gameOverDialogShownForCurrentMatch = true;
    _soundSynth.triggerCue(SoundCue::GAME_VICTORY);
    updateMatchActionsVisibility();
    if (_boardCanvas) {
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    uploadCompletedBotGameIfEligible(false);
    if (isVisible()) {
        GameOverDialog dlg(&_board, _theme, this);
        dlg.exec();
        if (dlg.newGameRequested()) {
            if (_inOnlineRoom && !_onlineRoomId.empty()) {
                proposeRematch();
            } else {
                NewLocalGameDialog newDlg(_theme, this);
                newDlg.setConfig(_localConfig);
                if (newDlg.exec() == QDialog::Accepted) {
                    startNewLocalGame(newDlg.config());
                    beginAwaitingMatchStart();
                    promptStartMatchIfVisible();
                }
            }
        }
    }
}

void OctiMainWindow::checkPromptTelemetryConsentAtStartup()
{
    if (!isVisible()) {
        return;
    }
    ClientPreferences &prefs = _configStore.preferences();
    if (prefs.telemetryConsent == "unset" ||
        prefs.telemetryConsentVersion < ClientPreferences::CURRENT_TELEMETRY_CONSENT_VERSION) {
        OctiTelemetryConsentDialog dlg(_theme, this);
        dlg.exec();
        if (dlg.userConsented()) {
            prefs.telemetryConsent = "share";
            if (prefs.installId.empty()) {
                prefs.installId = ClientConfigStore::generateUuidV4();
            }
        } else {
            prefs.telemetryConsent = "decline";
        }
        prefs.telemetryConsentVersion = ClientPreferences::CURRENT_TELEMETRY_CONSENT_VERSION;
        std::string err;
        _configStore.save(err);

        if (_heldTranscriptPendingConsent) {
            _heldTranscriptPendingConsent = false;
            uploadCompletedBotGameIfEligible(false);
        }
    }
}

void OctiMainWindow::setTelemetryConsentForTesting(const std::string &choice)
{
    ClientPreferences &prefs = _configStore.preferences();
    prefs.telemetryConsent = choice;
    prefs.telemetryConsentVersion = ClientPreferences::CURRENT_TELEMETRY_CONSENT_VERSION;
    if (choice == "share" && prefs.installId.empty()) {
        prefs.installId = ClientConfigStore::generateUuidV4();
    }
    std::string err;
    _configStore.save(err);
    if (choice != "unset") {
        _heldTranscriptPendingConsent = false;
    }
}

bool OctiMainWindow::uploadCompletedBotGameIfEligible(bool synchronousForTest)
{
    if (!_isLocalBotGame || _inOnlineRoom) {
        return false;
    }
    OctiGame *game = _board.getGame();
    if (!game || !game->isGameStarted() || !game->isGameOver()) {
        return false;
    }
    if (game->getNumMoves() == 0) {
        return false;
    }
    if (_transcriptUploadedForCurrentMatch) {
        return false;
    }

    const ClientPreferences &prefs = _configStore.preferences();
    if (prefs.telemetryConsent == "unset") {
        _heldTranscriptPendingConsent = true;
        return false;
    }

    _transcriptUploadedForCurrentMatch = true;
    _heldTranscriptPendingConsent = false;

    bool shareExtra = (prefs.telemetryConsent == "share");
    std::string jsonStr = buildCurrentGameTranscriptJson(shareExtra);

    std::string gameId = "game_" + std::to_string(QDateTime::currentSecsSinceEpoch()) +
                         "_" + std::to_string(game ? game->getNumMoves() : 0);
    std::string clientId = (shareExtra && !prefs.installId.empty()) ? prefs.installId : "";

    std::string configDir = std::filesystem::path(_configStore.configFilePath()).parent_path().string();
    if (!configDir.empty()) {
        octi::desktop::OctiCrashReporter::setStorageRootDir(configDir);
    }
    octi::desktop::OctiCrashReporter::spoolTrainingTranscript(gameId, jsonStr, clientId);

    std::string uploadInstallId = shareExtra ? prefs.installId : "";
    if (synchronousForTest) {
        if (_netClient) {
            int flushed = octi::desktop::OctiCrashReporter::flushPendingTranscripts(
                *_netClient, uploadInstallId);
            octi::desktop::OctiCrashReporter::flushPendingCrashReports(
                *_netClient, uploadInstallId);
            return flushed > 0;
        }
        return false;
    } else {
        if (_netClient) {
            if (_uploadThread.joinable()) {
                _uploadThread.join();
            }
            _uploadThread = std::thread([this, installId = uploadInstallId]() {
                try {
                    if (_netClient) {
                        octi::desktop::OctiCrashReporter::flushPendingTranscripts(
                            *_netClient, installId);
                        octi::desktop::OctiCrashReporter::flushPendingCrashReports(
                            *_netClient, installId);
                    }
                } catch (const std::exception &ex) {
                    octi::desktop::OctiCrashReporter::recordHandledException(
                        "OctiUploadThread", typeid(ex).name(), ex.what(),
                        octi::desktop::OctiCrashReporter::getGameContextJson(),
                        installId);
                } catch (...) {
                    octi::desktop::OctiCrashReporter::recordHandledException(
                        "OctiUploadThread", "unknown_exception",
                        "Unknown exception in upload thread",
                        octi::desktop::OctiCrashReporter::getGameContextJson(),
                        installId);
                }
            });
        }
        return true;
    }
}

void OctiMainWindow::waitForBackgroundUpload()
{
    if (_uploadThread.joinable()) {
        _uploadThread.join();
    }
}

void OctiMainWindow::flushPendingTelemetry(bool async)
{
    if (_offlineMode || !_netClient) {
        return;
    }
    std::string configDir = std::filesystem::path(_configStore.configFilePath()).parent_path().string();
    if (!configDir.empty()) {
        octi::desktop::OctiCrashReporter::setStorageRootDir(configDir);
    }
    const ClientPreferences &prefs = _configStore.preferences();
    bool shareExtra = (prefs.telemetryConsent == "share");
    std::string uploadInstallId = shareExtra ? prefs.installId : "";
    if (async) {
        if (_uploadThread.joinable()) {
            _uploadThread.join();
        }
        _uploadThread = std::thread([this, installId = uploadInstallId]() {
            try {
                if (_netClient) {
                    octi::desktop::OctiCrashReporter::flushPendingTranscripts(
                        *_netClient, installId);
                    octi::desktop::OctiCrashReporter::flushPendingCrashReports(
                        *_netClient, installId);
                }
            } catch (const std::exception &ex) {
                octi::desktop::OctiCrashReporter::recordHandledException(
                    "OctiUploadThread", typeid(ex).name(), ex.what(),
                    octi::desktop::OctiCrashReporter::getGameContextJson(),
                    installId);
            } catch (...) {
                octi::desktop::OctiCrashReporter::recordHandledException(
                    "OctiUploadThread", "unknown_exception",
                    "Unknown exception in upload thread",
                    octi::desktop::OctiCrashReporter::getGameContextJson(),
                    installId);
            }
        });
    } else {
        octi::desktop::OctiCrashReporter::flushPendingTranscripts(
            *_netClient, uploadInstallId);
        octi::desktop::OctiCrashReporter::flushPendingCrashReports(
            *_netClient, uploadInstallId);
    }
}

void OctiMainWindow::showTelemetrySettingsDialog()
{
    OctiTelemetrySettingsDialog dlg(&_configStore, _netClient.get(), _theme, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (_heldTranscriptPendingConsent && _configStore.preferences().telemetryConsent != "unset") {
            _heldTranscriptPendingConsent = false;
            uploadCompletedBotGameIfEligible(false);
        }
    }
}

void OctiMainWindow::updateProposalBannerUi()
{
    if (!_proposalBannerWidget || !_proposalBannerLabel) {
        return;
    }
    if (_onlineGameView.proposals.empty()) {
        _proposalBannerWidget->setVisible(false);
        _proposalBannerLabel->setText("No active proposal.");
        return;
    }
    const GameProposalInfo &prop = _onlineGameView.proposals.front();
    _proposalBannerLabel->setText(
        QString("Proposal: %1 (from %2)")
            .arg(QString::fromStdString(prop.type))
            .arg(QString::fromStdString(prop.proposerUuid)));
    _proposalBannerWidget->setVisible(true);
}

void OctiMainWindow::updateVoiceControlUi()
{
    if (!_voiceStatusLabel || !_joinLeaveVoiceBtn || !_holdToTalkBtn) {
        return;
    }
    if (!_voiceEngine || !_voiceEngine->isInVoiceRoom()) {
        _voiceStatusLabel->setText("Voice: Disconnected (Strict PTT)");
        _joinLeaveVoiceBtn->setText("Join Voice");
        _holdToTalkBtn->setText("Hold to Talk [V]");
        return;
    }
    _joinLeaveVoiceBtn->setText("Leave Voice");
    if (_voiceEngine->isVoicePolicyDisabled()) {
        _voiceStatusLabel->setText("Voice: Disabled by Room Policy");
    } else if (_voiceEngine->isVoiceMutedByServer()) {
        _voiceStatusLabel->setText("Voice: Muted by Room Moderator");
    } else if (_voiceEngine->isPushToTalkActive()) {
        _voiceStatusLabel->setText(
            QString("Voice: TRANSMITTING PTT (%1 peer(s))")
                .arg(static_cast<int>(_voiceEngine->activePeerCount())));
        _holdToTalkBtn->setText("TRANSMITTING [V]");
    } else {
        _voiceStatusLabel->setText(
            QString("Voice: Connected • Mic Muted (%1 peer(s))")
                .arg(static_cast<int>(_voiceEngine->activePeerCount())));
        _holdToTalkBtn->setText("Hold to Talk [V]");
    }
}

/* ---- Self-Update Control & Status ---- */

selfupdate::CheckerConfig OctiMainWindow::defaultSelfUpdateConfig()
{
    selfupdate::CheckerConfig cfg;
    cfg.applicationId = "com.selfso.octi";
    cfg.installedVersionName = OctiCore::versionString();
    cfg.installedVersionCode =
        selfupdate::UpdateManifest::parseVersionCode(cfg.installedVersionName);
    cfg.manifestUrl = selfupdate::UpdateManifest::DEFAULT_MANIFEST_URL;
    return cfg;
}

selfupdate::SelfUpdateController *OctiMainWindow::selfUpdateController() const
{
    return _updateController.get();
}

std::shared_ptr<selfupdate::SelfUpdateController>
OctiMainWindow::selfUpdateControllerShared() const
{
    return _updateController;
}

void OctiMainWindow::setSelfUpdateController(
    std::shared_ptr<selfupdate::SelfUpdateController> controller)
{
    if (_updateController) {
        _updateController->setStateCallback(nullptr);
        _updateController->setQuitCallback(nullptr);
        _updateController->setUiDispatcher(nullptr);
        _updateController->cancel();
    }
    _quitRequestedByUpdate = false;
    _updateController = std::move(controller);
    if (_updateController) {
        bindSelfUpdateController();
    }
}

void OctiMainWindow::checkForAppUpdates(bool isManualCheck)
{
    _updateBannerDismissed = false;
    _quitRequestedByUpdate = false;
    _currentUpdateState.phase = selfupdate::UpdatePhase::Checking;
    _currentUpdateState.isManualCheck = isManualCheck;
    if (_updateController) {
        _updateController->checkForUpdates(isManualCheck);
    }
}

void OctiMainWindow::triggerAppUpdateDownloadAndInstall()
{
    if (!_updateController) {
        return;
    }
    if (_currentUpdateState.phase == selfupdate::UpdatePhase::ReadyToInstall) {
        _updateController->installStagedPackage();
    } else if (_currentUpdateState.phase == selfupdate::UpdatePhase::UpToDate) {
        _updateController->reinstall();
    } else {
        _updateController->downloadAndInstall();
    }
}

void OctiMainWindow::cancelAppUpdateDownload()
{
    if (_updateController) {
        _updateController->cancel();
    }
}

void OctiMainWindow::dismissAppUpdateBanner()
{
    _updateBannerDismissed = true;
    if (_updateBannerWidget) {
        _updateBannerWidget->setVisible(false);
    }
    if (_updateController) {
        _updateController->dismiss();
    }
    _currentUpdateState.phase = selfupdate::UpdatePhase::Idle;
}

const selfupdate::UpdateState &OctiMainWindow::currentSelfUpdateState() const
{
    return _currentUpdateState;
}

bool OctiMainWindow::isUpdateBannerVisible() const
{
    return _updateBannerWidget && !_updateBannerWidget->isHidden();
}

QString OctiMainWindow::updateBannerText() const
{
    return _updateBannerLabel ? _updateBannerLabel->text() : QString();
}

QString OctiMainWindow::checkUpdateButtonText() const
{
    return _checkUpdateBtn ? _checkUpdateBtn->text() : QString();
}

bool OctiMainWindow::wasQuitRequestedByUpdate() const
{
    return _quitRequestedByUpdate;
}

void OctiMainWindow::registerUpdateDialog(SelfUpdateDialog *dlg)
{
    _activeUpdateDialog = dlg;
}

void OctiMainWindow::unregisterUpdateDialog(SelfUpdateDialog *dlg)
{
    if (_activeUpdateDialog == dlg) {
        _activeUpdateDialog = nullptr;
    }
}

void OctiMainWindow::bindSelfUpdateController()
{
    if (!_updateController) {
        return;
    }
    _updateController->setUiDispatcher([this](std::function<void()> task) {
        _dispatcher.post(std::move(task));
    });
    _updateController->setStateCallback(
        [this](const selfupdate::UpdateState &st) {
            handleSelfUpdateStateChanged(st);
        });
    _updateController->setQuitCallback([this]() {
        handleSelfUpdateQuitRequested();
    });
    _currentUpdateState = _updateController->state();
    updateSelfUpdateBannerUi();
}

void OctiMainWindow::handleSelfUpdateStateChanged(
    const selfupdate::UpdateState &state)
{
    _currentUpdateState = state;
    updateSelfUpdateBannerUi();
    if (_activeUpdateDialog != nullptr) {
        _activeUpdateDialog->applyUpdateState(state);
    }
}

void OctiMainWindow::handleSelfUpdateQuitRequested()
{
    _quitRequestedByUpdate = true;
    if (_activeUpdateDialog != nullptr) {
        _activeUpdateDialog->accept();
    }
    leaveOnlineRoom();
    if (_netClient) {
        _netClient->stopEventStreams();
    }
    close();
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->quit();
    }
}

void OctiMainWindow::updateSelfUpdateBannerUi()
{
    if (_checkUpdateBtn) {
        if (_currentUpdateState.phase == selfupdate::UpdatePhase::Available ||
            _currentUpdateState.phase == selfupdate::UpdatePhase::ReadyToInstall) {
            _checkUpdateBtn->setText(
                QString("Update Available (v%1)")
                    .arg(QString::fromStdString(
                        _currentUpdateState.manifest.versionName)));
        } else {
            _checkUpdateBtn->setText("Check for Updates");
        }
    }

    bool shouldShowBanner = false;
    if (!_updateBannerDismissed) {
        switch (_currentUpdateState.phase) {
        case selfupdate::UpdatePhase::Available:
        case selfupdate::UpdatePhase::Downloading:
        case selfupdate::UpdatePhase::ReadyToInstall:
        case selfupdate::UpdatePhase::Installing:
        case selfupdate::UpdatePhase::Relaunching:
            shouldShowBanner = true;
            break;
        case selfupdate::UpdatePhase::UpToDate:
            if (_currentUpdateState.isManualCheck) {
                shouldShowBanner = true;
            }
            break;
        case selfupdate::UpdatePhase::Error:
            if (_currentUpdateState.isManualCheck ||
                !_currentUpdateState.manifest.packageUrl.empty()) {
                shouldShowBanner = true;
            }
            break;
        case selfupdate::UpdatePhase::Idle:
        default:
            shouldShowBanner = false;
            break;
        }
    }

    if (!_updateBannerWidget) {
        return;
    }

    _updateBannerWidget->setVisible(shouldShowBanner);
    if (!shouldShowBanner) {
        return;
    }

    int pct = 0;
    double frac = _currentUpdateState.progressFraction();
    if (frac >= 0.0) {
        pct = std::clamp(static_cast<int>(frac * 100.0), 0, 100);
    }

    switch (_currentUpdateState.phase) {
    case selfupdate::UpdatePhase::Available:
        _updateBannerLabel->setText(
            QString("OCTI Desktop v%1 is available!")
                .arg(QString::fromStdString(
                    _currentUpdateState.manifest.versionName)));
        _updateBannerProgressBar->setVisible(false);
        _updateActionBtn->setVisible(true);
        _updateActionBtn->setEnabled(true);
        _updateActionBtn->setText("Download && Update");
        break;

    case selfupdate::UpdatePhase::Downloading:
        _updateBannerLabel->setText(
            QString("Downloading OCTI update (%1%)...").arg(pct));
        _updateBannerProgressBar->setVisible(true);
        _updateBannerProgressBar->setValue(pct);
        _updateActionBtn->setVisible(true);
        _updateActionBtn->setEnabled(false);
        _updateActionBtn->setText("Downloading...");
        break;

    case selfupdate::UpdatePhase::ReadyToInstall:
        _updateBannerLabel->setText(
            QString("OCTI Desktop v%1 ready to install.")
                .arg(QString::fromStdString(
                    _currentUpdateState.manifest.versionName)));
        _updateBannerProgressBar->setVisible(false);
        _updateActionBtn->setVisible(true);
        _updateActionBtn->setEnabled(true);
        _updateActionBtn->setText("Install && Relaunch");
        break;

    case selfupdate::UpdatePhase::Installing:
        _updateBannerLabel->setText("Installing update...");
        _updateBannerProgressBar->setVisible(false);
        _updateActionBtn->setVisible(true);
        _updateActionBtn->setEnabled(false);
        _updateActionBtn->setText("Installing...");
        break;

    case selfupdate::UpdatePhase::Relaunching:
        _updateBannerLabel->setText("Relaunching OCTI Desktop...");
        _updateBannerProgressBar->setVisible(false);
        _updateActionBtn->setVisible(false);
        break;

    case selfupdate::UpdatePhase::UpToDate:
        _updateBannerLabel->setText(
            QString("OCTI Desktop v%1 is up to date.")
                .arg(QString::fromStdString(
                    _currentUpdateState.installedVersionName.empty()
                        ? OctiCore::versionString()
                        : _currentUpdateState.installedVersionName)));
        _updateBannerProgressBar->setVisible(false);
        _updateActionBtn->setVisible(true);
        _updateActionBtn->setEnabled(true);
        _updateActionBtn->setText("Reinstall");
        break;

    case selfupdate::UpdatePhase::Error:
        _updateBannerLabel->setText(
            QString("Update error: %1")
                .arg(QString::fromStdString(_currentUpdateState.errorMessage)));
        _updateBannerProgressBar->setVisible(false);
        if (!_currentUpdateState.manifest.packageUrl.empty()) {
            _updateActionBtn->setVisible(true);
            _updateActionBtn->setEnabled(true);
            _updateActionBtn->setText("Retry Update");
        } else {
            _updateActionBtn->setVisible(false);
        }
        break;

    case selfupdate::UpdatePhase::Idle:
    default:
        _updateBannerProgressBar->setVisible(false);
        _updateActionBtn->setVisible(false);
        break;
    }
}

void OctiMainWindow::showProtocolUpgradeRequired(const std::string &message)
{
    QString msg = message.empty()
        ? QStringLiteral(
              "OCTI update required: client protocol version is outdated. "
              "Please update OCTI.")
        : QString::fromStdString(message);
    if (_updateBannerWidget && _updateBannerLabel) {
        _updateBannerLabel->setText(msg);
        _updateBannerWidget->setVisible(true);
        if (_updateBannerProgressBar) {
            _updateBannerProgressBar->setVisible(false);
        }
        if (_updateActionBtn) {
            _updateActionBtn->setVisible(true);
            _updateActionBtn->setEnabled(true);
            _updateActionBtn->setText("Check for Updates");
        }
    }
    if (_serverStatusLabel) {
        _serverStatusLabel->setText("Upgrade Required (426)");
    }
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
