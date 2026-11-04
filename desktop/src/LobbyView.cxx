/*
 * LobbyView.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <nlohmann/json.hpp>
#include <QAbstractItemView>
#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>
#include "LobbyView.hxx"

namespace octi {

using Json = nlohmann::json;

namespace {

QString seatLetterForIndex(int seat)
{
    switch (seat) {
    case 1:
        return "A";
    case 2:
        return "B";
    case 3:
        return "C";
    case 4:
        return "D";
    default:
        return QString::number(seat);
    }
}

QString formatHandleWithAt(const std::string &rawHandle)
{
    if (rawHandle.empty()) {
        return "@Player";
    }
    QString h = QString::fromStdString(rawHandle);
    if (!h.startsWith("@")) {
        h.prepend("@");
    }
    return h;
}

QIcon makeAvatarIcon(const std::string &handle, ThemeId theme)
{
    const OctiPalette &pal = paletteForTheme(theme);
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF rect(1.0, 1.0, 22.0, 22.0);
    p.setBrush(QBrush(pal.panelCardBg));
    p.setPen(QPen(pal.accentBrass, 1.5));
    p.drawEllipse(rect);

    QString initial = "?";
    std::string clean = handle;
    if (!clean.empty() && clean[0] == '@') {
        clean = clean.substr(1);
    }
    if (!clean.empty()) {
        initial = QString(QChar(clean[0])).toUpper();
    }
    QFont font = p.font();
    font.setPixelSize(11);
    font.setBold(true);
    p.setFont(font);
    p.setPen(pal.accentBrass);
    p.drawText(rect, Qt::AlignCenter, initial);
    p.end();

    return QIcon(pix);
}

QString formatCompactDurationSec(std::int64_t sec)
{
    std::int64_t s = sec < 0 ? 0 : sec;
    if (s < 60) {
        return QString("%1s").arg(s);
    }
    if (s < 3600) {
        return QString("%1m").arg(s / 60);
    }
    return QString("%1h %2m").arg(s / 3600).arg((s % 3600) / 60);
}

QString formatClientsSummary(const std::vector<std::string> &clients)
{
    if (clients.empty()) {
        return QString("—");
    }
    QStringList formatted;
    for (const auto &c : clients) {
        if (c == "web") {
            formatted << "Web";
        } else if (c == "android") {
            formatted << "Android";
        } else if (c == "desktop") {
            formatted << "Desktop";
        } else if (!c.empty()) {
            formatted << QString::fromStdString(c);
        }
    }
    return formatted.isEmpty() ? QString("—") : formatted.join(" • ");
}

const TableSeatInfo *findSeatInfo(const TableSummaryInfo &table, int seatNum)
{
    for (std::size_t i = 0; i < table.seats.size(); ++i) {
        if (table.seats[i].seat == seatNum) {
            return &table.seats[i];
        }
    }
    return nullptr;
}

} // namespace

LobbyView::LobbyView(OctiNetworkClient *netClient,
                     ThemeId theme,
                     QWidget *parent)
    : QWidget(parent),
      _netClient(netClient),
      _theme(theme),
      _myTableBanner(nullptr),
      _myTableBannerLabel(nullptr),
      _myTableSeatsLabel(nullptr),
      _openActiveBoardBtn(nullptr),
      _leaveActiveTableBtn(nullptr),
      _playersTable(nullptr),
      _tablesTable(nullptr),
      _inspectorBox(nullptr),
      _inspectorTitleLabel(nullptr),
      _inspectorSeatsLabel(nullptr),
      _chatTranscript(nullptr),
      _chatInput(nullptr),
      _sendChatBtn(nullptr),
      _createTableBtn(nullptr),
      _joinSelectedBtn(nullptr),
      _leaveTableBtn(nullptr),
      _observeSelectedBtn(nullptr),
      _matchmakingBtn(nullptr),
      _refreshBtn(nullptr),
      _statusLabel(nullptr)
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setSpacing(10);
    outerLayout->setContentsMargins(14, 14, 14, 14);

    /* Top Banner: My Active Table */
    _myTableBanner = new QFrame(this);
    _myTableBanner->setFrameShape(QFrame::StyledPanel);
    _myTableBanner->setStyleSheet(
        "QFrame { background: rgba(212, 168, 83, 0.14); "
        "border: 1px solid rgba(212, 168, 83, 0.55); border-radius: 6px; }");
    QHBoxLayout *bannerLayout = new QHBoxLayout(_myTableBanner);
    bannerLayout->setContentsMargins(12, 8, 12, 8);
    bannerLayout->setSpacing(12);

    QVBoxLayout *bannerTextCol = new QVBoxLayout();
    bannerTextCol->setSpacing(2);
    _myTableBannerLabel = new QLabel("My Active Table", _myTableBanner);
    _myTableBannerLabel->setStyleSheet("font-weight: 700; border: none; background: transparent;");
    _myTableSeatsLabel = new QLabel("", _myTableBanner);
    _myTableSeatsLabel->setStyleSheet("border: none; background: transparent;");
    bannerTextCol->addWidget(_myTableBannerLabel);
    bannerTextCol->addWidget(_myTableSeatsLabel);
    bannerLayout->addLayout(bannerTextCol, 1);

    _openActiveBoardBtn = new QPushButton("Open Board", _myTableBanner);
    _leaveActiveTableBtn = new QPushButton("Leave Table", _myTableBanner);
    bannerLayout->addWidget(_openActiveBoardBtn);
    bannerLayout->addWidget(_leaveActiveTableBtn);
    _myTableBanner->setVisible(false);
    outerLayout->addWidget(_myTableBanner);

    QHBoxLayout *root = new QHBoxLayout();
    root->setSpacing(14);
    outerLayout->addLayout(root, 1);

    /* Center / Main Column: Active Game Tables + Table Inspector + Connected Users */
    QGroupBox *tablesBox = new QGroupBox("Active Game Tables", this);
    QVBoxLayout *tLayout = new QVBoxLayout(tablesBox);

    QHBoxLayout *toolbar = new QHBoxLayout();
    _createTableBtn = new QPushButton("+ Create Table", tablesBox);
    _joinSelectedBtn = new QPushButton("Join Selected Table", tablesBox);
    _leaveTableBtn = new QPushButton("Leave Table", tablesBox);
    _observeSelectedBtn = new QPushButton("Observe Selected Table", tablesBox);
    _refreshBtn = new QPushButton("Refresh", tablesBox);
    _purgeTableBtn = new QPushButton("Purge Table...", tablesBox);
    _purgeTableBtn->setStyleSheet("QPushButton { background-color: #dc2626; color: white; font-weight: bold; }");
    _purgeTableBtn->setVisible(false);
    toolbar->addWidget(_createTableBtn);
    toolbar->addWidget(_joinSelectedBtn);
    toolbar->addWidget(_leaveTableBtn);
    toolbar->addWidget(_observeSelectedBtn);
    toolbar->addWidget(_refreshBtn);
    toolbar->addWidget(_purgeTableBtn);
    tLayout->addLayout(toolbar);

    _tablesTable = new QTableWidget(0, 7, tablesBox);
    _tablesTable->setHorizontalHeaderLabels(
        QStringList() << "Room ID" << "Variant" << "Phase"
                      << "Seats" << "Observers" << "Access" << "Host");
    _tablesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _tablesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tablesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tLayout->addWidget(_tablesTable, 2);

    _inspectorBox = new QGroupBox("Table Seats & Details Inspector", tablesBox);
    QVBoxLayout *inspLayout = new QVBoxLayout(_inspectorBox);
    inspLayout->setContentsMargins(10, 8, 10, 8);
    inspLayout->setSpacing(4);
    _inspectorTitleLabel = new QLabel(
        "Select a table above or click '+ Create Table' to host a new match.",
        _inspectorBox);
    _inspectorTitleLabel->setWordWrap(true);
    _inspectorTitleLabel->setStyleSheet("font-weight: 600;");
    _inspectorSeatsLabel = new QLabel(
        "Seat roster and ruleset details will appear here.",
        _inspectorBox);
    _inspectorSeatsLabel->setWordWrap(true);
    inspLayout->addWidget(_inspectorTitleLabel);
    inspLayout->addWidget(_inspectorSeatsLabel);
    tLayout->addWidget(_inspectorBox);

    /* Connected Users Roster directly below _tablesTable and _inspectorBox */
    QGroupBox *playersBox = new QGroupBox("Connected Users", tablesBox);
    QVBoxLayout *pLayout = new QVBoxLayout(playersBox);
    pLayout->setContentsMargins(8, 6, 8, 6);
    _playersTable = new QTableWidget(0, 5, playersBox);
    _playersTable->setHorizontalHeaderLabels(
        QStringList() << "User" << "Clients" << "Online" << "Idle" << "Status / Room");
    _playersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _playersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _playersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _playersTable->setMinimumHeight(130);
    pLayout->addWidget(_playersTable);
    tLayout->addWidget(playersBox, 2);

    _statusLabel = new QLabel("Connected to OCTI Lobby.", tablesBox);
    tLayout->addWidget(_statusLabel);
    root->addWidget(tablesBox, 2);

    /* Right Column: Global Lobby Chat */
    QGroupBox *chatBox = new QGroupBox("Global Lobby Chat", this);
    QVBoxLayout *cLayout = new QVBoxLayout(chatBox);
    _chatTranscript = new QPlainTextEdit(chatBox);
    _chatTranscript->setReadOnly(true);
    cLayout->addWidget(_chatTranscript, 1);

    QHBoxLayout *chatInputRow = new QHBoxLayout();
    _chatInput = new QLineEdit(chatBox);
    _chatInput->setPlaceholderText("Message the global lobby...");
    _sendChatBtn = new QPushButton("Send", chatBox);
    chatInputRow->addWidget(_chatInput, 1);
    chatInputRow->addWidget(_sendChatBtn);
    cLayout->addLayout(chatInputRow);
    chatBox->setMinimumWidth(280);
    chatBox->setMaximumWidth(380);
    root->addWidget(chatBox, 1);

    QObject::connect(_tablesTable, &QTableWidget::cellClicked,
                     [this](int row, int /*col*/) {
        if (row >= 0 && row < static_cast<int>(_tables.size())) {
            _selectedRoomId = _tables[static_cast<std::size_t>(row)].roomId;
            updateInspectorAndSelectionControls();
        }
    });
    QObject::connect(_tablesTable, &QTableWidget::itemSelectionChanged,
                     [this]() {
        int row = _tablesTable->currentRow();
        if (row >= 0 && row < static_cast<int>(_tables.size())) {
            _selectedRoomId = _tables[static_cast<std::size_t>(row)].roomId;
            updateInspectorAndSelectionControls();
        }
    });
    QObject::connect(_tablesTable, &QTableWidget::cellDoubleClicked,
                     [this](int row, int /*col*/) {
        if (row >= 0 && row < static_cast<int>(_tables.size())) {
            const TableSummaryInfo &t = _tables[static_cast<std::size_t>(row)];
            _selectedRoomId = t.roomId;
            updateInspectorAndSelectionControls();
            if (isViewerSeatedAtTable(t)) {
                if (_onOpenActiveBoardRequested) {
                    _onOpenActiveBoardRequested();
                }
            } else if (t.phase == "WAITING_FOR_PLAYERS" &&
                       t.seatsFilled < t.seatsTotal) {
                if (_onJoinTableRequested) {
                    _onJoinTableRequested(t);
                }
            } else if (_onObserveTableRequested) {
                _onObserveTableRequested(t);
            }
        }
    });
    QObject::connect(_createTableBtn, &QPushButton::clicked, [this]() {
        if (_onCreateTableRequested) {
            _onCreateTableRequested();
        }
    });
    QObject::connect(_joinSelectedBtn, &QPushButton::clicked, [this]() {
        const TableSummaryInfo *t = findTable(selectedRoomId());
        if (t && _onJoinTableRequested) {
            _onJoinTableRequested(*t);
        }
    });
    QObject::connect(_leaveTableBtn, &QPushButton::clicked, [this]() {
        std::string targetRoomId = selectedRoomId();
        const TableSummaryInfo *t = findTable(targetRoomId);
        if ((!t || !isViewerSeatedAtTable(*t)) && !_bannerRoomId.empty()) {
            targetRoomId = _bannerRoomId;
        }
        if (!targetRoomId.empty() && _onLeaveTableRequested) {
            _onLeaveTableRequested(targetRoomId);
        }
    });
    QObject::connect(_observeSelectedBtn, &QPushButton::clicked, [this]() {
        const TableSummaryInfo *t = findTable(selectedRoomId());
        if (t && _onObserveTableRequested) {
            _onObserveTableRequested(*t);
        }
    });
    QObject::connect(_openActiveBoardBtn, &QPushButton::clicked, [this]() {
        if (_onOpenActiveBoardRequested) {
            _onOpenActiveBoardRequested();
        }
    });
    QObject::connect(_leaveActiveTableBtn, &QPushButton::clicked, [this]() {
        std::string roomId = activeBannerRoomId();
        if (!roomId.empty() && _onLeaveTableRequested) {
            _onLeaveTableRequested(roomId);
        }
    });
    QObject::connect(_refreshBtn, &QPushButton::clicked, [this]() {
        refreshLobby();
    });
    QObject::connect(_purgeTableBtn, &QPushButton::clicked, [this]() {
        if (_selectedRoomId.empty() || !_netClient) {
            return;
        }
        auto reply = QMessageBox::warning(
            this,
            "Confirm Permanent Purge",
            QString("Are you sure you want to immediately destroy table #%1 without archiving?\n\nThis action cannot be undone.")
                .arg(QString::fromStdString(_selectedRoomId)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            auto res = _netClient->purgeTable(_selectedRoomId);
            if (!res.ok) {
                QMessageBox::critical(this, "Purge Failed", QString::fromStdString(res.errorMessage));
            } else {
                refreshLobby();
            }
        }
    });

    _secondTimer = new QTimer(this);
    QObject::connect(_secondTimer, &QTimer::timeout, this, [this]() {
        for (std::size_t i = 0; i < _tables.size(); ++i) {
            if (_tables[i].ttlRemainingMs > 0) {
                _tables[i].ttlRemainingMs = std::max<std::int64_t>(0, _tables[i].ttlRemainingMs - 1000);
                if (_tablesTable && i < static_cast<std::size_t>(_tablesTable->rowCount())) {
                    QString phaseStr = formatPhaseDisplay(_tables[i].phase);
                    std::int64_t totalSec = _tables[i].ttlRemainingMs / 1000;
                    std::int64_t m = totalSec / 60;
                    std::int64_t s = totalSec % 60;
                    phaseStr += QString(" [TTL %1:%2]")
                        .arg(m, 2, 10, QChar('0'))
                        .arg(s, 2, 10, QChar('0'));
                    QTableWidgetItem *item = _tablesTable->item(static_cast<int>(i), 2);
                    if (item) {
                        item->setText(phaseStr);
                    }
                }
            }
        }
    });
    _secondTimer->start(1000);
    QObject::connect(_sendChatBtn, &QPushButton::clicked, [this]() {
        std::string txt = _chatInput->text().trimmed().toStdString();
        if (!txt.empty() && sendLobbyChatMessage(txt)) {
            _chatInput->clear();
        }
    });
    QObject::connect(_chatInput, &QLineEdit::returnPressed, [this]() {
        std::string txt = _chatInput->text().trimmed().toStdString();
        if (!txt.empty() && sendLobbyChatMessage(txt)) {
            _chatInput->clear();
        }
    });

    setTheme(_theme);
    updateAuthUiState();
}

LobbyView::~LobbyView() = default;

void LobbyView::attachNetworkClient(OctiNetworkClient *netClient)
{
    _netClient = netClient;
    updateAuthUiState();
}

OctiNetworkClient *LobbyView::networkClient() const
{
    return _netClient;
}

void LobbyView::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId LobbyView::theme() const
{
    return _theme;
}

void LobbyView::setViewerIdentity(const std::string &playerUuid,
                                  const std::string &handle)
{
    _viewerUuid = playerUuid;
    _viewerHandle = handle;
    updateMyTableBanner();
    updateInspectorAndSelectionControls();
}

void LobbyView::setActiveRoomState(bool inRoom,
                                   const std::string &roomId,
                                   int viewerSeat,
                                   const GameViewData &gameView)
{
    _inActiveRoom = inRoom && !roomId.empty();
    _activeRoomId = _inActiveRoom ? roomId : std::string();
    _activeViewerSeat = _inActiveRoom ? viewerSeat : 0;
    _activeGameView = _inActiveRoom ? gameView : GameViewData();
    updateMyTableBanner();
    updateInspectorAndSelectionControls();
}

QString LobbyView::formatChatTimestamp(std::int64_t atMs)
{
    QDateTime localDt = (atMs > 0)
        ? QDateTime::fromMSecsSinceEpoch(
              static_cast<qint64>(atMs), QTimeZone::utc()).toLocalTime()
        : QDateTime::currentDateTime();
    if (localDt.date() == QDate::currentDate()) {
        return QString("[%1] ").arg(localDt.toString("HH:mm"));
    }
    return QString("[%1] ").arg(localDt.toString("MM-dd HH:mm"));
}

QString LobbyView::formatChatMessageLine(const LobbyChatEntry &entry)
{
    std::int64_t ts = (entry.createdAtMs > 0) ? entry.createdAtMs : entry.atMs;
    QString prefix = formatChatTimestamp(ts);
    QString handle = formatHandleWithAt(entry.handle);
    return QString("%1%2: %3")
        .arg(prefix, handle, QString::fromStdString(entry.text));
}

QString LobbyView::formatPhaseDisplay(const std::string &phase)
{
    if (phase == "WAITING_FOR_PLAYERS") {
        return "Waiting for Players";
    }
    if (phase == "IN_PROGRESS") {
        return "In Progress";
    }
    if (phase == "FINISHED" || phase == "COMPLETED" || phase == "GAME_OVER") {
        return "Completed";
    }
    return QString::fromStdString(phase);
}

QString LobbyView::formatSeatOccupancySummary(const TableSummaryInfo &table)
{
    int totalSeats = (table.seatsTotal > 0)
        ? table.seatsTotal
        : ((table.mode == "4P" || table.mode == "4P_FFA") ? 4 : 2);
    QStringList parts;
    for (int s = 1; s <= totalSeats; ++s) {
        QString letter = seatLetterForIndex(s);
        const TableSeatInfo *seat = findSeatInfo(table, s);
        if (seat && (!seat->handle.empty() || !seat->playerUuid.empty())) {
            QString botTag = seat->isRobot ? " [BOT]" : "";
            parts << QString("%1: %2%3").arg(
                letter, formatHandleWithAt(seat->handle), botTag);
        } else if (table.seats.empty() && s == 1 &&
                   table.seatsFilled >= 1 && !table.hostHandle.empty()) {
            parts << QString("%1: %2").arg(
                letter, formatHandleWithAt(table.hostHandle));
        } else {
            parts << QString("%1: [Open]").arg(letter);
        }
    }
    return QString("%1/%2 — %3")
        .arg(table.seatsFilled)
        .arg(totalSeats)
        .arg(parts.join(", "));
}

QString LobbyView::formatSeatBreakdownText(const TableSummaryInfo &table)
{
    int totalSeats = (table.seatsTotal > 0)
        ? table.seatsTotal
        : ((table.mode == "4P" || table.mode == "4P_FFA") ? 4 : 2);
    QStringList lines;
    for (int s = 1; s <= totalSeats; ++s) {
        QString letter = seatLetterForIndex(s);
        const TableSeatInfo *seat = findSeatInfo(table, s);
        if (seat && (!seat->handle.empty() || !seat->playerUuid.empty())) {
            QString connTag = seat->connected ? "" : " [away]";
            QString botTag = seat->isRobot ? " [BOT]" : "";
            lines << QString("Seat %1 (%2): %3%4 (%5)%6")
                         .arg(s)
                         .arg(letter)
                         .arg(formatHandleWithAt(seat->handle))
                         .arg(botTag)
                         .arg(seat->rating > 0 ? seat->rating : 1500)
                         .arg(connTag);
        } else if (table.seats.empty() && s == 1 &&
                   table.seatsFilled >= 1 && !table.hostHandle.empty()) {
            lines << QString("Seat %1 (%2): %3")
                         .arg(s)
                         .arg(letter)
                         .arg(formatHandleWithAt(table.hostHandle));
        } else {
            lines << QString("Seat %1 (%2): [Open]").arg(s).arg(letter);
        }
    }
    return lines.join("  •  ");
}

bool LobbyView::isViewerSeatedAtTable(const TableSummaryInfo &table) const
{
    if (_inActiveRoom && !_activeRoomId.empty() &&
        table.roomId == _activeRoomId && _activeViewerSeat > 0) {
        return true;
    }
    for (std::size_t i = 0; i < table.seats.size(); ++i) {
        const TableSeatInfo &s = table.seats[i];
        if (!_viewerUuid.empty() && !s.playerUuid.empty() &&
            s.playerUuid == _viewerUuid) {
            return true;
        }
        if (!_viewerHandle.empty() && !s.handle.empty() &&
            s.handle == _viewerHandle) {
            return true;
        }
    }
    if (!_viewerUuid.empty() && !table.hostUuid.empty() &&
        table.hostUuid == _viewerUuid) {
        return true;
    }
    if (!_viewerHandle.empty() && !table.hostHandle.empty() &&
        table.hostHandle == _viewerHandle) {
        return true;
    }
    return false;
}

bool LobbyView::isMyTableBannerVisible() const
{
    return _myTableBanner != nullptr && !_myTableBanner->isHidden();
}

std::string LobbyView::activeBannerRoomId() const
{
    return _bannerRoomId;
}

void LobbyView::updateMyTableBanner()
{
    if (!_myTableBanner || !_myTableBannerLabel || !_myTableSeatsLabel) {
        return;
    }
    const TableSummaryInfo *matchedTable = nullptr;
    if (_inActiveRoom && !_activeRoomId.empty()) {
        matchedTable = findTable(_activeRoomId);
    }
    if (!matchedTable) {
        for (std::size_t i = 0; i < _tables.size(); ++i) {
            if (isViewerSeatedAtTable(_tables[i]) &&
                (_tables[i].phase == "WAITING_FOR_PLAYERS" ||
                 _tables[i].phase == "IN_PROGRESS")) {
                matchedTable = &_tables[i];
                break;
            }
        }
    }

    if (matchedTable) {
        _bannerRoomId = matchedTable->roomId;
        QString phaseText =
            (matchedTable->phase == "WAITING_FOR_PLAYERS")
                ? "WAITING FOR PLAYERS"
                : ((matchedTable->phase == "IN_PROGRESS")
                       ? "IN PROGRESS"
                       : QString::fromStdString(matchedTable->phase));
        QString variant = QString("%1 • %2B%3%4")
            .arg(QString::fromStdString(matchedTable->mode))
            .arg(matchedTable->bases)
            .arg(matchedTable->superProngs ? " +!" : "")
            .arg(matchedTable->edgeless ? " +Wrap" : "");
        _myTableBannerLabel->setText(
            QString("My Active Table: Room #%1  |  %2  |  Phase: %3")
                .arg(QString::fromStdString(matchedTable->roomId),
                     variant,
                     phaseText));
        _myTableSeatsLabel->setText(formatSeatBreakdownText(*matchedTable));
        _myTableBanner->setVisible(true);
        return;
    }

    if (_inActiveRoom && !_activeRoomId.empty()) {
        _bannerRoomId = _activeRoomId;
        std::string phase = _activeGameView.phase.empty()
            ? "WAITING_FOR_PLAYERS"
            : _activeGameView.phase;
        QString phaseText =
            (phase == "WAITING_FOR_PLAYERS")
                ? "WAITING FOR PLAYERS"
                : ((phase == "IN_PROGRESS")
                       ? "IN PROGRESS"
                       : QString::fromStdString(phase));
        std::string mode = _activeGameView.mode.empty()
            ? "2P"
            : _activeGameView.mode;
        int bases = _activeGameView.bases > 0 ? _activeGameView.bases : 3;
        TableSummaryInfo synth;
        synth.roomId = _activeRoomId;
        synth.mode = mode;
        synth.bases = bases;
        synth.superProngs = _activeGameView.superProngs;
        synth.edgeless = _activeGameView.edgeless;
        synth.phase = phase;
        synth.seatsTotal = (mode == "4P" || mode == "4P_FFA") ? 4 : 2;
        synth.seatsFilled = static_cast<int>(_activeGameView.seats.size());
        for (std::size_t i = 0; i < _activeGameView.seats.size(); ++i) {
            TableSeatInfo s;
            s.seat = _activeGameView.seats[i].seat;
            s.playerUuid = _activeGameView.seats[i].playerUuid;
            s.handle = _activeGameView.seats[i].handle;
            s.connected = _activeGameView.seats[i].connected;
            synth.seats.push_back(s);
        }
        QString variant = QString("%1 • %2B%3%4")
            .arg(QString::fromStdString(synth.mode))
            .arg(synth.bases)
            .arg(synth.superProngs ? " +!" : "")
            .arg(synth.edgeless ? " +Wrap" : "");
        _myTableBannerLabel->setText(
            QString("My Active Table: Room #%1  |  %2  |  Phase: %3")
                .arg(QString::fromStdString(_activeRoomId),
                     variant,
                     phaseText));
        _myTableSeatsLabel->setText(formatSeatBreakdownText(synth));
        _myTableBanner->setVisible(true);
        return;
    }

    _bannerRoomId.clear();
    _myTableBanner->setVisible(false);
}

void LobbyView::updateInspectorAndSelectionControls()
{
    bool authed = (_netClient && _netClient->isAuthenticated());
    const TableSummaryInfo *selected = findTable(selectedRoomId());

    if (_inspectorTitleLabel && _inspectorSeatsLabel) {
        if (selected) {
            QString variant = QString("%1 • %2 Bases%3%4")
                .arg(QString::fromStdString(selected->mode))
                .arg(selected->bases)
                .arg(selected->superProngs ? " • SuperProngs" : "")
                .arg(selected->edgeless ? " • Edgeless" : "");
            QString access = selected->hasPasscode
                ? "Locked [Passcode Required]"
                : "Open Table";
            _inspectorTitleLabel->setText(
                QString("Room #%1  •  Host: %2  •  %3  •  Phase: %4  •  %5  •  Observers: %6")
                    .arg(QString::fromStdString(selected->roomId),
                         formatHandleWithAt(selected->hostHandle),
                         variant,
                         formatPhaseDisplay(selected->phase),
                         access,
                         QString::number(selected->observerCount)));
            _inspectorSeatsLabel->setText(formatSeatBreakdownText(*selected));
        } else {
            _inspectorTitleLabel->setText(
                "No table selected — select a table above or click '+ Create Table' to host a match.");
            _inspectorSeatsLabel->setText(
                "Seat 1 (A): [Open]  •  Seat 2 (B): [Open]");
        }
    }

    if (_joinSelectedBtn) {
        if (!authed) {
            _joinSelectedBtn->setEnabled(false);
        } else if (selected) {
            bool canJoin =
                (selected->phase == "WAITING_FOR_PLAYERS" &&
                 selected->seatsFilled < selected->seatsTotal &&
                 !isViewerSeatedAtTable(*selected));
            _joinSelectedBtn->setEnabled(canJoin);
        } else {
            _joinSelectedBtn->setEnabled(true);
        }
    }

    if (_leaveTableBtn) {
        bool canLeaveSelected =
            authed && selected && isViewerSeatedAtTable(*selected);
        bool canLeaveBanner = authed && !_bannerRoomId.empty();
        _leaveTableBtn->setEnabled(canLeaveSelected || canLeaveBanner);
    }

    if (_observeSelectedBtn) {
        _observeSelectedBtn->setEnabled(true);
    }

    if (_purgeTableBtn && _netClient) {
        _purgeTableBtn->setEnabled(
            _netClient->currentProfile().isAdmin && !_selectedRoomId.empty());
    }
}

void LobbyView::updateAuthUiState()
{
    bool authed = (_netClient && _netClient->isAuthenticated());
    bool isAdmin = (_netClient && _netClient->currentProfile().isAdmin);
    if (_purgeTableBtn) {
        _purgeTableBtn->setVisible(isAdmin);
        _purgeTableBtn->setEnabled(isAdmin && !_selectedRoomId.empty());
    }
    if (_refreshBtn) {
        _refreshBtn->setEnabled(true);
    }
    if (_observeSelectedBtn) {
        _observeSelectedBtn->setEnabled(true);
    }
    if (_createTableBtn) {
        _createTableBtn->setEnabled(authed);
    }
    if (_chatInput) {
        _chatInput->setEnabled(authed);
        if (!authed) {
            _chatInput->setPlaceholderText("Sign in to chat");
        } else {
            _chatInput->setPlaceholderText("Message the global lobby...");
        }
    }
    if (_sendChatBtn) {
        _sendChatBtn->setEnabled(authed);
    }
    if (_statusLabel) {
        if (!authed) {
            _statusLabel->setText(
                QString("Guest observation mode • %1 online player(s) • %2 active table(s)")
                    .arg(static_cast<int>(_players.size()))
                    .arg(static_cast<int>(_tables.size())));
        } else {
            _statusLabel->setText(
                QString("%1 online player(s) • %2 active table(s)")
                    .arg(static_cast<int>(_players.size()))
                    .arg(static_cast<int>(_tables.size())));
        }
    }
    updateMyTableBanner();
    updateInspectorAndSelectionControls();
}

bool LobbyView::refreshLobby()
{
    if (!_netClient) {
        return false;
    }
    auto res = _netClient->fetchLobbyState();
    if (!res.ok) {
        return false;
    }
    refreshFromLobbyState(res.data);
    return true;
}

void LobbyView::refreshFromLobbyState(const LobbyStateData &state)
{
    _players = state.players;
    _tables = state.tables;
    _chatMessages = state.recentChat;
    if (!_selectedRoomId.empty() && !findTable(_selectedRoomId)) {
        _selectedRoomId.clear();
    }
    if (_selectedRoomId.empty() && !_tables.empty()) {
        _selectedRoomId = _tables.front().roomId;
    }
    rebuildPlayersTable();
    rebuildTablesWidget();
    rebuildChatTranscript();
    updateAuthUiState();
}

void LobbyView::handleLobbySseEvent(const SseEvent &ev)
{
    Json j = Json::parse(ev.dataJson, nullptr, false);
    if (ev.event == "LOBBY_CHAT" && j.is_object()) {
        LobbyChatEntry entry;
        entry.seq = j.value("seq", static_cast<std::int64_t>(ev.id));
        std::int64_t ts = j.value(
            "created_at_ms",
            j.value("at_ms", static_cast<std::int64_t>(0)));
        entry.createdAtMs = ts;
        entry.atMs = ts;
        entry.playerUuid = j.value("player_uuid", "");
        entry.handle = j.value("handle", "");
        entry.text = j.value("text", "");
        appendLobbyChatEntry(entry);
        return;
    }
    if (ev.event == "MATCH_FOUND" && j.is_object()) {
        std::string roomId = j.value("room_id", "");
        if (!roomId.empty() && _onMatchFoundCallback) {
            _onMatchFoundCallback(roomId);
        }
        refreshLobby();
        return;
    }
    if (ev.event == "PRESENCE" || ev.event == "TABLE_UPDATED" ||
        ev.event == "TABLE_CLOSED") {
        refreshLobby();
    }
}

bool LobbyView::sendLobbyChatMessage(const std::string &text)
{
    if (!_netClient || !_netClient->isAuthenticated() || text.empty()) {
        return false;
    }
    auto res = _netClient->postLobbyChat(text);
    if (!res.ok) {
        return false;
    }
    refreshLobby();
    return true;
}

void LobbyView::appendLobbyChatEntry(const LobbyChatEntry &entry)
{
    for (std::size_t i = 0; i < _chatMessages.size(); ++i) {
        if (entry.seq > 0 && _chatMessages[i].seq == entry.seq) {
            return;
        }
    }
    LobbyChatEntry normalized = entry;
    if (normalized.createdAtMs <= 0 && normalized.atMs > 0) {
        normalized.createdAtMs = normalized.atMs;
    } else if (normalized.atMs <= 0 && normalized.createdAtMs > 0) {
        normalized.atMs = normalized.createdAtMs;
    }
    _chatMessages.push_back(normalized);
    if (_chatTranscript) {
        _chatTranscript->appendPlainText(formatChatMessageLine(normalized));
    }
}

const std::vector<TableSummaryInfo> &LobbyView::tables() const
{
    return _tables;
}

const std::vector<LobbyPlayerInfo> &LobbyView::players() const
{
    return _players;
}

const std::vector<LobbyChatEntry> &LobbyView::chatMessages() const
{
    return _chatMessages;
}

std::string LobbyView::chatTranscriptText() const
{
    if (!_chatTranscript) {
        return std::string();
    }
    return _chatTranscript->toPlainText().toStdString();
}

std::string LobbyView::selectedRoomId() const
{
    if (!_selectedRoomId.empty()) {
        return _selectedRoomId;
    }
    if (!_tables.empty()) {
        return _tables.front().roomId;
    }
    return std::string();
}

void LobbyView::selectRoomId(const std::string &roomId)
{
    _selectedRoomId = roomId;
    for (int i = 0; i < static_cast<int>(_tables.size()); ++i) {
        if (_tables[static_cast<std::size_t>(i)].roomId == roomId) {
            if (_tablesTable) {
                _tablesTable->selectRow(i);
            }
            break;
        }
    }
    updateInspectorAndSelectionControls();
}

const TableSummaryInfo *LobbyView::findTable(const std::string &roomId) const
{
    for (std::size_t i = 0; i < _tables.size(); ++i) {
        if (_tables[i].roomId == roomId) {
            return &_tables[i];
        }
    }
    return nullptr;
}

void LobbyView::setOnCreateTableRequested(std::function<void()> cb)
{
    _onCreateTableRequested = std::move(cb);
}

void LobbyView::setOnJoinTableRequested(
    std::function<void(const TableSummaryInfo &)> cb)
{
    _onJoinTableRequested = std::move(cb);
}

void LobbyView::setOnLeaveTableRequested(
    std::function<void(const std::string &roomId)> cb)
{
    _onLeaveTableRequested = std::move(cb);
}

void LobbyView::setOnObserveTableRequested(
    std::function<void(const TableSummaryInfo &)> cb)
{
    _onObserveTableRequested = std::move(cb);
}

void LobbyView::setOnOpenActiveBoardRequested(std::function<void()> cb)
{
    _onOpenActiveBoardRequested = std::move(cb);
}

void LobbyView::setOnMatchmakingRequested(std::function<void()> cb)
{
    _onMatchmakingRequested = std::move(cb);
}

void LobbyView::setOnMatchFoundCallback(
    std::function<void(const std::string &roomId)> cb)
{
    _onMatchFoundCallback = std::move(cb);
}

void LobbyView::rebuildPlayersTable()
{
    if (!_playersTable) {
        return;
    }
    _playersTable->setRowCount(static_cast<int>(_players.size()));
    for (int i = 0; i < static_cast<int>(_players.size()); ++i) {
        const LobbyPlayerInfo &p = _players[static_cast<std::size_t>(i)];

        QString handleText = formatHandleWithAt(p.handle);
        if (p.isRobot) {
            handleText += " [BOT]";
        }
        QTableWidgetItem *userItem = new QTableWidgetItem(handleText);
        userItem->setIcon(makeAvatarIcon(p.handle, _theme));
        _playersTable->setItem(i, 0, userItem);

        _playersTable->setItem(
            i, 1, new QTableWidgetItem(formatClientsSummary(p.clients)));

        _playersTable->setItem(
            i, 2, new QTableWidgetItem(formatCompactDurationSec(p.onlineSec)));

        QString idleStr = (p.idleSec < 10) ? QString("Active")
                                           : formatCompactDurationSec(p.idleSec);
        _playersTable->setItem(i, 3, new QTableWidgetItem(idleStr));

        QString statusRoom = QString::fromStdString(p.status);
        if (!p.roomId.empty()) {
            statusRoom += " #" + QString::fromStdString(p.roomId);
        }
        _playersTable->setItem(i, 4, new QTableWidgetItem(statusRoom));
    }
}

void LobbyView::rebuildTablesWidget()
{
    if (!_tablesTable) {
        return;
    }
    _tablesTable->setRowCount(static_cast<int>(_tables.size()));
    int selectedRow = -1;
    for (int i = 0; i < static_cast<int>(_tables.size()); ++i) {
        const TableSummaryInfo &t = _tables[static_cast<std::size_t>(i)];
        if (t.roomId == _selectedRoomId) {
            selectedRow = i;
        }
        QString variant = QString("%1 • %2B%3%4")
            .arg(QString::fromStdString(t.mode))
            .arg(t.bases)
            .arg(t.superProngs ? " +!" : "")
            .arg(t.edgeless ? " +Wrap" : "");
        QString seats = formatSeatOccupancySummary(t);
        QString access = t.hasPasscode ? QString("Locked [Passcode]")
                                       : QString("Open");
        _tablesTable->setItem(
            i, 0, new QTableWidgetItem(QString::fromStdString(t.roomId)));
        _tablesTable->setItem(
            i, 1, new QTableWidgetItem(variant));
        QString phaseStr = formatPhaseDisplay(t.phase);
        if (t.ttlRemainingMs >= 0) {
            std::int64_t totalSec = std::max<std::int64_t>(0, t.ttlRemainingMs / 1000);
            std::int64_t m = totalSec / 60;
            std::int64_t s = totalSec % 60;
            phaseStr += QString(" [TTL %1:%2]")
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0'));
        }
        _tablesTable->setItem(
            i, 2, new QTableWidgetItem(phaseStr));
        _tablesTable->setItem(
            i, 3, new QTableWidgetItem(seats));
        _tablesTable->setItem(
            i, 4, new QTableWidgetItem(QString::number(t.observerCount)));
        _tablesTable->setItem(
            i, 5, new QTableWidgetItem(access));
        _tablesTable->setItem(
            i, 6, new QTableWidgetItem(formatHandleWithAt(t.hostHandle)));
    }
    if (selectedRow >= 0) {
        _tablesTable->selectRow(selectedRow);
    }
    updateMyTableBanner();
    updateInspectorAndSelectionControls();
}

void LobbyView::rebuildChatTranscript()
{
    if (!_chatTranscript) {
        return;
    }
    _chatTranscript->clear();
    for (std::size_t i = 0; i < _chatMessages.size(); ++i) {
        const LobbyChatEntry &c = _chatMessages[i];
        _chatTranscript->appendPlainText(formatChatMessageLine(c));
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
