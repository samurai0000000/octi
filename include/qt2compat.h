#ifndef _QT2COMPAT_H_
#define _QT2COMPAT_H_

/*
 * Small Qt 2 → Qt 5 compatibility helpers used by the OCTI GUI.
 */

#include <QWidget>
#include <QDialog>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QPolygon>
#include <QTransform>
#include <QDrag>
#include <QMimeData>
#include <QDropEvent>
#include <QMouseEvent>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QIcon>
#include <QString>
#include <QPoint>
#include <QColor>
#include <QtGlobal>
#include <QObject>

typedef Qt::WindowFlags WFlags;
typedef QPolygon QPointArray;
typedef QTransform QWMatrix;

#ifndef QABS
#define QABS qAbs
#endif

inline void bitBlt(QPaintDevice *dst, int x, int y, const QPixmap *src)
{
    QPainter p(dst);
    p.drawPixmap(x, y, *src);
}

inline QColor qt2BackgroundColor(const QWidget *w)
{
    return w->palette().color(w->backgroundRole());
}

inline void qt2InitWidget(QWidget *w, const char *name)
{
    if(name) {
        w->setObjectName(QString::fromLatin1(name));
    }
}

class QTextDrag {

public:

    QTextDrag(const QString &text, QWidget *source)
        : _text(text), _source(source) {
    }

    void setPixmap(const QPixmap &pm, const QPoint &hotspot = QPoint()) {
        _pixmap = pm;
        _hotspot = hotspot;
    }

    bool dragMove() {
        QDrag *drag = new QDrag(_source);
        QMimeData *mime = new QMimeData;
        mime->setText(_text);
        drag->setMimeData(mime);
        if(!_pixmap.isNull()) {
            drag->setPixmap(_pixmap);
            drag->setHotSpot(_hotspot);
        }
        return drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction)
            != Qt::IgnoreAction;
    }

    static bool canDecode(const QDropEvent *e) {
        return e && e->mimeData() && e->mimeData()->hasText();
    }

    static bool decode(const QDropEvent *e, QString &s) {
        if(!canDecode(e)) {
            return false;
        }
        s = e->mimeData()->text();
        return true;
    }

private:

    QString _text;
    QWidget *_source;
    QPixmap _pixmap;
    QPoint _hotspot;

};

inline QToolButton *qt2ToolButton(const QPixmap &pm, const QString &text,
                                  QObject *receiver, const char *slot,
                                  QToolBar *toolbar)
{
    QToolButton *b = new QToolButton(toolbar);
    b->setIcon(QIcon(pm));
    b->setToolTip(text);
    QObject::connect(b, SIGNAL(clicked()), receiver, slot);
    toolbar->addWidget(b);
    return b;
}

#endif
