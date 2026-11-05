/*
 * OctiReservePanel.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_RESERVE_PANEL_HXX
#define OCTI_DESKTOP_OCTI_RESERVE_PANEL_HXX

#include <array>
#include <functional>
#include <map>
#include <string>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QWidget>
#include "OctiBoard.hxx"
#include "OctiBoardCanvas.hxx"
#include "OctiMoveComposer.hxx"
#include "OctiSoundSynth.hxx"
#include "OctiTheme.hxx"

namespace octi {

struct ReserveSeatMetadata {
    std::string handle;
    int rating = 0;
    bool isRobot = false;
    bool connected = true;
    bool eliminated = false;
};

class OctiReservePanel : public QWidget {
public:
    explicit OctiReservePanel(QWidget *parent = nullptr);
    ~OctiReservePanel() override;

    void attach(OctiBoard *board,
                OctiMoveComposer *composer,
                OctiBoardCanvas *canvas = nullptr,
                OctiSoundSynth *soundSynth = nullptr);

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    void refresh();
    QImage renderToImage(int width = 340, int height = 720);
    void paintReserveScene(QPainter &p, int width, int height);

    void setPushToTalkCallback(std::function<void(bool talking)> cb);
    void setPushToTalkActive(bool talking);
    bool isPushToTalkActive() const;

    void setVoiceMutedByRoom(bool muted);
    bool isVoiceMutedByRoom() const;

    static QString formatClockMs(long ms);

    QRectF freePodWellRect() const;
    QRectF liberatePodWellRect() const;
    QRectF capturedPodWellRect(OctiPlayer player) const;
    QRectF normalProngWellRect() const;
    QRectF superProngWellRect() const;

    QRectF playerCardRect(OctiPlayer player) const;
    int iconGlyphCountRendered() const { return _iconGlyphCountRendered; }

    void setTimeControlEnabled(bool enabled);
    bool isTimeControlEnabled() const;
    void setSeatMetadata(int seat, const std::string &handle, int rating,
                         bool isRobot, bool connected, bool eliminated);
    void clearSeatMetadata();
    const ReserveSeatMetadata *seatMetadata(int seat) const;

    void setOnlineWaitingSeatState(bool waitingRoom,
                                   int viewerSeat,
                                   const std::array<bool, 5> &seatOccupied,
                                   std::function<void(int seat)> onSitSeatRequested);
    QRectF sitSeatButtonRect(int seat) const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    OctiBoard *_board;
    OctiMoveComposer *_composer;
    OctiBoardCanvas *_canvas;
    OctiSoundSynth *_soundSynth;
    ThemeId _theme;

    bool _pttActive;
    bool _voiceMutedByRoom;
    bool _timeControlEnabled;
    std::map<int, ReserveSeatMetadata> _seatMetadata;
    std::function<void(bool talking)> _pttCallback;

    QRectF _freePodWellRect;
    QRectF _liberatePodWellRect;
    QRectF _normalProngWellRect;
    QRectF _superProngWellRect;
    QRectF _pttButtonRect;
    QRectF _playerCardRects[5];
    QRectF _capturedPodWellRects[5];
    QRectF _sitSeatBtnRects[5];
    bool _onlineWaitingRoom = false;
    int _onlineViewerSeat = 0;
    std::array<bool, 5> _onlineSeatOccupied = {false, false, false, false, false};
    std::function<void(int seat)> _onSitSeatRequested;
    int _iconGlyphCountRendered = 0;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_RESERVE_PANEL_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
