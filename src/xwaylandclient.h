/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "x11client.h"

namespace KWin
{

/**
 * The XwaylandClient class represents a managed Xwayland client.
 */
class XwaylandClient : public X11Client
{
    Q_OBJECT

public:
    explicit XwaylandClient();

    bool wantsSyncCounter() const override;
    Surface *createSceneSurface() override;

private:
    void associate();
    void initialize();
};

} // namespace KWin
