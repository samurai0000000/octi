/*
 * LobbyView.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_LOBBY_VIEW_HXX
#define OCTI_DESKTOP_LOBBY_VIEW_HXX

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <QFrame>
#include <QGroupBox>
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

    void setViewerIdentity(const std::string &playerUuid,
                           const std::string &handle);
    void setActiveRoomState(bool inRoom,
                            const std::string &roomId,
                            int viewerSeat,
                            const GameViewData &gameView);

    bool refreshLobby();
    void refreshFromLobbyState(const LobbyStateData &state);
    void handleLobbySseEvent(const SseEvent &ev);

    bool sendLobbyChatMessage(const std::string &text);
    void appendLobbyChatEntry(const LobbyChatEntry &entry);

    static QString formatChatTimestamp(std::int64_t atMs);
    static QString formatChatMessageLine(const LobbyChatEntry &entry);
    static QString formatPhaseDisplay(const std::string &phase);
    static QString formatSeatOccupancySummary(const TableSummaryInfo &table);
    static QString formatSeatBreakdownText(const TableSummaryInfo &table);

    const std::vector<TableSummaryInfo> &tables() const;
    const std::vector<LobbyPlayerInfo> &players() const;
    const std::vector<LobbyChatEntry> &chatMessages() const;
    std::string chatTranscriptText() const;

    std::string selectedRoomId() const;
    void selectRoomId(const std::string &roomId);
    const TableSummaryInfo *findTable(const std::string &roomId) const;
    bool isViewerSeatedAtTable(const TableSummaryInfo &table) const;

    bool isMyTableBannerVisible() const;
    std::string activeBannerRoomId() const;

    void setOnCreateTableRequested(std::function<void()> cb);
    void setOnJoinTableRequested(
        std::function<void(const TableSummaryInfo &)> cb);
    void setOnLeaveTableRequested(
        std::function<void(const std::string &roomId)> cb);
    void setOnObserveTableRequested(
        std::function<void(const TableSummaryInfo &)> cb);
    void setOnOpenActiveBoardRequested(std::function<void()> cb);
    void setOnMatchmakingRequested(std::function<void()> cb);
    void setOnMatchFoundCallback(
        std::function<void(const std::string &roomId)> cb);

    void updateAuthUiState();

    QFrame *myTableBanner() const { return _myTableBanner; }
    QLabel *myTableBannerLabel() const { return _myTableBannerLabel; }
    QLabel *myTableSeatsLabel() const { return _myTableSeatsLabel; }
    QPushButton *openActiveBoardButton() const { return _openActiveBoardBtn; }
    QPushButton *leaveActiveTableButton() const { return _leaveActiveTableBtn; }

    QPushButton *createTableButton() const { return _createTableBtn; }
    QPushButton *joinTableButton() const { return _joinSelectedBtn; }
    QPushButton *joinButton() const { return _joinSelectedBtn; }
    QPushButton *leaveTableButton() const { return _leaveTableBtn; }
    QPushButton *observeTableButton() const { return _observeSelectedBtn; }
    QPushButton *observeButton() const { return _observeSelectedBtn; }
    QPushButton *matchmakingButton() const { return _matchmakingBtn; }
    QPushButton *quickMatchButton() const { return _matchmakingBtn; }
    QPushButton *refreshButton() const { return _refreshBtn; }
    QTableWidget *playersTable() const { return _playersTable; }
    QTableWidget *tablesTable() const { return _tablesTable; }
    QGroupBox *inspectorBox() const { return _inspectorBox; }
    QLabel *inspectorTitleLabel() const { return _inspectorTitleLabel; }
    QLabel *inspectorSeatsLabel() const { return _inspectorSeatsLabel; }
    QPlainTextEdit *chatTranscript() const { return _chatTranscript; }
    QLineEdit *chatInput() const { return _chatInput; }
    QPushButton *chatSendButton() const { return _sendChatBtn; }
    QLabel *statusLabel() const { return _statusLabel; }

private:
    void rebuildPlayersTable();
    void rebuildTablesWidget();
    void rebuildChatTranscript();
    void updateInspectorAndSelectionControls();
    void updateMyTableBanner();

    OctiNetworkClient *_netClient;
    ThemeId _theme;

    std::string _viewerUuid;
    std::string _viewerHandle;
    bool _inActiveRoom = false;
    std::string _activeRoomId;
    int _activeViewerSeat = 0;
    GameViewData _activeGameView;
    std::string _bannerRoomId;

    QFrame *_myTableBanner;
    QLabel *_myTableBannerLabel;
    QLabel *_myTableSeatsLabel;
    QPushButton *_openActiveBoardBtn;
    QPushButton *_leaveActiveTableBtn;

    QTableWidget *_playersTable;
    QTableWidget *_tablesTable;
    QGroupBox *_inspectorBox;
    QLabel *_inspectorTitleLabel;
    QLabel *_inspectorSeatsLabel;
    QPlainTextEdit *_chatTranscript;
    QLineEdit *_chatInput;
    QPushButton *_sendChatBtn;
    QPushButton *_createTableBtn;
    QPushButton *_joinSelectedBtn;
    QPushButton *_leaveTableBtn;
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
    std::function<void(const std::string &roomId)> _onLeaveTableRequested;
    std::function<void(const TableSummaryInfo &)> _onObserveTableRequested;
    std::function<void()> _onOpenActiveBoardRequested;
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
