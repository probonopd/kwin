/*
    SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "connection.h"

#include "main.h"
#include "libeis_logging.h"

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

static void eis_log_handler(eis *eis, eis_log_priority priority, const char *message, bool is_continuation)
{
    switch (priority) {
    case EIS_LOG_PRIORITY_DEBUG:
        qCDebug(KWIN_EIS) << message;
        break;
    case EIS_LOG_PRIORITY_INFO:
        qCInfo(KWIN_EIS) << message;
        break;
    case EIS_LOG_PRIORITY_WARNING:
        qCWarning(KWIN_EIS) << message;
        break;
    case EIS_LOG_PRIORITY_ERROR:
        qCritical(KWIN_EIS) << message;
        break;
    }
}

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

    constexpr int maxSocketNumber = 32;
    QByteArray socketName;
    int socketNum = 0;
    do {
        if (socketNum == maxSocketNumber) {
            return;
        }
        socketName = QByteArrayLiteral("eis-") + QByteArray::number(socketNum++);
    } while (eis_setup_backend_socket(m_eis.get(), socketName));

    qputenv("LIBEI_SOCKET", socketName);
    auto env = kwinApp()->processStartupEnvironment();
    env.insert("LIBEI_SOCKET", socketName);
    static_cast<ApplicationWaylandAbstract*>(kwinApp())->setProcessStartupEnvironment(env);

    const auto fd = eis_get_fd(m_eis.get());
    auto m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    m_thread->start();
    moveToThread(m_thread.get());
    connect(m_notifier, &QSocketNotifier::activated, this, &Connection::handleEvents);

    eis_log_set_priority(m_eis.get(), EIS_LOG_PRIORITY_DEBUG);
    eis_log_set_handler(m_eis.get(), eis_log_handler);
}

void Connection::handleEvents()
{
    qCDebug(KWIN_EIS) << "got events";
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
                qCDebug(KWIN_EIS) << "new client" << eis_client_get_name(client);
                break;
            }
            case EIS_EVENT_CLIENT_DISCONNECT: {
                auto client = eis_event_get_client(event);
                qCDebug(KWIN_EIS) << "client disconnected" << eis_client_get_name(client);
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
                qCDebug(KWIN_EIS) << "new device" << eis_device_get_name(device) << "from" << eis_client_get_name(eis_event_get_client(event));
                break;
            }
            case EIS_EVENT_DEVICE_REMOVED:
                eis_device_disconnect(eis_event_get_device(event));
                qCDebug(KWIN_EIS) << "device removed" << eis_device_get_name(eis_event_get_device(event));
                break;
            case EIS_EVENT_POINTER_MOTION: {
                const auto x = eis_event_pointer_get_dx(event);
                const auto y = eis_event_pointer_get_dy(event);
                qCDebug(KWIN_EIS) <<  eis_client_get_name(eis_event_get_client(event)) << "pointer motion" << x << y;
                Q_EMIT pointerMoved({x, y});
                break;
            }
            case EIS_EVENT_POINTER_MOTION_ABSOLUTE: {
                const auto x = eis_event_pointer_get_absolute_x(event);
                const auto y = eis_event_pointer_get_absolute_y(event);
                qCDebug(KWIN_EIS) <<  eis_client_get_name(eis_event_get_client(event)) << "pointer motion absolute" << x << y;
                Q_EMIT pointerPositionChanged({x, y});
                break;
            }
            case EIS_EVENT_POINTER_BUTTON: {
                const auto button = eis_event_pointer_get_button(event);
                const auto press = eis_event_pointer_get_button_is_press(event);
                if (press) {
                    qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event)) << "pointer press" << button;
                    Q_EMIT pointerButtonPressed(button);
                } else {
                    qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event)) << "pointer release" << button;
                    Q_EMIT pointerButtonReleased(button);
                }
                break;
            }
            case EIS_EVENT_POINTER_SCROLL: {
                const auto x = eis_event_pointer_get_scroll_x(event);
                const auto y = eis_event_pointer_get_scroll_y(event);
                qCDebug(KWIN_EIS) <<  eis_client_get_name(eis_event_get_client(event)) << "pointer scroll" << x << y;
                Q_EMIT pointerScroll({x, y});
                break;
            }
            case EIS_EVENT_POINTER_SCROLL_DISCRETE: {
                const auto x = eis_event_pointer_get_scroll_discrete_x(event);
                const auto y = eis_event_pointer_get_scroll_discrete_y(event);
                qCDebug(KWIN_EIS) <<  eis_client_get_name(eis_event_get_client(event)) << "pointer scroll discrete" << x << y;
                Q_EMIT pointerScrollDiscrete({x / 120.0, y / 120.0});
                break;
            }
            case EIS_EVENT_KEYBOARD_KEY: {
                const auto key = eis_event_keyboard_get_key(event);
                const auto press = eis_event_keyboard_get_key_is_press(event);
                if (press) {
                    qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event))  << "key press" << key;
                    Q_EMIT keyboardKeyPressed(key);
                } else {
                    qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event))  << "key release" << key;
                    Q_EMIT keyboardKeyReleased(key);
                }
                break;
            }
            case EIS_EVENT_TOUCH_DOWN: {
                const auto x = eis_event_touch_get_x(event);
                const auto y = eis_event_touch_get_y(event);
                const auto id = eis_event_touch_get_id(event);
                qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event)) << "touch down" << id << x << y;
                Q_EMIT touchDown(id, {x, y});
                break;
            }
            case EIS_EVENT_TOUCH_UP: {
                const auto id = eis_event_touch_get_id(event);
                qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event)) << "touch up" << id;
                Q_EMIT touchUp(id);
                break;
            }
            case EIS_EVENT_TOUCH_MOTION: {
                const auto x = eis_event_touch_get_x(event);
                const auto y = eis_event_touch_get_y(event);
                const auto id = eis_event_touch_get_id(event);
                qCDebug(KWIN_EIS) << eis_client_get_name(eis_event_get_client(event)) << "touch move" << id << x << y;
                Q_EMIT touchMotion(id, {x, y});
                break;
            }
        }
    }
}

}
}


