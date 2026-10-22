/*
 * OctiReservePanel.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_RESERVE_PANEL_HXX
#define OCTI_DESKTOP_OCTI_RESERVE_PANEL_HXX

#include <functional>
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
    QRectF normalProngWellRect() const;
    QRectF superProngWellRect() const;

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
    std::function<void(bool talking)> _pttCallback;

    QRectF _freePodWellRect;
    QRectF _liberatePodWellRect;
    QRectF _normalProngWellRect;
    QRectF _superProngWellRect;
    QRectF _pttButtonRect;
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
