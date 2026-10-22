/*
 * QtCompat.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_QT_COMPAT_HXX
#define OCTI_DESKTOP_QT_COMPAT_HXX

#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QTimer>
#include <QWheelEvent>
#include <QtGlobal>

namespace octi {

inline QPointF mousePosF(const QMouseEvent *e)
{
    if (!e) {
        return QPointF(0.0, 0.0);
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}

inline int wheelDeltaY(const QWheelEvent *e)
{
    if (!e) {
        return 0;
    }
    return e->angleDelta().y();
}

inline int textAdvance(const QFontMetrics &fm, const QString &s)
{
    return fm.horizontalAdvance(s);
}

class UiDispatcher : public QObject {
public:
    explicit UiDispatcher(QObject *parent = nullptr)
        : QObject(parent)
    {
        QObject::connect(&_timer, &QTimer::timeout, [this]() {
            drain();
        });
        _timer.start(20);
    }

    ~UiDispatcher() override
    {
        _timer.stop();
    }

    UiDispatcher(const UiDispatcher &) = delete;
    UiDispatcher &operator=(const UiDispatcher &) = delete;

    void post(std::function<void()> fn)
    {
        if (!fn) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push_back(std::move(fn));
        }
        if (QCoreApplication::instance()) {
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        }
    }

    void drain()
    {
        std::deque<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            batch.swap(_queue);
        }
        for (auto &fn : batch) {
            if (fn) {
                fn();
            }
        }
    }

    std::size_t pendingCount() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

protected:
    void customEvent(QEvent *event) override
    {
        Q_UNUSED(event);
        drain();
    }

private:
    mutable std::mutex _mutex;
    std::deque<std::function<void()>> _queue;
    QTimer _timer;
};

} // namespace octi

#endif // OCTI_DESKTOP_QT_COMPAT_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
