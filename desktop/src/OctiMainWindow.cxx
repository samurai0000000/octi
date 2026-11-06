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
#include <QInputDialog>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
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
      _lastGameChatSeq(0),
      _vcrTotalPlies(0),
      _brandLabel(nullptr),
      _serverStatusLabel(nullptr),
      _profileBadgeLabel(nullptr),
      _profileAvatarLabel(nullptr),
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
      _updateDismissBtn(nullptr),
      _startMatchOverlay(nullptr),
      _onlineTtlTimer(nullptr)
{
    selfupdate::UpdateInstaller::cleanupStaleBackup();
    _updateController = std::make_shared<selfupdate::SelfUpdateController>(
        defaultSelfUpdateConfig());
    bindSelfUpdateController();

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
            _profile.bio = prefs.bio;
            _profile.location = prefs.location;
            _authenticated = !prefs.playerUuid.empty();
        }
    }
    if (_configStore.preferences().installId.empty()) {
        _configStore.preferences().installId = ClientConfigStore::generateUuidV4();
        std::string saveErr;
        _configStore.save(saveErr);
    }
    if (_netClient) {
        _netClient->setInstallId(_configStore.preferences().installId);
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

    _profileAvatarLabel = new QLabel(topBar);
    _profileAvatarLabel->setFixedSize(28, 28);
    _profileAvatarLabel->setStyleSheet("border: none; background: transparent;");
    _profileAvatarLabel->setCursor(Qt::PointingHandCursor);
    _profileAvatarLabel->installEventFilter(this);
    topBarLayout->addWidget(_profileAvatarLabel);

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
    // Note: _updateBannerWidget is intentionally not added to rootLayout to keep
    // the tabletop board layout completely undisturbed above the board.

    /* ---- Center View Stack (Board Page vs Lobby View) ---- */
    _viewStack = new QStackedWidget(central);

    _boardPage = new QWidget(_viewStack);
    QHBoxLayout *boardLayout = new QHBoxLayout(_boardPage);
    boardLayout->setContentsMargins(0, 0, 0, 0);
    boardLayout->setSpacing(12);

    _reservePanel = new OctiReservePanel(_boardPage);
    _reservePanel->setMinimumWidth(260);
    _reservePanel->setMaximumWidth(330);

    _boardCanvas = new OctiBoardCanvas(_boardPage);
    _boardCanvas->setMinimumSize(500, 500);

    _startMatchOverlay = new OctiStartMatchDialog(_localConfig, _theme, _boardCanvas, true);
    _startMatchOverlay->setOnStartMatchCallback([this]() {
        confirmMatchStart();
    });
    _startMatchOverlay->setOnChangeTypeCallback([this]() {
        NewLocalGameDialog setupDlg(_theme, this);
        setupDlg.setConfig(_localConfig);
        if (setupDlg.exec() == QDialog::Accepted) {
            reconfigurePendingMatch(setupDlg.config());
        }
        updateStartMatchOverlay();
    });
    _startMatchOverlay->hide();

    _board.getGame()->start(PLAYER_A);
    _boardCanvas->attachBoard(&_board, &_composer, &_soundSynth);
    _reservePanel->attach(&_board, &_composer, _boardCanvas, &_soundSynth);

    _composer.setMoveCommittedCallback(
        [this](const std::string &moveNotation) {
            handleLocalOrOnlineMoveCommitted(moveNotation);
        });

    /* ---- Right Side Panel (Move History / VCR, Proposals, Chat) ---- */
    QWidget *rightSide = new QWidget(_boardPage);
    _rightSide = rightSide;
    rightSide->setMinimumWidth(320);
    rightSide->setMaximumWidth(380);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightSide);
    _rightLayout = rightLayout;
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    /* ---- Top Half: Status, Proposals, Move History, Actions ---- */
    _rightTopHalfWidget = new QWidget(rightSide);
    _rightTopHalfWidget->setObjectName("rightTopHalfWidget");
    QVBoxLayout *topHalfLayout = new QVBoxLayout(_rightTopHalfWidget);
    topHalfLayout->setContentsMargins(0, 0, 0, 0);
    topHalfLayout->setSpacing(4);

    /* 0. Match Status Card (PCE-D06 / PCE-D09) */
    QGroupBox *statusBox = new QGroupBox("Match Status", _rightTopHalfWidget);
    _statusBox = statusBox;
    QVBoxLayout *statusLayout = new QVBoxLayout(statusBox);
    statusLayout->setContentsMargins(6, 4, 6, 6);
    statusLayout->setSpacing(2);
    _matchModeLabel = new QLabel("Local vs AI Bot", statusBox);
    _matchModeLabel->setWordWrap(true);
    _matchTurnStatusLabel = new QLabel("Active Turn: Player A", statusBox);
    _matchTurnStatusLabel->setStyleSheet("font-size: 13px; font-weight: bold;");
    _matchTurnStatusLabel->setWordWrap(true);
    statusLayout->addWidget(_matchModeLabel);
    statusLayout->addWidget(_matchTurnStatusLabel);
    topHalfLayout->addWidget(statusBox);

    /* 0b. Online Waiting-Room Banner (shown when phase == WAITING_FOR_PLAYERS) */
    _waitingRoomBannerWidget = new QGroupBox("Waiting for Players", _rightTopHalfWidget);
    QVBoxLayout *waitLayout = new QVBoxLayout(_waitingRoomBannerWidget);
    waitLayout->setContentsMargins(6, 4, 6, 6);
    waitLayout->setSpacing(4);
    _waitingRoomBannerLabel = new QLabel(
        "Waiting for opponents to join this table...",
        _waitingRoomBannerWidget);
    _waitingRoomBannerLabel->setWordWrap(true);
    waitLayout->addWidget(_waitingRoomBannerLabel);
    QHBoxLayout *waitBtnRow = new QHBoxLayout();
    waitBtnRow->setSpacing(4);
    _sitSeatBtn = new QPushButton("Sit / Join Game", _waitingRoomBannerWidget);
    _sitSeatBtn->setStyleSheet("QPushButton { background-color: #2563eb; color: white; font-weight: bold; padding: 4px 6px; font-size: 12px; }");
    _deleteWaitingTableBtn = new QPushButton("Delete Table", _waitingRoomBannerWidget);
    _deleteWaitingTableBtn->setStyleSheet("QPushButton { background-color: #dc2626; color: white; font-weight: bold; padding: 4px 6px; font-size: 12px; }");
    _leaveWaitingTableBtn = new QPushButton("Leave Table", _waitingRoomBannerWidget);
    _leaveWaitingTableBtn->setStyleSheet("QPushButton { padding: 4px 6px; font-size: 12px; }");
    waitBtnRow->addWidget(_sitSeatBtn);
    waitBtnRow->addWidget(_deleteWaitingTableBtn);
    waitBtnRow->addWidget(_leaveWaitingTableBtn);
    waitLayout->addLayout(waitBtnRow);
    _waitingRoomBannerWidget->setVisible(false);
    topHalfLayout->addWidget(_waitingRoomBannerWidget);

    /* 1. Move History & VCR Replay */
    QGroupBox *historyBox = new QGroupBox("Move History • VCR Replay", _rightTopHalfWidget);
    _historyBox = historyBox;
    QVBoxLayout *histLayout = new QVBoxLayout(historyBox);
    histLayout->setContentsMargins(6, 4, 6, 6);
    histLayout->setSpacing(4);
    _moveHistoryList = new QListWidget(historyBox);
    _moveHistoryList->setMinimumHeight(68);
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

    const QString compactBtnCss = "QPushButton { padding: 4px 6px; font-size: 12px; }";
    QHBoxLayout *vcrRow = new QHBoxLayout();
    vcrRow->setSpacing(4);
    _vcrFirstBtn = new QPushButton("|<<", historyBox);
    _vcrFirstBtn->setToolTip("Jump to First Ply");
    _vcrFirstBtn->setStyleSheet(compactBtnCss);
    _vcrPrevBtn = new QPushButton("< Prev", historyBox);
    _vcrPrevBtn->setToolTip("Previous Ply");
    _vcrPrevBtn->setStyleSheet(compactBtnCss);
    _vcrNextBtn = new QPushButton("Next >", historyBox);
    _vcrNextBtn->setToolTip("Next Ply");
    _vcrNextBtn->setStyleSheet(compactBtnCss);
    _vcrLastBtn = new QPushButton(">>|", historyBox);
    _vcrLastBtn->setToolTip("Return to Live / Latest Ply");
    _vcrLastBtn->setStyleSheet(compactBtnCss);
    _exportNotationBtn = new QPushButton("Export", historyBox);
    _exportNotationBtn->setToolTip("Export Match Notation / FEN...");
    _exportNotationBtn->setStyleSheet(compactBtnCss);
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
    topHalfLayout->addWidget(historyBox, 1);

    /* 2. Active Proposal Banner & Game Action Buttons */
    _proposalBannerWidget = new QGroupBox("Pending Proposal", _rightTopHalfWidget);
    QVBoxLayout *propLayout = new QVBoxLayout(_proposalBannerWidget);
    propLayout->setContentsMargins(6, 4, 6, 6);
    propLayout->setSpacing(4);
    _proposalBannerLabel = new QLabel("No active proposal.", _proposalBannerWidget);
    _proposalBannerLabel->setWordWrap(true);
    propLayout->addWidget(_proposalBannerLabel);
    QHBoxLayout *propRespRow = new QHBoxLayout();
    propRespRow->setSpacing(4);
    _acceptProposalBtn = new QPushButton("Accept", _proposalBannerWidget);
    _acceptProposalBtn->setStyleSheet(compactBtnCss);
    _declineProposalBtn = new QPushButton("Decline", _proposalBannerWidget);
    _declineProposalBtn->setStyleSheet(compactBtnCss);
    propRespRow->addWidget(_acceptProposalBtn);
    propRespRow->addWidget(_declineProposalBtn);
    propLayout->addLayout(propRespRow);
    _proposalBannerWidget->setVisible(false);
    topHalfLayout->addWidget(_proposalBannerWidget);

    QGroupBox *actionsBox = new QGroupBox("Match Actions", _rightTopHalfWidget);
    _actionsBox = actionsBox;
    QVBoxLayout *actLayout = new QVBoxLayout(actionsBox);
    actLayout->setContentsMargins(6, 4, 6, 6);
    actLayout->setSpacing(4);
    QHBoxLayout *actRow1 = new QHBoxLayout();
    actRow1->setSpacing(4);
    _sidebarNewGameBtn = new QPushButton("New Game...", actionsBox);
    _sidebarNewGameBtn->setStyleSheet(compactBtnCss);
    _offerDrawBtn = new QPushButton("Offer Draw", actionsBox);
    _offerDrawBtn->setStyleSheet(compactBtnCss);
    _pauseResumeBtn = new QPushButton("Pause", actionsBox);
    _pauseResumeBtn->setStyleSheet(compactBtnCss);
    _claimForfeitBtn = new QPushButton("Forfeit", actionsBox);
    _claimForfeitBtn->setStyleSheet(compactBtnCss);
    _undoRequestBtn = new QPushButton("Undo", actionsBox);
    _undoRequestBtn->setStyleSheet(compactBtnCss);
    actRow1->addWidget(_sidebarNewGameBtn);
    actRow1->addWidget(_offerDrawBtn);
    actRow1->addWidget(_pauseResumeBtn);
    actRow1->addWidget(_claimForfeitBtn);
    actRow1->addWidget(_undoRequestBtn);
    actLayout->addLayout(actRow1);

    QHBoxLayout *actRow2 = new QHBoxLayout();
    actRow2->setSpacing(4);
    _resignBtn = new QPushButton("Resign", actionsBox);
    _resignBtn->setStyleSheet(compactBtnCss);
    _leaveTableBtn = new QPushButton("Leave Table", actionsBox);
    _leaveTableBtn->setStyleSheet(compactBtnCss);
    _moderationBtn = new QPushButton("Moderation", actionsBox);
    _moderationBtn->setStyleSheet(compactBtnCss);
    actRow2->addWidget(_resignBtn);
    actRow2->addWidget(_leaveTableBtn);
    actRow2->addWidget(_moderationBtn);
    actLayout->addLayout(actRow2);
    topHalfLayout->addWidget(actionsBox);

    rightLayout->addWidget(_rightTopHalfWidget, 3);

    /* ---- Bottom Half: Table Chat (Always Visible) & Integrated Voice Strip ---- */
    QGroupBox *chatBox = new QGroupBox("Table Chat", rightSide);
    chatBox->setObjectName("chatBox");
    _chatBox = chatBox;
    QVBoxLayout *chatLayout = new QVBoxLayout(chatBox);
    chatLayout->setContentsMargins(6, 4, 6, 6);
    chatLayout->setSpacing(4);

    // Header row inside chatBox: Zoom button
    QHBoxLayout *chatHeaderRow = new QHBoxLayout();
    chatHeaderRow->setSpacing(4);
    chatHeaderRow->addStretch(1);

    _zoomChatBtn = new QPushButton("Zoom ⤢", chatBox);
    _zoomChatBtn->setObjectName("zoomChatBtn");
    _zoomChatBtn->setStyleSheet(
        "QPushButton { background: #21262D; color: #C8A253; border: 1px solid #30363D; border-radius: 4px; padding: 2px 8px; font-size: 11px; font-weight: bold; }"
        "QPushButton:hover { background: #30363D; color: #F0F6FC; }"
    );
    chatHeaderRow->addWidget(_zoomChatBtn, 0, Qt::AlignRight);
    chatLayout->addLayout(chatHeaderRow);

    _gameChatTranscript = new QPlainTextEdit(chatBox);
    _gameChatTranscript->setReadOnly(true);
    _gameChatTranscript->setMinimumHeight(54);
    _gameChatTranscript->setMaximumBlockCount(200);
    _gameChatTranscript->viewport()->installEventFilter(this);
    chatLayout->addWidget(_gameChatTranscript, 1);

    QHBoxLayout *chatInputRow = new QHBoxLayout();
    chatInputRow->setSpacing(4);
    _chatScopeCombo = new QComboBox(chatBox);
    _chatScopeCombo->addItem("ALL", "ALL");
    _chatScopeCombo->addItem("TEAM", "TEAM");
    _gameChatInput = new QLineEdit(chatBox);
    _gameChatInput->setPlaceholderText("Send table message...");
    _sendGameChatBtn = new QPushButton("Send", chatBox);
    _sendGameChatBtn->setStyleSheet(compactBtnCss);
    chatInputRow->addWidget(_chatScopeCombo);
    chatInputRow->addWidget(_gameChatInput, 1);
    chatInputRow->addWidget(_sendGameChatBtn);
    chatLayout->addLayout(chatInputRow);

    rightLayout->addWidget(chatBox, 2);

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
        if (_composer.isAwaitingConfirmation() || _composer.isGuidedMoveActive()) {
            _composer.cancelAndRollback();
            if (_boardCanvas) {
                _boardCanvas->update();
            }
            if (_reservePanel) {
                _reservePanel->refresh();
            }
            refreshMoveHistoryAndFenUi();
            return;
        }
        if (_inOnlineRoom) {
            return;
        }
        if (_board.getGame() && _board.getGame()->canUndo()) {
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
    QObject::connect(_leaveTableBtn, &QPushButton::clicked, [this]() {
        leaveActiveOnlineTable(_onlineRoomId);
    });
    QObject::connect(_leaveWaitingTableBtn, &QPushButton::clicked, [this]() {
        leaveActiveOnlineTable(_onlineRoomId);
    });
    QObject::connect(_sitSeatBtn, &QPushButton::clicked, [this]() {
        if (!_onlineRoomId.empty()) {
            std::string code = _pendingOnlinePasscode;
            if (_onlineGameView.hasPasscode && code.empty()) {
                PasscodeDialog pd(_onlineRoomId, _theme, this);
                if (pd.exec() != QDialog::Accepted) {
                    return;
                }
                code = pd.passcode();
                _pendingOnlinePasscode = code;
            }
            int chosenSeat = 0;
            int totalSeats = (_onlineGameView.mode == "4P" ||
                              _onlineGameView.mode == "4P_FFA") ? 4 : 2;
            for (int s = 1; s <= totalSeats; ++s) {
                bool occupied = false;
                for (const auto &seatInfo : _onlineGameView.seats) {
                    if (seatInfo.seat == s) {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied) {
                    chosenSeat = s;
                    break;
                }
            }
            joinAndEnterOnlineTable(_onlineRoomId, chosenSeat, code);
        }
    });
    QObject::connect(_deleteWaitingTableBtn, &QPushButton::clicked, [this]() {
        if (_onlineRoomId.empty() || !_netClient) {
            return;
        }
        QString title = "Delete Table";
        auto reply = QMessageBox::warning(
            this,
            title,
            QString("Are you sure you want to delete table #%1?").arg(QString::fromStdString(_onlineRoomId)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            auto res = _netClient->purgeTable(_onlineRoomId);
            if (!res.ok) {
                QMessageBox::critical(this, QString("%1 Failed").arg(title), QString::fromStdString(res.errorMessage));
            } else {
                leaveOnlineRoom();
                showLobbyView();
            }
        }
    });
    QObject::connect(_moderationBtn, &QPushButton::clicked, [this]() {
        if (!_onlineRoomId.empty()) {
            RoomModerationDialog dlg(
                _netClient.get(), _onlineRoomId, _theme, this);
            dlg.exec();
            refreshOnlineGameView();
        }
    });

    QObject::connect(_zoomChatBtn, &QPushButton::clicked, [this]() {
        openBoardChatZoomDialog();
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

    _lobbyView->setViewerIdentity(_profile.uuid, _profile.handle);
    _lobbyView->setOnCreateTableRequested([this]() {
        CreateTableDialog dlg(_netClient.get(), _theme, this);
        if (dlg.exec() == QDialog::Accepted &&
            !dlg.createdTable().roomId.empty()) {
            _isLocalBotGame = false;
            _inOnlineRoom = true;
            _onlineRoomId = dlg.createdTable().roomId;
            _onlineViewerSeat = dlg.createdTable().seat;
            _onlineAutoOriented = false;
            _lastOrientedViewerSeat = 0;
            _lastAppliedGameEventSeq = 0;
            _appliedOnlineMoves.clear();
            refreshOnlineGameView();
            _netClient->startGameEventStream(
                _onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        handleGameSseEvent(ev);
                    });
                });
            if (_lobbyView) {
                _lobbyView->setActiveRoomState(
                    _inOnlineRoom, _onlineRoomId, _onlineViewerSeat, _onlineGameView);
                _lobbyView->refreshLobby();
            }
            showBoardView();
        }
    });
    _lobbyView->setOnJoinTableRequested(
        [this](const TableSummaryInfo &table) {
            std::string code = _pendingOnlinePasscode;
            if (table.hasPasscode && code.empty()) {
                PasscodeDialog pd(table.roomId, _theme, this);
                if (pd.exec() != QDialog::Accepted) {
                    return;
                }
                code = pd.passcode();
                _pendingOnlinePasscode = code;
            }
            observeOnlineTable(table.roomId, code);
        });
    _lobbyView->setOnLeaveTableRequested(
        [this](const std::string &roomId) {
            leaveActiveOnlineTable(roomId);
        });
    _lobbyView->setOnOpenActiveBoardRequested([this]() {
        if (!_inOnlineRoom && _lobbyView) {
            std::string targetRoomId = _lobbyView->activeBannerRoomId();
            if (targetRoomId.empty()) {
                targetRoomId = _lobbyView->selectedRoomId();
            }
            if (!targetRoomId.empty() && _netClient) {
                _inOnlineRoom = true;
                _onlineRoomId = targetRoomId;
                _onlineAutoOriented = false;
                _lastOrientedViewerSeat = 0;
                _lastAppliedGameEventSeq = 0;
                refreshOnlineGameView();
                _netClient->startGameEventStream(
                    _onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
                        _dispatcher.post([this, ev]() {
                            handleGameSseEvent(ev);
                        });
                    });
            }
        }
        showBoardView();
    });
    _lobbyView->setOnObserveTableRequested(
        [this](const TableSummaryInfo &table) {
            std::string code = _pendingOnlinePasscode;
            if (table.hasPasscode && code.empty()) {
                PasscodeDialog pd(table.roomId, _theme, this);
                if (pd.exec() != QDialog::Accepted) {
                    return;
                }
                code = pd.passcode();
                _pendingOnlinePasscode = code;
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
            _lastOrientedViewerSeat = 0;
            _lastAppliedGameEventSeq = 0;
            _appliedOnlineMoves.clear();
            refreshOnlineGameView();
            _netClient->startGameEventStream(
                _onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        handleGameSseEvent(ev);
                    });
                });
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
    updateMatchActionsVisibility();
    setupMenuBar();
    updateStartMatchOverlay();

    _onlineTtlTimer = new QTimer(this);
    _onlineTtlTimer->setInterval(100);
    QObject::connect(_onlineTtlTimer, &QTimer::timeout, this, [this]() {
        tickLocalClock(100);
    });
    _onlineTtlTimer->start();

    if (_authenticated) {
        QTimer::singleShot(0, this, [this]() {
            refreshAuthenticatedProfileFromServer();
        });
    }
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
    if (profile.avatarUrl != _profile.avatarUrl) {
        _cachedAvatarUrl.clear();
        _cachedAvatarImage = QImage();
    }
    _profile = profile;
    _authenticated = !profile.uuid.empty();
    _offlineMode = false;
    if (_netClient) {
        _netClient->setCurrentProfile(profile);
    }
    updateTopBarLabels();

    ClientPreferences &prefs = _configStore.preferences();
    prefs.playerUuid = profile.uuid;
    prefs.handle = profile.handle;
    prefs.displayName = profile.displayName;
    prefs.avatarUrl = profile.avatarUrl;
    prefs.bio = profile.bio;
    prefs.location = profile.location;
    std::string saveErr;
    _configStore.save(saveErr);

    if (_authenticated && _netClient) {
        if (_lobbyView) {
            _lobbyView->refreshLobby();
        }
        if (!_netClient->isLobbyEventStreamActive()) {
            std::int64_t sinceSeq = _lobbyView ? _lobbyView->latestLobbySeq() : 0;
            _netClient->startLobbyEventStream(
                sinceSeq, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        if (_lobbyView) {
                            _lobbyView->handleLobbySseEvent(ev);
                        }
                    });
                });
        }
    }
    if (_lobbyView) {
        _lobbyView->setViewerIdentity(_profile.uuid, _profile.handle);
        _lobbyView->setActiveRoomState(
            _inOnlineRoom, _onlineRoomId, _onlineViewerSeat, _onlineGameView);
        _lobbyView->updateAuthUiState();
    }
    updateMatchActionsVisibility();
}

void OctiMainWindow::refreshAuthenticatedProfileFromServer()
{
    if (!_authenticated || !_netClient || _configStore.preferences().sessionToken.empty()) {
        return;
    }
    auto res = _netClient->fetchMyProfile();
    if (res.ok && !res.data.uuid.empty()) {
        applyAuthenticatedProfile(res.data);
    } else if (!res.ok && res.httpStatus == 401) {
        _authenticated = false;
        _profile = PlayerProfileInfo();
        _netClient->clearSessionToken();
        _configStore.clearSession();
        std::string err;
        _configStore.save(err);
        if (_lobbyView) {
            _lobbyView->setViewerIdentity("", "");
            _lobbyView->updateAuthUiState();
        }
        updateTopBarLabels();
    }
}

void OctiMainWindow::showBoardView()
{
    if (_viewStack && _boardPage) {
        _viewStack->setCurrentWidget(_boardPage);
    }
    updateStartMatchOverlay();
}

void OctiMainWindow::showLobbyView()
{
    if (_viewStack && _lobbyView) {
        _lobbyView->setViewerIdentity(_profile.uuid, _profile.handle);
        _lobbyView->setActiveRoomState(
            _inOnlineRoom, _onlineRoomId, _onlineViewerSeat, _onlineGameView);
        _viewStack->setCurrentWidget(_lobbyView);
        _lobbyView->refreshLobby();
        if (!_offlineMode && _netClient && !_netClient->isLobbyEventStreamActive()) {
            std::int64_t sinceSeq = _lobbyView->latestLobbySeq();
            _netClient->startLobbyEventStream(sinceSeq, [this](const SseEvent &ev) {
                _dispatcher.post([this, ev]() {
                    if (_lobbyView) {
                        _lobbyView->handleLobbySseEvent(ev);
                    }
                });
            });
        }
    }
    updateStartMatchOverlay();
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
    syncSidePanelsToBoardEdges();
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
    syncSidePanelsToBoardEdges();
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
    updateStartMatchOverlay();
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

    _actNewGame = gameMenu->addAction(createMenuIcon(MenuIconKind::NEW_GAME, _theme), "New Local / Bot Game...");
    _actNewGame->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    QObject::connect(_actNewGame, &QAction::triggered, this, [this]() {
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

    _actUndo = gameMenu->addAction(createMenuIcon(MenuIconKind::UNDO_MOVE, _theme), "Undo Move");
    _actUndo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    _actUndo->setEnabled(!_inOnlineRoom);
    QObject::connect(_actUndo, &QAction::triggered, this, [this]() {
        if (_composer.isAwaitingConfirmation() || _composer.isGuidedMoveActive()) {
            _composer.cancelAndRollback();
            if (_boardCanvas) {
                _boardCanvas->update();
            }
            if (_reservePanel) {
                _reservePanel->refresh();
            }
            refreshMoveHistoryAndFenUi();
            return;
        }
        if (_inOnlineRoom) {
            return;
        }
        if (_board.getGame() && _board.getGame()->canUndo()) {
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
        dlg.setOnSignedInCallback([this](const PlayerProfileInfo &prof) {
            applyAuthenticatedProfile(prof);
        });
        if (dlg.exec() == QDialog::Accepted) {
            _authenticated = true;
            if (!dlg.loggedInProfile().uuid.empty()) {
                applyAuthenticatedProfile(dlg.loggedInProfile());
            }
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
        if (!dlg.currentProfile().uuid.empty()) {
            applyAuthenticatedProfile(dlg.currentProfile());
        }
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

    QAction *actRobots = onlineMenu->addAction("Robots Directory & Profiles...");
    actRobots->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    QObject::connect(actRobots, &QAction::triggered, this, [this]() {
        RobotsDirectoryDialog dlg(_netClient.get(), _theme, this);
        dlg.setOnLoadReplayCallback(
            [this](const std::string &, const std::string &transcript, const ArchivedGameSummary &) {
                loadOctiTranscriptIntoReplay(transcript);
                showBoardView();
            });
        dlg.refreshRobots();
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
    syncSidePanelsToBoardEdges();

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
        _boardCanvas->setInteractiveEnabled(false);
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
    updateStartMatchOverlay();
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
        _boardCanvas->setInteractiveEnabled(true);
        _boardCanvas->setTurnIndicatorVisible(true);
    }
    _soundSynth.triggerCue(SoundCue::POD_PLACE);
    updateTopBarLabels();
    updateStartMatchOverlay();
    if (_isLocalBotGame) {
        executeBotTurnIfNeeded();
    }
}

void OctiMainWindow::reconfigurePendingMatch(const LocalGameConfig &cfg)
{
    startNewLocalGame(cfg);
    beginAwaitingMatchStart();
    updateStartMatchOverlay();
}

void OctiMainWindow::promptStartMatchIfVisible()
{
    if (!_awaitingMatchStart) {
        beginAwaitingMatchStart();
    } else {
        updateStartMatchOverlay();
    }
}

void OctiMainWindow::syncSidePanelsToBoardEdges()
{
    if (_boardCanvas && _rightLayout) {
        if (_boardCanvas->width() > 0 && _boardCanvas->height() > 0) {
            _boardCanvas->syncGeometryToBoard(_boardCanvas->width(), _boardCanvas->height());
        }
        QRectF outer = _boardCanvas->geometry().boardOuterRect();
        if (outer.height() > 0 && _boardCanvas->height() > 0) {
            int topMargin = std::max(0, static_cast<int>(std::round(outer.top())));
            int bottomMargin = std::max(0, _boardCanvas->height() - static_cast<int>(std::round(outer.bottom())));
            _rightLayout->setContentsMargins(0, topMargin, 0, bottomMargin);
            _rightLayout->invalidate();
            if (_rightSide && _rightSide->layout()) {
                _rightSide->layout()->activate();
            }
        }
    }
    if (_reservePanel) {
        _reservePanel->update();
    }
    updateStartMatchOverlay();
}

void OctiMainWindow::updateStartMatchOverlay()
{
    if (!_startMatchOverlay || !_boardCanvas) {
        return;
    }
    _startMatchOverlay->updateConfigAndTheme(_localConfig, _theme);
    _startMatchOverlay->positionCenterOfBoard(this, _boardCanvas);
    const bool shouldShow = _awaitingMatchStart && !_inOnlineRoom &&
                            isBoardViewVisible();
    if (shouldShow) {
        _startMatchOverlay->show();
        _startMatchOverlay->raise();
    } else {
        _startMatchOverlay->hide();
    }
}

QRect OctiMainWindow::rightPanelTopWidgetGeometry() const
{
    if (_rightSide && _statusBox) {
        QPoint pt = _statusBox->mapTo(_rightSide, QPoint(0, 0));
        return QRect(pt, _statusBox->size());
    }
    return _statusBox ? _statusBox->geometry() : QRect();
}

QRect OctiMainWindow::rightPanelBottomWidgetGeometry() const
{
    if (_chatBox && !_chatBox->isHidden()) {
        return _chatBox->geometry();
    }
    if (_actionsBox) {
        return _actionsBox->geometry();
    }
    return QRect();
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
                            parsedBotMove, game->useEdgelessBoard(),
                            _board.getBoardWidth(), _board.getBoardHeight()));
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

void OctiMainWindow::showHint()
{
    if (!_board.getGame() || _board.getGame()->isGameOver()) {
        return;
    }
    BotSearchStats stats;
    std::string bestMoveStr = _botEngine.chooseMove(_board, &stats);
    if (bestMoveStr.empty()) {
        return;
    }
    try {
        OctiMove move(bestMoveStr);
        if (move.getOps().empty()) {
            return;
        }
        const OctiMoveOp &op = move.getOps().front();
        if (op.addPod) {
            _composer.stageOrCommitAddPod(op.x0 + 1, op.y0 + 1, op.liberate);
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir == DIR_UNDEF) {
            int podIdx = -1;
            const OctiSquare &sq = _board.squareAt(op.x0, op.y0);
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (sq.getPodAt(i) == op.pod) {
                    podIdx = i;
                    break;
                }
            }
            if (podIdx < 0) {
                podIdx = 0;
            }
            _composer.stageOrCommitAddProng(op.x0 + 1, op.y0 + 1, podIdx, op.addProngDir, op.superProng);
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir != DIR_UNDEF) {
            int podIdx = -1;
            const OctiSquare &sq = _board.squareAt(op.x0, op.y0);
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (sq.getPodAt(i) == op.pod) {
                    podIdx = i;
                    break;
                }
            }
            if (podIdx < 0) {
                podIdx = 0;
            }
            _composer.stageOrCommitRepositionProng(op.x0 + 1, op.y0 + 1, podIdx, op.rmProngDir, op.addProngDir);
        } else if (!op.hops.empty()) {
            int podIdx = -1;
            const OctiSquare &sq = _board.squareAt(op.x0, op.y0);
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (sq.getPodAt(i) == op.pod) {
                    podIdx = i;
                    break;
                }
            }
            if (podIdx < 0) {
                podIdx = 0;
            }
            _composer.beginGuidedPodMove(op.x0 + 1, op.y0 + 1, podIdx, false);
            for (const auto &hop : op.hops) {
                _composer.selectGuidedDestination(hop.x + 1, hop.y + 1);
            }
        }
        if (_boardCanvas) {
            _boardCanvas->update();
        }
    } catch (...) {
    }
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
    res = _netClient->createTable(normalized);
    if (!res.ok) {
        if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
            showProtocolUpgradeRequired(res.errorMessage);
        }
        return res;
    }

    leaveOnlineRoom();
    _isLocalBotGame = false;
    _awaitingMatchStart = false;
    _gameOverDialogShownForCurrentMatch = false;
    updateStartMatchOverlay();
    _inOnlineRoom = true;
    _onlineRoomId = res.data.roomId;
    if (!req.passcode.empty()) {
        _pendingOnlinePasscode = req.passcode;
    }
    _onlineViewerSeat = res.data.seat;
    _onlineAutoOriented = false;
    _lastOrientedViewerSeat = 0;
    _lastAppliedGameEventSeq = 0;
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    _lastGameChatSeq = 0;
    if (_gameChatTranscript) {
        _gameChatTranscript->clear();
    }

    refreshOnlineGameView();
    _netClient->startGameEventStream(
        _onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
            _dispatcher.post([this, ev]() {
                handleGameSseEvent(ev);
            });
        });

    showBoardView();
    updateMatchActionsVisibility();
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
    _awaitingMatchStart = false;
    _gameOverDialogShownForCurrentMatch = false;
    updateStartMatchOverlay();
    _inOnlineRoom = true;
    _onlineRoomId = res.data.roomId;
    if (!passcode.empty()) {
        _pendingOnlinePasscode = passcode;
    }
    _onlineViewerSeat = res.data.seat;
    _onlineAutoOriented = false;
    _lastOrientedViewerSeat = 0;
    _lastAppliedGameEventSeq = 0;
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    _lastGameChatSeq = 0;
    if (_gameChatTranscript) {
        _gameChatTranscript->clear();
    }

    refreshOnlineGameView();
    _netClient->startGameEventStream(
        _onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
            _dispatcher.post([this, ev]() {
                handleGameSseEvent(ev);
            });
        });

    showBoardView();
    updateMatchActionsVisibility();
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
    _awaitingMatchStart = false;
    _gameOverDialogShownForCurrentMatch = false;
    updateStartMatchOverlay();
    _inOnlineRoom = true;
    _onlineRoomId = roomId;
    if (!passcode.empty()) {
        _pendingOnlinePasscode = passcode;
    }
    _onlineViewerSeat = 0;
    _onlineAutoOriented = false;
    _lastOrientedViewerSeat = 0;
    _lastAppliedGameEventSeq = 0;
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    _lastGameChatSeq = 0;
    if (_gameChatTranscript) {
        _gameChatTranscript->clear();
    }

    refreshOnlineGameView();
    _netClient->startGameEventStream(
        _onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
            _dispatcher.post([this, ev]() {
                handleGameSseEvent(ev);
            });
        });

    showBoardView();
    updateMatchActionsVisibility();
    return res;
}

ApiResult<bool> OctiMainWindow::leaveActiveOnlineTable(
    const std::string &roomId)
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    std::string targetRoomId = !roomId.empty() ? roomId : _onlineRoomId;
    if (targetRoomId.empty() && _lobbyView) {
        targetRoomId = _lobbyView->activeBannerRoomId();
        if (targetRoomId.empty()) {
            targetRoomId = _lobbyView->selectedRoomId();
        }
    }
    if (targetRoomId.empty()) {
        res.errorMessage = "No active table to leave";
        return res;
    }

    if (_inOnlineRoom && targetRoomId == _onlineRoomId) {
        bool isHost = (!_profile.uuid.empty() && !_onlineGameView.hostUuid.empty() &&
                       _onlineGameView.hostUuid == _profile.uuid);
        if (_onlineViewerSeat > 0 && _onlineGameView.phase != "IN_PROGRESS" &&
            (!isHost || _onlineGameView.phase == "FINISHED")) {
            res = _netClient->leaveTable(targetRoomId);
            if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
                showProtocolUpgradeRequired(res.errorMessage);
            }
        } else if (_onlineViewerSeat > 0) {
            // Seated player exiting table view: do not vacate/forfeit so seat can be resumed from lobby
            res.ok = true;
            res.data = true;
        } else {
            res = _netClient->unobserveTable(targetRoomId);
            if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
                showProtocolUpgradeRequired(res.errorMessage);
            }
        }
    } else {
        res = _netClient->leaveTable(targetRoomId);
        if (res.errorCode == "UPGRADE_REQUIRED" || res.httpStatus == 426) {
            showProtocolUpgradeRequired(res.errorMessage);
        }
    }

    if (targetRoomId == _onlineRoomId || _inOnlineRoom) {
        leaveOnlineRoom();
    }
    if (_lobbyView) {
        _lobbyView->setActiveRoomState(
            _inOnlineRoom, _onlineRoomId, _onlineViewerSeat, _onlineGameView);
        _lobbyView->refreshLobby();
    }
    showLobbyView();
    return res;
}

void OctiMainWindow::leaveOnlineRoom()
{
    if (_netClient) {
        _netClient->stopGameEventStream();
    }
    _inOnlineRoom = false;
    _onlineRoomId.clear();
    _pendingOnlinePasscode.clear();
    _onlineViewerSeat = 0;
    _onlineAutoOriented = false;
    _lastOrientedViewerSeat = 0;
    _onlineGameView = GameViewData();
    _appliedOnlineMoves.clear();
    _gameChatLines.clear();
    _lastGameChatSeq = 0;
    _lastAppliedGameEventSeq = 0;
    _gameOverDialogShownForCurrentMatch = false;
    if (_boardCanvas) {
        _boardCanvas->setInteractiveEnabled(!_awaitingMatchStart);
        _boardCanvas->setTurnIndicatorVisible(!_awaitingMatchStart);
    }
    if (_reservePanel) {
        _reservePanel->setTimeControlEnabled(true);
        _reservePanel->clearSeatMetadata();
        _reservePanel->setOnlineWaitingSeatState(
            false, 0, {false, false, false, false, false}, nullptr);
        _reservePanel->refresh();
    }
    if (_lobbyView) {
        _lobbyView->setActiveRoomState(
            _inOnlineRoom, _onlineRoomId, _onlineViewerSeat, _onlineGameView);
    }
    updateProposalBannerUi();
    updateMatchActionsVisibility();
    setupMenuBar();
    updateStartMatchOverlay();
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

bool OctiMainWindow::isInteractiveOnlineTurn() const
{
    if (!_inOnlineRoom) {
        return !_awaitingMatchStart;
    }
    if (_onlineGameView.phase != "IN_PROGRESS" || _onlineGameView.paused) {
        return false;
    }
    if (_onlineViewerSeat < 1 || _onlineViewerSeat > 4) {
        return false;
    }
    if (_onlineGameView.turnSeat == _onlineViewerSeat) {
        return true;
    }
    if (_board.getGame() &&
        static_cast<int>(_board.getGame()->getTurn()) == _onlineViewerSeat) {
        return true;
    }
    return false;
}

const GameViewData &OctiMainWindow::currentOnlineGameView() const
{
    return _onlineGameView;
}

bool OctiMainWindow::isWaitingRoomBannerVisible() const
{
    return _waitingRoomBannerWidget != nullptr &&
           !_waitingRoomBannerWidget->isHidden();
}

QString OctiMainWindow::waitingRoomBannerText() const
{
    return _waitingRoomBannerLabel ? _waitingRoomBannerLabel->text() : QString();
}

void OctiMainWindow::tickLocalClock(int elapsedMs)
{
    if (_inOnlineRoom && !_onlineRoomId.empty()) {
        if (_onlineGameView.phase == "IN_PROGRESS" && !_onlineGameView.paused &&
            _onlineGameView.timeControlEnabled) {
            OctiPlayer turn = _board.getGame() ? _board.getGame()->getTurn() : PLAYER_UNDEF;
            if (turn >= PLAYER_A && turn <= PLAYER_D) {
                long curMs = _board.getGame()->getTimeRemaining(turn);
                long newMs = std::max(0L, curMs - static_cast<long>(elapsedMs));
                _board.getGame()->setTimeRemaining(turn, newMs);
                for (auto &s : _onlineGameView.seats) {
                    if (s.seat == static_cast<int>(turn)) {
                        s.clockMs = std::max<int64_t>(0, s.clockMs - elapsedMs);
                        break;
                    }
                }
                if (_reservePanel) {
                    _reservePanel->update();
                }
            }
        } else if (_onlineGameView.phase == "WAITING_FOR_PLAYERS" && _onlineGameView.ttlRemainingMs > 0) {
            _onlineGameView.ttlRemainingMs = std::max<int64_t>(0, _onlineGameView.ttlRemainingMs - elapsedMs);
            updateTopBarLabels();
            updateMatchActionsVisibility();
        }
    }
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
    if (!view.rematchRoomId.empty() && view.rematchRoomId != _onlineRoomId && !_onlineRoomId.empty()) {
        if (_netClient) {
            _netClient->stopGameEventStream();
        }
        _onlineRoomId = view.rematchRoomId;
        _onlineAutoOriented = false;
        _lastOrientedViewerSeat = 0;
        _lastAppliedGameEventSeq = 0;
        _appliedOnlineMoves.clear();
        _gameChatLines.clear();
        _lastGameChatSeq = 0;
        if (_gameChatTranscript) {
            _gameChatTranscript->clear();
        }
        _gameOverDialogShownForCurrentMatch = false;
        refreshOnlineGameView();
        if (_netClient) {
            _netClient->startGameEventStream(_onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
                _dispatcher.post([this, ev]() {
                    handleGameSseEvent(ev);
                });
            });
        }
        return;
    }

    _awaitingMatchStart = false;
    if (!view.roomId.empty()) {
        _inOnlineRoom = true;
        if (_onlineRoomId.empty()) {
            _onlineRoomId = view.roomId;
        }
    }
    updateStartMatchOverlay();
    if (view.phase != "FINISHED") {
        _gameOverDialogShownForCurrentMatch = false;
    }

    bool prevPaused = _onlineGameView.paused;
    std::string prevPhase = _onlineGameView.phase;
    std::size_t prevMovesCount = _appliedOnlineMoves.size();
    _onlineGameView = view;
    if (view.seq > _lastAppliedGameEventSeq) {
        _lastAppliedGameEventSeq = view.seq;
    }

    if (view.viewerSeat > 0) {
        _onlineViewerSeat = view.viewerSeat;
    } else if (!view.seats.empty() && !_profile.uuid.empty()) {
        _onlineViewerSeat = 0;
        for (const auto &s : view.seats) {
            if (s.playerUuid == _profile.uuid) {
                _onlineViewerSeat = s.seat;
                break;
            }
        }
    } else {
        _onlineViewerSeat = 0;
    }

    OctiGameMode targetMode = GAME_MODE_2;
    if (view.mode == "4P") {
        targetMode = GAME_MODE_4;
    } else if (view.mode == "OFK") {
        targetMode = GAME_MODE_OFK;
    }
    OctiGameBases targetBases =
        (view.bases == 1) ? GAME_BASES_1 : GAME_BASES_3;

    if (_onlineViewerSeat >= 1 && _onlineViewerSeat <= 4 &&
        (!_onlineAutoOriented || _lastOrientedViewerSeat != _onlineViewerSeat) &&
        _boardCanvas) {
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
        _lastOrientedViewerSeat = _onlineViewerSeat;
        syncSidePanelsToBoardEdges();
    }

    OctiGame *g = _board.getGame();
    bool needFullRebuild =
        !g || _board.getGameMode() != targetMode ||
        g->getBases() != targetBases ||
        g->useSuperProngs() != view.superProngs ||
        g->useEdgelessBoard() != view.edgeless ||
        g->getNumMoves() != static_cast<int>(_appliedOnlineMoves.size()) ||
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

    bool boardChanged = needFullRebuild ||
                        (_appliedOnlineMoves.size() != prevMovesCount) ||
                        (view.phase != prevPhase) ||
                        (view.paused != prevPaused);

    if (boardChanged || !_composer.hasStagedMove()) {
        _composer.attachBoard(&_board);
        if (_boardCanvas) {
            _boardCanvas->attachBoard(&_board, &_composer, &_soundSynth);
            if (!latestOnlineMoveStr.empty()) {
                try {
                    OctiMove parsedOnlineMove(latestOnlineMoveStr);
                    _boardCanvas->setLastMoveModifiedSquares(
                        OctiBoardCanvas::extractModifiedSquares(
                            parsedOnlineMove, view.edgeless,
                            _board.getBoardWidth(), _board.getBoardHeight()));
                } catch (...) {
                }
            }
        }
    }

    if (_boardCanvas) {
        bool isInProg = (view.phase == "IN_PROGRESS") &&
                        !(_board.getGame() && _board.getGame()->isGameOver()) &&
                        !(_board.getGame() && _board.getGame()->canRedo());
        _boardCanvas->setInteractiveEnabled(isInteractiveOnlineTurn());
        _boardCanvas->setTurnIndicatorVisible(isInProg);
        _boardCanvas->update();
    }

    if (_reservePanel) {
        _reservePanel->setTimeControlEnabled(view.timeControlEnabled);
        _reservePanel->clearSeatMetadata();
        std::array<bool, 5> seatOcc = {false, false, false, false, false};
        for (const auto &s : view.seats) {
            _reservePanel->setSeatMetadata(
                s.seat, s.handle, s.rating, s.isRobot, s.connected, s.eliminated);
            if (s.seat >= 1 && s.seat <= 4 &&
                (!s.playerUuid.empty() || !s.handle.empty() || s.isRobot)) {
                seatOcc[s.seat] = true;
            }
        }
        _reservePanel->setOnlineWaitingSeatState(
            view.phase == "WAITING_FOR_PLAYERS",
            _onlineViewerSeat,
            seatOcc,
            [this](int seat) {
                if (!_onlineRoomId.empty()) {
                    std::string code = _pendingOnlinePasscode;
                    if (_onlineGameView.hasPasscode && code.empty()) {
                        PasscodeDialog pd(_onlineRoomId, _theme, this);
                        if (pd.exec() != QDialog::Accepted) {
                            return;
                        }
                        code = pd.passcode();
                        _pendingOnlinePasscode = code;
                    }
                    joinAndEnterOnlineTable(_onlineRoomId, seat, code);
                }
            });
        if (boardChanged || !_composer.hasStagedMove()) {
            _reservePanel->attach(&_board, &_composer, _boardCanvas, &_soundSynth);
        }
        _reservePanel->refresh();
    }

    if (_lobbyView) {
        _lobbyView->setActiveRoomState(
            _inOnlineRoom, _onlineRoomId, _onlineViewerSeat, _onlineGameView);
    }

    refreshMoveHistoryAndFenUi();
    updateProposalBannerUi();
    updateMatchActionsVisibility();
    checkAndPresentGameOver();
    setupMenuBar();
}

void OctiMainWindow::handleGameSseEvent(const SseEvent &ev)
{
    std::int64_t evId = static_cast<std::int64_t>(ev.id);
    if (evId > 0) {
        if (evId <= _lastAppliedGameEventSeq) {
            return;
        }
        _lastAppliedGameEventSeq = evId;
    }

    Json j = Json::parse(ev.dataJson, nullptr, false);
    if (ev.event == "CHAT" && j.is_object()) {
        std::int64_t seq = (ev.id > 0) ? ev.id : j.value("seq", 0LL);
        std::string scope = j.value("scope", "ALL");
        std::string handle = j.value("handle", "Player");
        std::string text = j.value("text", "");
        std::string playerUuid = j.value("player_uuid", "");
        std::string line = "[" + scope + "] " + handle + ": " + text;
        if (seq > 0 && seq == _lastGameChatSeq) {
            return;
        }
        if (!playerUuid.empty() && playerUuid == _profile.uuid &&
            !_gameChatLines.empty() && _gameChatLines.back() == line) {
            return;
        }
        _gameChatLines.push_back(line);
        if (_gameChatTranscript) {
            _gameChatTranscript->appendPlainText(
                QString::fromStdString(line));
            _gameChatTranscript->verticalScrollBar()->setValue(
                _gameChatTranscript->verticalScrollBar()->maximum());
        }
        if (_boardChatZoomDialog) {
            QPlainTextEdit *zt = _boardChatZoomDialog->findChild<QPlainTextEdit *>("boardChatZoomTranscript");
            if (zt) {
                zt->appendPlainText(QString::fromStdString(line));
                zt->verticalScrollBar()->setValue(zt->verticalScrollBar()->maximum());
            }
        }
        return;
    }

    if (ev.event == "REMATCH_CREATED" && j.is_object()) {
        std::string newRoomId = j.value("room_id", "");
        if (!newRoomId.empty() && newRoomId != _onlineRoomId && !_onlineRoomId.empty()) {
            if (_netClient) {
                _netClient->stopGameEventStream();
            }
            _onlineRoomId = newRoomId;
            _onlineAutoOriented = false;
            _lastOrientedViewerSeat = 0;
            _lastAppliedGameEventSeq = 0;
            _appliedOnlineMoves.clear();
            _gameChatLines.clear();
            _lastGameChatSeq = 0;
            if (_gameChatTranscript) {
                _gameChatTranscript->clear();
            }
            _gameOverDialogShownForCurrentMatch = false;
            refreshOnlineGameView();
            if (_netClient) {
                _netClient->startGameEventStream(_onlineRoomId, _onlineGameView.seq, [this](const SseEvent &ev) {
                    _dispatcher.post([this, ev]() {
                        handleGameSseEvent(ev);
                    });
                });
            }
            return;
        }
    }

    if (ev.event == "TABLE_CLOSED") {
        leaveOnlineRoom();
        if (_lobbyView) {
            _lobbyView->refreshLobby();
        }
        showLobbyView();
        return;
    }

    std::string sysMsg;
    if (ev.event == "SEAT_JOINED") {
        std::string h = j.value("handle", "Player");
        int s = j.value("seat", 0);
        sysMsg = "[System] @" + h + " joined Seat " + std::to_string(s);
    } else if (ev.event == "SEAT_LEFT") {
        std::string h = j.value("handle", "Player");
        int s = j.value("seat", 0);
        sysMsg = "[System] @" + h + " left Seat " + std::to_string(s);
    } else if (ev.event == "OBSERVER_JOINED") {
        std::string h = j.value("handle", "Spectator");
        sysMsg = "[System] @" + h + " joined as observer";
    } else if (ev.event == "OBSERVER_LEFT") {
        std::string h = j.value("handle", "Spectator");
        sysMsg = "[System] @" + h + " left observer";
    } else if (ev.event == "GAME_STARTED") {
        sysMsg = "[System] Game started";
    } else if (ev.event == "PROPOSAL_OFFERED") {
        std::string h = j.value("handle", "Player");
        std::string prop = j.value("proposal", "");
        sysMsg = "[System] @" + h + " offered " + prop;
    } else if (ev.event == "PROPOSAL_ACCEPTED") {
        std::string prop = j.value("proposal", "");
        sysMsg = "[System] Proposal accepted: " + prop;
    } else if (ev.event == "PROPOSAL_DECLINED") {
        std::string prop = j.value("proposal", "");
        sysMsg = "[System] Proposal declined: " + prop;
    } else if (ev.event == "UNDO_APPLIED") {
        sysMsg = "[System] Move undone";
    } else if (ev.event == "GAME_PAUSED") {
        sysMsg = "[System] Game paused";
    } else if (ev.event == "GAME_RESUMED") {
        sysMsg = "[System] Game resumed";
    } else if (ev.event == "PLAYER_DISCONNECTED") {
        std::string h = j.value("handle", "Player");
        sysMsg = "[System] @" + h + " disconnected";
    } else if (ev.event == "PLAYER_RECONNECTED") {
        std::string h = j.value("handle", "Player");
        sysMsg = "[System] @" + h + " reconnected";
    } else if (ev.event == "GAME_OVER") {
        sysMsg = "[System] Game over";
    }

    if (!sysMsg.empty()) {
        _gameChatLines.push_back(sysMsg);
        if (_gameChatTranscript) {
            _gameChatTranscript->appendPlainText(QString::fromStdString(sysMsg));
            _gameChatTranscript->verticalScrollBar()->setValue(
                _gameChatTranscript->verticalScrollBar()->maximum());
        }
        if (_boardChatZoomDialog) {
            QPlainTextEdit *zt = _boardChatZoomDialog->findChild<QPlainTextEdit *>("boardChatZoomTranscript");
            if (zt) {
                zt->appendPlainText(QString::fromStdString(sysMsg));
                zt->verticalScrollBar()->setValue(zt->verticalScrollBar()->maximum());
            }
        }
    }

    if (ev.event == "MOVE" || ev.event == "UNDO_APPLIED") {
        _onlineGameView.ttlRemainingMs = 3600000;
    }

    std::string type = j.is_object() ? j.value("type", "") : "";
    if (ev.event == "MOVE" || ev.event == "GAME_STARTED" ||
        ev.event == "PROPOSAL_OFFERED" || ev.event == "PROPOSAL_ACCEPTED" ||
        ev.event == "PROPOSAL_DECLINED" || ev.event == "PROPOSAL_EXPIRED" ||
        ev.event == "GAME_PAUSED" || ev.event == "GAME_RESUMED" ||
        ev.event == "UNDO_APPLIED" || ev.event == "REMATCH_CREATED" ||
        ev.event == "GAME_OVER" || ev.event == "POLICY_UPDATED" ||
        ev.event == "SEAT_JOINED" || ev.event == "SEAT_LEFT" ||
        ev.event == "OBSERVER_JOINED" || ev.event == "OBSERVER_LEFT" ||
        ev.event == "OBSERVER_BANNED" ||
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
    if (_awaitingMatchStart && !_inOnlineRoom) {
        return;
    }

    if (_boardCanvas && !moveNotation.empty()) {
        try {
            OctiMove parsedMove(moveNotation);
            bool edgeless = _board.getGame() && _board.getGame()->useEdgelessBoard();
            _boardCanvas->setLastMoveModifiedSquares(
                OctiBoardCanvas::extractModifiedSquares(
                    parsedMove, edgeless,
                    _board.getBoardWidth(), _board.getBoardHeight()));
        } catch (...) {
        }
    }

    if (_inOnlineRoom && !_onlineRoomId.empty() && _netClient) {
        if (!isInteractiveOnlineTurn()) {
            _appliedOnlineMoves.clear();
            refreshOnlineGameView();
            return;
        }
        auto res = _netClient->submitMove(_onlineRoomId, moveNotation);
        if (res.ok) {
            std::string appliedMove = res.data.move.empty() ? moveNotation : res.data.move;
            _appliedOnlineMoves.push_back(appliedMove);
        } else {
            if (_matchTurnStatusLabel) {
                _matchTurnStatusLabel->setText(
                    QString::fromStdString(res.errorMessage.empty() ? "Move rejected" : res.errorMessage));
            }
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

void OctiMainWindow::openBoardChatZoomDialog()
{
    if (_boardChatZoomDialog) {
        _boardChatZoomDialog->show();
        _boardChatZoomDialog->raise();
        _boardChatZoomDialog->activateWindow();
        return;
    }

    QDialog *dlg = new QDialog(this, Qt::Dialog | Qt::FramelessWindowHint);
    dlg->setObjectName("boardChatZoomDialog");
    dlg->setMinimumSize(540, 420);
    dlg->resize(560, 440);
    dlg->setStyleSheet(
        "QDialog#boardChatZoomDialog {"
        "  background-color: #161D26;"
        "  border: 2px solid #C8A253;"
        "  border-radius: 8px;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // Header bar
    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("Table Chat", dlg);
    titleLabel->setStyleSheet("color: #E6EDF3; font-size: 14px; font-weight: bold;");
    headerRow->addWidget(titleLabel);

    headerRow->addStretch();

    QPushButton *btnClose = new QPushButton("Close [Esc]", dlg);
    btnClose->setStyleSheet("QPushButton { background: #30363D; color: #F0F6FC; border-radius: 4px; padding: 2px 8px; font-size: 11px; }"
                            "QPushButton:hover { background: #8B0000; }");

    headerRow->addWidget(btnClose);
    layout->addLayout(headerRow);

    // Transcript
    QPlainTextEdit *zoomTranscript = new QPlainTextEdit(dlg);
    zoomTranscript->setObjectName("boardChatZoomTranscript");
    zoomTranscript->setReadOnly(true);
    zoomTranscript->setMaximumBlockCount(500);
    for (const auto &line : _gameChatLines) {
        zoomTranscript->appendPlainText(QString::fromStdString(line));
    }
    zoomTranscript->verticalScrollBar()->setValue(zoomTranscript->verticalScrollBar()->maximum());
    layout->addWidget(zoomTranscript, 1);

    // Input row
    QHBoxLayout *inputRow = new QHBoxLayout();
    QComboBox *scopeCombo = new QComboBox(dlg);
    scopeCombo->setObjectName("boardChatZoomScopeCombo");
    scopeCombo->addItem("ALL", "ALL");
    scopeCombo->addItem("TEAM", "TEAM");

    QLineEdit *inputEdit = new QLineEdit(dlg);
    inputEdit->setObjectName("boardChatZoomInput");
    inputEdit->setPlaceholderText("Type a table message...");

    QPushButton *sendBtn = new QPushButton("Send", dlg);
    sendBtn->setObjectName("boardChatZoomSendBtn");

    inputRow->addWidget(scopeCombo);
    inputRow->addWidget(inputEdit, 1);
    inputRow->addWidget(sendBtn);
    layout->addLayout(inputRow);

    auto doSend = [this, scopeCombo, inputEdit, zoomTranscript]() {
        QString text = inputEdit->text().trimmed();
        if (text.isEmpty()) {
            return;
        }
        std::string scope = scopeCombo->currentText().toStdString();
        sendGameChatMessage(scope, text.toStdString());
        inputEdit->clear();
        zoomTranscript->clear();
        for (const auto &line : _gameChatLines) {
            zoomTranscript->appendPlainText(QString::fromStdString(line));
        }
        zoomTranscript->verticalScrollBar()->setValue(zoomTranscript->verticalScrollBar()->maximum());
    };

    QObject::connect(sendBtn, &QPushButton::clicked, dlg, doSend);
    QObject::connect(inputEdit, &QLineEdit::returnPressed, dlg, doSend);
    QObject::connect(btnClose, &QPushButton::clicked, dlg, &QDialog::close);

    _boardChatZoomDialog = dlg;
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    inputEdit->setFocus();
}

bool OctiMainWindow::sendGameChatMessage(const std::string &scope,
                                         const std::string &text)
{
    if (text.empty()) {
        return false;
    }
    if (_inOnlineRoom && !_onlineRoomId.empty() && _netClient) {
        if (!isAuthenticated()) {
            return false;
        }
        auto res = _netClient->postGameChat(_onlineRoomId, scope, text);
        if (!res.ok) {
            return false;
        }
        _lastGameChatSeq = res.data;
        std::string handle =
            _profile.handle.empty() ? "Me" : _profile.handle;
        std::string line = "[" + scope + "] " + handle + ": " + text;
        if (_gameChatLines.empty() || _gameChatLines.back() != line) {
            _gameChatLines.push_back(line);
            if (_gameChatTranscript) {
                _gameChatTranscript->appendPlainText(
                    QString::fromStdString(line));
                _gameChatTranscript->verticalScrollBar()->setValue(
                    _gameChatTranscript->verticalScrollBar()->maximum());
            }
            if (_boardChatZoomDialog) {
                QPlainTextEdit *zt = _boardChatZoomDialog->findChild<QPlainTextEdit *>("boardChatZoomTranscript");
                if (zt) {
                    zt->appendPlainText(QString::fromStdString(line));
                    zt->verticalScrollBar()->setValue(zt->verticalScrollBar()->maximum());
                }
            }
        }
        return true;
    }

    std::string line = "[LOCAL] Player: " + text;
    _gameChatLines.push_back(line);
    if (_gameChatTranscript) {
        _gameChatTranscript->appendPlainText(QString::fromStdString(line));
        _gameChatTranscript->verticalScrollBar()->setValue(
            _gameChatTranscript->verticalScrollBar()->maximum());
    }

    if (_boardChatZoomDialog) {
        QPlainTextEdit *zt = _boardChatZoomDialog->findChild<QPlainTextEdit *>("boardChatZoomTranscript");
        if (zt) {
            zt->clear();
            for (const auto &l : _gameChatLines) {
                zt->appendPlainText(QString::fromStdString(l));
            }
            zt->verticalScrollBar()->setValue(zt->verticalScrollBar()->maximum());
        }
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
    _awaitingMatchStart = false;
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
    updateStartMatchOverlay();
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
        // Do not force BASE_OCCUPATION; let OctiGame::serializeJson derive
        // BASE_OCCUPATION vs TOTAL_CAPTURE vs STALEMATE.
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
    if (layout()) {
        layout()->activate();
    }
    if (centralWidget() && centralWidget()->layout()) {
        centralWidget()->layout()->activate();
    }
    if (_boardPage && _boardPage->layout()) {
        _boardPage->layout()->activate();
    }
    syncSidePanelsToBoardEdges();
    if (_rightSide && _rightSide->layout()) {
        _rightSide->layout()->activate();
    }
    pumpUiEvents();

    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(paletteForTheme(_theme).windowBg);
    render(&img);

    syncSidePanelsToBoardEdges();
    if (_rightSide && _rightSide->layout()) {
        _rightSide->layout()->activate();
    }
    pumpUiEvents();
    return img;
}

void OctiMainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    syncSidePanelsToBoardEdges();
}

void OctiMainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    syncSidePanelsToBoardEdges();
}

void OctiMainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
}

void OctiMainWindow::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);
}

bool OctiMainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (_gameChatTranscript && watched == _gameChatTranscript->viewport() && event && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me && me->button() == Qt::LeftButton) {
            if (!_gameChatTranscript->textCursor().hasSelection()) {
                openBoardChatZoomDialog();
                return true;
            }
        }
    }

    if (watched == _profileAvatarLabel && event && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me && me->button() == Qt::LeftButton) {
            if (_authenticated) {
                AccountLinkDialog dlg(_netClient.get(), &_configStore, _theme, this);
                dlg.setOnAccountSwitchedCallback([this](const PlayerProfileInfo &prof) {
                    applyAuthenticatedProfile(prof);
                });
                dlg.exec();
                if (!dlg.currentProfile().uuid.empty()) {
                    applyAuthenticatedProfile(dlg.currentProfile());
                }
            } else {
                SsoOnboardingDialog dlg(_netClient.get(), &_configStore, _theme, this);
                dlg.setOnSignedInCallback([this](const PlayerProfileInfo &prof) {
                    applyAuthenticatedProfile(prof);
                });
                if (dlg.exec() == QDialog::Accepted) {
                    _authenticated = true;
                    updateTopBarLabels();
                }
            }
            return true;
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
    if (_profileAvatarLabel) {
        if (_profile.avatarUrl.empty()) {
            _cachedAvatarUrl.clear();
            _cachedAvatarImage = QImage();
        } else if (_profile.avatarUrl != _cachedAvatarUrl) {
            _cachedAvatarUrl = _profile.avatarUrl;
            _cachedAvatarImage = AccountLinkDialog::loadAvatarImage(_profile.avatarUrl, _netClient.get());
        }
        QString hStr = _authenticated ? QString::fromStdString(_profile.handle) : QString("Guest");
        QPixmap pm = AccountLinkDialog::renderCircularAvatarPixmap(_cachedAvatarImage, hStr, 28, _theme);
        _profileAvatarLabel->setPixmap(pm);
        _profileAvatarLabel->setToolTip(_authenticated ? ("@" + hStr) : "Guest");
        _profileAvatarLabel->setVisible(true);
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
    bool waitingForPlayers =
        online && (_onlineGameView.phase == "WAITING_FOR_PLAYERS" ||
                   (!_onlineGameView.live && _onlineGameView.phase.empty()));
    bool inProgress = (online && _onlineGameView.phase == "IN_PROGRESS");
    bool seatedPlayer = (online && _onlineViewerSeat > 0);
    bool viewerEliminated = false;
    if (_onlineViewerSeat >= 1 && _onlineViewerSeat <= 4) {
        for (const auto &s : _onlineGameView.seats) {
            if (s.seat == _onlineViewerSeat && s.eliminated) {
                viewerEliminated = true;
                break;
            }
        }
        if (!viewerEliminated && _board.getGame()) {
            viewerEliminated = _board.getGame()->isPlayerEliminated(
                static_cast<OctiPlayer>(_onlineViewerSeat));
        }
    }
    bool canActInMatch = (seatedPlayer && inProgress && !viewerEliminated);

    if (_sidebarNewGameBtn) {
        _sidebarNewGameBtn->setVisible(!online);
        _sidebarNewGameBtn->setEnabled(!online);
    }
    if (_actNewGame) {
        _actNewGame->setEnabled(!online);
    }

    if (_waitingRoomBannerWidget) {
        _waitingRoomBannerWidget->setVisible(waitingForPlayers);
        if (waitingForPlayers && _waitingRoomBannerLabel) {
            int totalSeats = (_onlineGameView.mode == "4P" ||
                              _onlineGameView.mode == "4P_FFA") ? 4 : 2;
            int filledSeats = static_cast<int>(_onlineGameView.seats.size());
            _waitingRoomBannerLabel->setText(
                QString("Room #%1 • Seats %2/%3 — Waiting for opponents to join...")
                    .arg(QString::fromStdString(_onlineRoomId))
                    .arg(filledSeats)
                    .arg(totalSeats));
        }
    }
    if (_sitSeatBtn) {
        bool canSit = (waitingForPlayers && _onlineViewerSeat == 0);
        _sitSeatBtn->setVisible(canSit);
        _sitSeatBtn->setEnabled(canSit);
    }
    if (_deleteWaitingTableBtn) {
        _deleteWaitingTableBtn->setVisible(false);
        _deleteWaitingTableBtn->setEnabled(false);
    }
    if (_leaveWaitingTableBtn) {
        _leaveWaitingTableBtn->setEnabled(waitingForPlayers);
    }
    if (_leaveTableBtn) {
        _leaveTableBtn->setVisible(online);
        _leaveTableBtn->setEnabled(online);
    }
    if (_offerDrawBtn) {
        _offerDrawBtn->setVisible(online);
        _offerDrawBtn->setEnabled(canActInMatch);
    }
    if (_pauseResumeBtn) {
        _pauseResumeBtn->setVisible(online);
        _pauseResumeBtn->setEnabled(canActInMatch);
        _pauseResumeBtn->setText(_onlineGameView.paused ? "Resume" : "Pause");
    }
    if (_claimForfeitBtn) {
        bool opponentForfeitClaimable = false;
        for (const auto &s : _onlineGameView.seats) {
            if (s.seat != _onlineViewerSeat && s.forfeitClaimable) {
                opponentForfeitClaimable = true;
                break;
            }
        }
        _claimForfeitBtn->setVisible(online);
        _claimForfeitBtn->setEnabled(canActInMatch && opponentForfeitClaimable);
    }
    bool localGameActive = (_board.getGame() && _board.getGame()->isGameStarted() && !_board.getGame()->isGameOver());
    if (_undoRequestBtn) {
        if (online) {
            _undoRequestBtn->setVisible(false);
            _undoRequestBtn->setEnabled(false);
        } else {
            _undoRequestBtn->setVisible(true);
            _undoRequestBtn->setEnabled(localGameActive);
        }
    }
    if (_actUndo) {
        _actUndo->setEnabled(!online && localGameActive);
    }
    if (_resignBtn) {
        if (online) {
            _resignBtn->setEnabled(canActInMatch);
        } else {
            _resignBtn->setEnabled(localGameActive);
        }
    }
    if (_moderationBtn) {
        _moderationBtn->setVisible(online);
    }
    if (_chatBox) {
        _chatBox->setVisible(true);
    }

    if (_chatScopeCombo) {
        _chatScopeCombo->blockSignals(true);
        _chatScopeCombo->clear();
        _chatScopeCombo->addItem("ALL", "ALL");
        if (_onlineGameView.mode == "4P" && _onlineViewerSeat > 0) {
            _chatScopeCombo->addItem("TEAM", "TEAM");
        }
        _chatScopeCombo->blockSignals(false);
    }

    bool isObservingUnauthed = (online && !isAuthenticated());
    bool isSilentObserver = (online && _onlineViewerSeat <= 0 &&
                             _onlineGameView.observerPolicy == "SILENT_OBSERVERS");
    if (_gameChatInput) {
        if (isSilentObserver) {
            _gameChatInput->setEnabled(false);
            _gameChatInput->setPlaceholderText("Silent observers policy in effect");
        } else if (isObservingUnauthed) {
            _gameChatInput->setEnabled(false);
            _gameChatInput->setPlaceholderText("Sign in to chat");
        } else {
            _gameChatInput->setEnabled(true);
            _gameChatInput->setPlaceholderText("Send board message...");
        }
    }
    if (_sendGameChatBtn) {
        _sendGameChatBtn->setEnabled(!isObservingUnauthed && !isSilentObserver);
    }

    if (_matchModeLabel) {
        if (online) {
            QString roomDisp = QString::fromStdString(_onlineRoomId);
            if (roomDisp.size() > 16) {
                roomDisp = roomDisp.left(16);
            }
            QString modeStr = QString("Room #%1").arg(roomDisp);
            if (_onlineGameView.ttlRemainingMs >= 0) {
                int64_t totalSec = _onlineGameView.ttlRemainingMs / 1000;
                int mm = static_cast<int>(totalSec / 60);
                int ss = static_cast<int>(totalSec % 60);
                modeStr += QString::fromUtf8(" \xE2\x80\xA2 TTL %1:%2")
                    .arg(mm, 2, 10, QChar('0'))
                    .arg(ss, 2, 10, QChar('0'));
            }
            _matchModeLabel->setText(modeStr);
            _matchModeLabel->setToolTip(QString::fromStdString(_onlineRoomId));
        } else if (_isLocalBotGame) {
            _matchModeLabel->setText(
                QString::fromUtf8("Local vs AI Bot \xE2\x80\xA2 Offline Tabletop"));
        } else {
            _matchModeLabel->setText(
                QString::fromUtf8("Local Pass-and-Play \xE2\x80\xA2 Offline Tabletop"));
        }
    }

    if (_matchTurnStatusLabel) {
        if (waitingForPlayers) {
            _matchTurnStatusLabel->setText("Waiting for Players...");
        } else if (online && _onlineGameView.phase == "FINISHED") {
            QString term = QString::fromStdString(_onlineGameView.termination);
            if (!term.isEmpty()) {
                _matchTurnStatusLabel->setText(
                    QString("Match Complete \xe2\x80\x94 %1").arg(term));
            } else {
                _matchTurnStatusLabel->setText("Match Complete");
            }
        } else if (_awaitingMatchStart && !online) {
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

    syncSidePanelsToBoardEdges();
}

void OctiMainWindow::checkAndPresentGameOver()
{
    const OctiGame *g = _board.getGame();
    bool localGameOver = (g && g->isGameOver());
    bool onlineGameOver = (_inOnlineRoom && _onlineGameView.phase == "FINISHED");
    if (!localGameOver && !onlineGameOver) {
        return;
    }
    if (_gameOverDialogShownForCurrentMatch) {
        return;
    }
    _gameOverDialogShownForCurrentMatch = true;
    _soundSynth.triggerCue(SoundCue::GAME_VICTORY);
    updateTopBarLabels();
    updateMatchActionsVisibility();
    if (_boardCanvas) {
        _boardCanvas->update();
    }
    if (_reservePanel) {
        _reservePanel->refresh();
    }
    uploadCompletedBotGameIfEligible(false);
    if (isVisible()) {
        if (onlineGameOver && !localGameOver) {
            std::string banner = "Match Complete";
            if (_onlineGameView.winnerSeat > 0) {
                std::string winHandle = "Seat " + std::to_string(_onlineGameView.winnerSeat);
                for (const auto &s : _onlineGameView.seats) {
                    if (s.seat == _onlineGameView.winnerSeat && !s.handle.empty()) {
                        winHandle = "@" + s.handle;
                        break;
                    }
                }
                banner = "Winner: " + winHandle;
            } else if (_onlineGameView.result == "DRAW") {
                banner = "Match Drawn";
            }
            GameOverDialog dlg(banner, _onlineGameView.termination,
                               static_cast<int>(_appliedOnlineMoves.size()),
                               exportCurrentOctiTranscript(), _theme, this);
            dlg.exec();
            if (dlg.newGameRequested()) {
                proposeRematch();
            }
        } else {
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
        if (_netClient) {
            _netClient->setInstallId(prefs.installId);
        }

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
    if (_netClient) {
        _netClient->setInstallId(prefs.installId);
    }
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
        if (_netClient) {
            _netClient->setInstallId(_configStore.preferences().installId);
        }
        if (_heldTranscriptPendingConsent && _configStore.preferences().telemetryConsent != "unset") {
            _heldTranscriptPendingConsent = false;
            uploadCompletedBotGameIfEligible(false);
        }
    }
}

void OctiMainWindow::updateProposalBannerUi()
{
    if (!_proposalBannerWidget || !_proposalBannerLabel ||
        !_acceptProposalBtn || !_declineProposalBtn) {
        return;
    }
    if (_onlineGameView.proposals.empty()) {
        _proposalBannerWidget->setVisible(false);
        _proposalBannerLabel->setText("No active proposal.");
        return;
    }
    const GameProposalInfo &prop = _onlineGameView.proposals.front();
    std::string proposerName = prop.proposerUuid;
    for (const auto &seat : _onlineGameView.seats) {
        if (seat.playerUuid == prop.proposerUuid && !seat.handle.empty()) {
            proposerName = "@" + seat.handle;
            break;
        }
    }
    _proposalBannerLabel->setText(
        QString("Proposal: %1 (from %2)")
            .arg(QString::fromStdString(prop.type))
            .arg(QString::fromStdString(proposerName)));

    if (_onlineViewerSeat <= 0) {
        _acceptProposalBtn->setVisible(false);
        _declineProposalBtn->setVisible(false);
    } else if (prop.proposerUuid == _profile.uuid) {
        _acceptProposalBtn->setVisible(false);
        _declineProposalBtn->setText("Withdraw");
        _declineProposalBtn->setVisible(true);
        _declineProposalBtn->setEnabled(true);
    } else {
        _acceptProposalBtn->setText("Accept");
        _acceptProposalBtn->setVisible(true);
        bool alreadyAccepted = false;
        for (const auto &acceptedUuid : prop.acceptedBy) {
            if (acceptedUuid == _profile.uuid) {
                alreadyAccepted = true;
                break;
            }
        }
        _acceptProposalBtn->setEnabled(!alreadyAccepted);
        _declineProposalBtn->setText("Decline");
        _declineProposalBtn->setVisible(true);
        _declineProposalBtn->setEnabled(true);
    }

    _proposalBannerWidget->setVisible(true);
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
    (void)shouldShowBanner;

    if (!_updateBannerWidget) {
        return;
    }

    _updateBannerWidget->setVisible(false);

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
        _updateBannerWidget->setVisible(false);
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
