/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "surface.h"

#include <xcb/damage.h>
#include <xcb/xfixes.h>

namespace KWin
{

class Deleted;
class Toplevel;

/**
 * The SurfaceX11 class represents the contents of an X11 window.
 */
class KWIN_EXPORT SurfaceX11 : public Surface
{
    Q_OBJECT

public:
    explicit SurfaceX11(Toplevel *window, QObject *parent = nullptr);

    void processDamage();
    bool fetchDamage();
    void waitForDamage();
    void destroyDamage();

    SurfacePixmap *createPixmap() override;

    QRegion shape() const override;
    QRegion opaque() const override;

private Q_SLOTS:
    void handleBufferGeometryChanged(Toplevel *toplevel, const QRect &old);

private:
    Toplevel *m_window;
    xcb_damage_damage_t m_damageHandle = XCB_NONE;
    xcb_xfixes_fetch_region_cookie_t m_damageCookie;
    bool m_isDamaged = false;
    bool m_havePendingDamageRegion = false;
};

class KWIN_EXPORT SurfacePixmapX11 final : public SurfacePixmap
{
    Q_OBJECT

public:
    explicit SurfacePixmapX11(SurfaceX11 *surface, QObject *parent = nullptr);
    ~SurfacePixmapX11() override;

    xcb_pixmap_t pixmap() const;
    xcb_visualid_t visual() const;

    void create() override;
    bool isValid() const override;

private:
    SurfaceX11 *m_surface;
    xcb_pixmap_t m_pixmap = XCB_PIXMAP_NONE;
};

} // namespace KWin
