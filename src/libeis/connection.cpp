/*
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "connection.h"

#include <QDebug>
#include <QSocketNotifier>
#include <QThread>

#include <libeis.h>

#include <linux/input-event-codes.h>
#include <mutex>

namespace KWin
{
namespace LibEis
{

Connection *Connection::create()
{
    static Connection self = Connection();
    return &self;
}

Connection::~Connection()
{
    m_thread->quit();
    m_thread->wait();
}

Connection::Connection()
    : m_thread(std::make_unique<QThread>())
    , m_eis(eis_new(nullptr), eis_unref)
{
    m_thread->setObjectName("libeis thread");

    int socketNum = 0;
//     while () )// (QByteArrayLiteral("kwin-eis-") + QByteArray::number(socketNum++)).operator QByteArray()) != 0)
//     {
//     }
    eis_setup_backend_socket(m_eis.get(), "eis-0");
    const auto fd = eis_get_fd(m_eis.get());
    qDebug() << "fd is" << fd;
    if (fd) {
        auto m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        m_thread->start();
        moveToThread(m_thread.get());
        connect(m_notifier, &QSocketNotifier::activated, this, &Connection::handleEvents);
    }
}

void Connection::handleEvents()
{
    qDebug() << "got events";
    eis_dispatch(m_eis.get());
    while (eis_event * const event = eis_get_event(m_eis.get())) {
        switch (eis_event_get_type(event)) {
            case EIS_EVENT_CLIENT_CONNECT: {
                auto client = eis_event_get_client(event);
                eis_client_connect(client);
                auto seat = eis_client_new_seat(client, "seat");
                eis_seat_allow_capability(seat, EIS_DEVICE_CAP_POINTER);
                eis_seat_allow_capability(seat, EIS_DEVICE_CAP_POINTER_ABSOLUTE);
                eis_seat_allow_capability(seat, EIS_DEVICE_CAP_KEYBOARD);
                eis_seat_allow_capability(seat, EIS_DEVICE_CAP_TOUCH);
                eis_seat_add(seat);
                qDebug() << "new client" << eis_client_get_name(client);
                break;
            }
            case EIS_EVENT_CLIENT_DISCONNECT: {
                auto client = eis_event_get_client(event);
                qDebug() << "client disconnected" << eis_client_get_name(client);
                eis_client_disconnect(eis_event_get_client(event));
                break;
            }
            case EIS_EVENT_DEVICE_ADDED: {
                auto device = eis_event_get_device(event);
                eis_device_allow_capability(device,  EIS_DEVICE_CAP_POINTER);
                eis_device_allow_capability(device,  EIS_DEVICE_CAP_POINTER_ABSOLUTE);
                eis_device_allow_capability(device,  EIS_DEVICE_CAP_KEYBOARD);
                eis_device_allow_capability(device,  EIS_DEVICE_CAP_TOUCH);
                eis_device_connect(device);
                eis_device_resume(device);
                qDebug() << "new device" << eis_device_get_name(device) << "from" << eis_client_get_name(eis_event_get_client(event));
                break;
            }
            case EIS_EVENT_DEVICE_REMOVED:
                eis_device_disconnect(eis_event_get_device(event));
                qDebug() << "device removed" << eis_device_get_name(eis_event_get_device(event));
                break;
            case EIS_EVENT_POINTER_MOTION: {
                const auto x = eis_event_pointer_get_dx(event);
                const auto y = eis_event_pointer_get_dy(event);
                qDebug() <<  eis_client_get_name(eis_event_get_client(event)) << "pointer motion" << x << y;
                Q_EMIT pointerMoved({x, y});
                break;
            }
            case EIS_EVENT_POINTER_MOTION_ABSOLUTE: {
                const auto x = eis_event_pointer_get_absolute_x(event);
                const auto y = eis_event_pointer_get_absolute_y(event);
                qDebug() <<  eis_client_get_name(eis_event_get_client(event)) << "pointer motion absolute" << x << y;
                Q_EMIT pointerPositionChanged({x, y});
                break;
            }
            case EIS_EVENT_POINTER_BUTTON: {
                const auto button = eis_event_pointer_get_button(event);
                const auto press = eis_event_pointer_get_button_is_press(event);
                if (press) {
                    qDebug() << eis_client_get_name(eis_event_get_client(event)) << "pointer press" << button;
                    Q_EMIT pointerButtonPressed(button);
                } else {
                    qDebug() << eis_client_get_name(eis_event_get_client(event)) << "pointer release" << button;
                    Q_EMIT pointerButtonReleased(button);
                }
                break;
            }
            case EIS_EVENT_POINTER_SCROLL: {
                const auto x = eis_event_pointer_get_scroll_x(event);
                const auto y = eis_event_pointer_get_scroll_y(event);
                qDebug() <<  eis_client_get_name(eis_event_get_client(event)) << "pointer scroll" << x << y;
                Q_EMIT pointerScroll({x, y});
                break;
            }
            case EIS_EVENT_POINTER_SCROLL_DISCRETE: {
                const auto x = eis_event_pointer_get_scroll_discrete_x(event);
                const auto y = eis_event_pointer_get_scroll_discrete_y(event);
                qDebug() <<  eis_client_get_name(eis_event_get_client(event)) << "pointer scroll discrete" << x << y;
                Q_EMIT pointerScrollDiscrete({x / 120.0, y / 120.0});
                break;
            }
            case EIS_EVENT_KEYBOARD_KEY: {
                const auto key = eis_event_keyboard_get_key(event);
                const auto press = eis_event_keyboard_get_key_is_press(event);
                if (press) {
                    qDebug() << eis_client_get_name(eis_event_get_client(event))  << "key press" << key;
                    Q_EMIT keyboardKeyPressed(key);
                } else {
                    qDebug() << eis_client_get_name(eis_event_get_client(event))  << "key release" << key;
                    Q_EMIT keyboardKeyReleased(key);
                }
                break;
            }
            case EIS_EVENT_TOUCH_DOWN: {
                const auto x = eis_event_touch_get_x(event);
                const auto y = eis_event_touch_get_y(event);
                const auto id = eis_event_touch_get_id(event);
                qDebug() << eis_client_get_name(eis_event_get_client(event)) << "touch down" << id << x << y;
                Q_EMIT touchDown(id, {x, y});
                break;
            }
            case EIS_EVENT_TOUCH_UP: {
                const auto id = eis_event_touch_get_id(event);
                qDebug() << eis_client_get_name(eis_event_get_client(event)) << "touch up" << id;
                Q_EMIT touchUp(id);
                break;
            }
            case EIS_EVENT_TOUCH_MOTION: {
                const auto x = eis_event_touch_get_x(event);
                const auto y = eis_event_touch_get_y(event);
                const auto id = eis_event_touch_get_id(event);
                qDebug() << eis_client_get_name(eis_event_get_client(event)) << "touch move" << id << x << y;
                Q_EMIT touchMotion(id, {x, y});
                break;
            }
        }
    }
}

}
}


