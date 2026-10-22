/*
 * LobbyView.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <nlohmann/json.hpp>
#include <QAbstractItemView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include "LobbyView.hxx"

namespace octi {

using Json = nlohmann::json;

LobbyView::LobbyView(OctiNetworkClient *netClient,
                     ThemeId theme,
                     QWidget *parent)
    : QWidget(parent),
      _netClient(netClient),
      _theme(theme),
      _playersTable(nullptr),
      _tablesTable(nullptr),
      _chatTranscript(nullptr),
      _chatInput(nullptr),
      _sendChatBtn(nullptr),
      _createTableBtn(nullptr),
      _joinSelectedBtn(nullptr),
      _observeSelectedBtn(nullptr),
      _matchmakingBtn(nullptr),
      _refreshBtn(nullptr),
      _statusLabel(nullptr)
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(14, 14, 14, 14);

    /* Left Column: Online Players Roster */
    QGroupBox *playersBox = new QGroupBox("Online Players Roster", this);
    QVBoxLayout *pLayout = new QVBoxLayout(playersBox);
    _playersTable = new QTableWidget(0, 3, playersBox);
    _playersTable->setHorizontalHeaderLabels(
        QStringList() << "Handle" << "Status" << "Room");
    _playersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _playersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _playersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pLayout->addWidget(_playersTable);
    playersBox->setMinimumWidth(260);
    playersBox->setMaximumWidth(340);
    root->addWidget(playersBox, 1);

    /* Center Column: Active Game Tables */
    QGroupBox *tablesBox = new QGroupBox("Active Game Tables", this);
    QVBoxLayout *tLayout = new QVBoxLayout(tablesBox);

    QHBoxLayout *toolbar = new QHBoxLayout();
    _createTableBtn = new QPushButton("Create Table", tablesBox);
    _joinSelectedBtn = new QPushButton("Join Selected", tablesBox);
    _observeSelectedBtn = new QPushButton("Observe Selected", tablesBox);
    _matchmakingBtn = new QPushButton("Quick Matchmaking", tablesBox);
    _refreshBtn = new QPushButton("Refresh", tablesBox);
    toolbar->addWidget(_createTableBtn);
    toolbar->addWidget(_joinSelectedBtn);
    toolbar->addWidget(_observeSelectedBtn);
    toolbar->addWidget(_matchmakingBtn);
    toolbar->addWidget(_refreshBtn);
    tLayout->addLayout(toolbar);

    _tablesTable = new QTableWidget(0, 7, tablesBox);
    _tablesTable->setHorizontalHeaderLabels(
        QStringList() << "Room ID" << "Variant" << "Phase"
                      << "Seats" << "Observers" << "Access" << "Host");
    _tablesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _tablesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tablesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tLayout->addWidget(_tablesTable);

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
        }
    });
    QObject::connect(_tablesTable, &QTableWidget::cellDoubleClicked,
                     [this](int row, int /*col*/) {
        if (row >= 0 && row < static_cast<int>(_tables.size())) {
            _selectedRoomId = _tables[static_cast<std::size_t>(row)].roomId;
            if (_onJoinTableRequested) {
                _onJoinTableRequested(_tables[static_cast<std::size_t>(row)]);
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
    QObject::connect(_observeSelectedBtn, &QPushButton::clicked, [this]() {
        const TableSummaryInfo *t = findTable(selectedRoomId());
        if (t && _onObserveTableRequested) {
            _onObserveTableRequested(*t);
        }
    });
    QObject::connect(_matchmakingBtn, &QPushButton::clicked, [this]() {
        if (_onMatchmakingRequested) {
            _onMatchmakingRequested();
        }
    });
    QObject::connect(_refreshBtn, &QPushButton::clicked, [this]() {
        refreshLobby();
    });
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
}

LobbyView::~LobbyView() = default;

void LobbyView::attachNetworkClient(OctiNetworkClient *netClient)
{
    _netClient = netClient;
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
    if (_selectedRoomId.empty() && !_tables.empty()) {
        _selectedRoomId = _tables.front().roomId;
    }
    rebuildPlayersTable();
    rebuildTablesWidget();
    rebuildChatTranscript();
    if (_statusLabel) {
        _statusLabel->setText(
            QString("%1 online player(s) • %2 active table(s)")
                .arg(static_cast<int>(_players.size()))
                .arg(static_cast<int>(_tables.size())));
    }
}

void LobbyView::handleLobbySseEvent(const SseEvent &ev)
{
    Json j = Json::parse(ev.dataJson, nullptr, false);
    if (ev.event == "LOBBY_CHAT" && j.is_object()) {
        LobbyChatEntry entry;
        entry.seq = j.value("seq", static_cast<std::int64_t>(ev.id));
        entry.atMs = j.value("at_ms", static_cast<std::int64_t>(0));
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
    if (!_netClient || text.empty()) {
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
    _chatMessages.push_back(entry);
    if (_chatTranscript) {
        _chatTranscript->appendPlainText(
            QString("[%1]: %2")
                .arg(QString::fromStdString(
                    entry.handle.empty() ? "Player" : entry.handle))
                .arg(QString::fromStdString(entry.text)));
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

void LobbyView::setOnObserveTableRequested(
    std::function<void(const TableSummaryInfo &)> cb)
{
    _onObserveTableRequested = std::move(cb);
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
        _playersTable->setItem(
            i, 0, new QTableWidgetItem(QString::fromStdString(p.handle)));
        _playersTable->setItem(
            i, 1, new QTableWidgetItem(QString::fromStdString(p.status)));
        _playersTable->setItem(
            i, 2, new QTableWidgetItem(
                p.roomId.empty() ? QString("—") : QString::fromStdString(p.roomId)));
    }
}

void LobbyView::rebuildTablesWidget()
{
    if (!_tablesTable) {
        return;
    }
    _tablesTable->setRowCount(static_cast<int>(_tables.size()));
    for (int i = 0; i < static_cast<int>(_tables.size()); ++i) {
        const TableSummaryInfo &t = _tables[static_cast<std::size_t>(i)];
        QString variant = QString("%1 • %2B%3%4")
            .arg(QString::fromStdString(t.mode))
            .arg(t.bases)
            .arg(t.superProngs ? " +!" : "")
            .arg(t.edgeless ? " +Wrap" : "");
        QString seats = QString("%1/%2").arg(t.seatsFilled).arg(t.seatsTotal);
        QString access = t.hasPasscode ? QString("Locked [Passcode]")
                                       : QString("Open");
        _tablesTable->setItem(
            i, 0, new QTableWidgetItem(QString::fromStdString(t.roomId)));
        _tablesTable->setItem(
            i, 1, new QTableWidgetItem(variant));
        _tablesTable->setItem(
            i, 2, new QTableWidgetItem(QString::fromStdString(t.phase)));
        _tablesTable->setItem(
            i, 3, new QTableWidgetItem(seats));
        _tablesTable->setItem(
            i, 4, new QTableWidgetItem(QString::number(t.observerCount)));
        _tablesTable->setItem(
            i, 5, new QTableWidgetItem(access));
        _tablesTable->setItem(
            i, 6, new QTableWidgetItem(QString::fromStdString(t.hostHandle)));
    }
}

void LobbyView::rebuildChatTranscript()
{
    if (!_chatTranscript) {
        return;
    }
    _chatTranscript->clear();
    for (std::size_t i = 0; i < _chatMessages.size(); ++i) {
        const LobbyChatEntry &c = _chatMessages[i];
        _chatTranscript->appendPlainText(
            QString("[%1]: %2")
                .arg(QString::fromStdString(
                    c.handle.empty() ? "Player" : c.handle))
                .arg(QString::fromStdString(c.text)));
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
