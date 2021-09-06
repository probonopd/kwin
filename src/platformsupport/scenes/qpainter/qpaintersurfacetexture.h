/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "surface.h"

#include <QImage>

namespace KWin
{

class QPainterBackend;

class KWIN_EXPORT QPainterSurfaceTexture : public SurfaceTexture
{
public:
    explicit QPainterSurfaceTexture(QPainterBackend *backend);

    bool isValid() const;

    QPainterBackend *backend() const;
    QImage image() const;

    virtual bool create() = 0;
    virtual void update(const QRegion &region) = 0;

protected:
    QPainterBackend *m_backend;
    QImage m_image;
};

} // namespace KWin
