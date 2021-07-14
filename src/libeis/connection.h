/*
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "client.h"
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
