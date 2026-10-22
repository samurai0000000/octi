/*
 * LobbyView.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_LOBBY_VIEW_HXX
#define OCTI_DESKTOP_LOBBY_VIEW_HXX

#include <functional>
#include <string>
#include <vector>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include "OctiNetworkClient.hxx"
#include "OctiTheme.hxx"

namespace octi {

class LobbyView : public QWidget {
public:
    explicit LobbyView(OctiNetworkClient *netClient = nullptr,
                       ThemeId theme = ThemeId::OBSIDIAN_BRASS,
                       QWidget *parent = nullptr);
    ~LobbyView() override;

    void attachNetworkClient(OctiNetworkClient *netClient);
    OctiNetworkClient *networkClient() const;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    bool refreshLobby();
    void refreshFromLobbyState(const LobbyStateData &state);
    void handleLobbySseEvent(const SseEvent &ev);

    bool sendLobbyChatMessage(const std::string &text);
    void appendLobbyChatEntry(const LobbyChatEntry &entry);

    const std::vector<TableSummaryInfo> &tables() const;
    const std::vector<LobbyPlayerInfo> &players() const;
    const std::vector<LobbyChatEntry> &chatMessages() const;

    std::string selectedRoomId() const;
    void selectRoomId(const std::string &roomId);
    const TableSummaryInfo *findTable(const std::string &roomId) const;

    void setOnCreateTableRequested(std::function<void()> cb);
    void setOnJoinTableRequested(
        std::function<void(const TableSummaryInfo &)> cb);
    void setOnObserveTableRequested(
        std::function<void(const TableSummaryInfo &)> cb);
    void setOnMatchmakingRequested(std::function<void()> cb);
    void setOnMatchFoundCallback(
        std::function<void(const std::string &roomId)> cb);

private:
    void rebuildPlayersTable();
    void rebuildTablesWidget();
    void rebuildChatTranscript();

    OctiNetworkClient *_netClient;
    ThemeId _theme;

    QTableWidget *_playersTable;
    QTableWidget *_tablesTable;
    QPlainTextEdit *_chatTranscript;
    QLineEdit *_chatInput;
    QPushButton *_sendChatBtn;
    QPushButton *_createTableBtn;
    QPushButton *_joinSelectedBtn;
    QPushButton *_observeSelectedBtn;
    QPushButton *_matchmakingBtn;
    QPushButton *_refreshBtn;
    QLabel *_statusLabel;

    std::vector<LobbyPlayerInfo> _players;
    std::vector<TableSummaryInfo> _tables;
    std::vector<LobbyChatEntry> _chatMessages;
    std::string _selectedRoomId;

    std::function<void()> _onCreateTableRequested;
    std::function<void(const TableSummaryInfo &)> _onJoinTableRequested;
    std::function<void(const TableSummaryInfo &)> _onObserveTableRequested;
    std::function<void()> _onMatchmakingRequested;
    std::function<void(const std::string &roomId)> _onMatchFoundCallback;
};

} // namespace octi

#endif // OCTI_DESKTOP_LOBBY_VIEW_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
