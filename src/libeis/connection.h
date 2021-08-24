/*
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "kwinglobals.h"

#include <memory>
#include <QMutex>

extern "C" {
struct eis;
eis *eis_unref(eis *e);
}

namespace KWin
{
namespace LibEis
{

class KWIN_EXPORT Connection : public QObject
{
    Q_OBJECT

public:
    ~Connection() override;
    static Connection *create();
Q_SIGNALS:
    void pointerMoved(const QSizeF &delta);
    void pointerPositionChanged(const QPointF &pos);
    void pointerButtonPressed(quint32 button);
    void pointerButtonReleased(quint32 button);
    void pointerScroll(const QSizeF &delta);
    void pointerScrollDiscrete(const QSizeF &clicks);
    void keyboardKeyPressed(quint32 key);
    void keyboardKeyReleased(quint32 key);
    void touchDown(quint32 id, const QPointF &pos);
    void touchMotion(quint32 id, const QPointF &pos);
    void touchUp(quint32 id);
private Q_SLOTS:
    void handleEvents();
private:
    Connection();
    QMutex m_eventQueueMutex;
    std::unique_ptr<QThread> m_thread;
    std::unique_ptr<eis, decltype(&eis_unref)> m_eis;
};

}
}
