/****************************************************************************
**
** Copyright (C) 2017-2020 Elros https://github.com/elros34
**               2020 Rinigus https://github.com/rinigus
**               2012 Digia Plc and/or its subsidiary(-ies).
**
** This file is part of Flatpak Runner.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of the copyright holder nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

#include "qmlcompositor.h"
#include <QGuiApplication>
#include <QQuickView>
#include <QWaylandSurfaceInterface>
#include <QtCompositor/qwaylandoutput.h>

#include <QDebug>

QmlCompositor::QmlCompositor(QQuickView *quickView, const char *socketName, qreal renderScale, int refreshRate)
    : QObject(quickView)
    , QWaylandQuickCompositor(socketName, DefaultExtensions | SubSurfaceExtension)
    , m_fullscreenSurface(0),
      m_window(quickView),
      m_renderScale(renderScale)
{
    addDefaultShell();
    createOutput(m_window, "", "");
    // wl_shell maximize answers with the output's available geometry; the
    // scaled size must be there before Android's first window appears
    QSize size = quickView->size();
    setSize(size.width(), size.height());
    if (refreshRate > 0)
        setOutputRefreshRate(refreshRate * 1000);

    connect(m_window, &QWindow::heightChanged, this, &QmlCompositor::sizeChanged);
    connect(m_window, &QWindow::widthChanged, this, &QmlCompositor::sizeChanged);
}

QWaylandQuickSurface *QmlCompositor::fullscreenSurface() const
{
    return m_fullscreenSurface;
}

void QmlCompositor::sizeChanged(int)
{
    QSize size = m_window->size();
    setSize(size.width(), size.height());
}

void QmlCompositor::setSize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    QRect geometry(0, 0, qRound(width * m_renderScale), qRound(height * m_renderScale));
    qDebug() << "Available geometry" << geometry.size() << "from window" << width << "x" << height;
    // setOutputGeometry()/setMode() also resize the window, which feeds back
    // into sizeChanged(); the available geometry leaves the window alone
    if (primaryOutput())
        primaryOutput()->setAvailableGeometry(geometry);
}

qreal QmlCompositor::renderScale() const
{
    return m_renderScale;
}

QWaylandSurfaceItem *QmlCompositor::item(QWaylandSurface *surf)
{
    return static_cast<QWaylandSurfaceItem *>(surf->views().first());
}

void QmlCompositor::destroyWindow(QVariant window)
{
    qvariant_cast<QObject *>(window)->deleteLater();
}

void QmlCompositor::setFullscreenSurface(QWaylandQuickSurface *surface)
{
    if (surface == m_fullscreenSurface)
        return;
    m_fullscreenSurface = surface;
    emit fullscreenSurfaceChanged();
}

void QmlCompositor::surfaceMapped()
{
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());
    emit windowAdded(QVariant::fromValue(surface));
}
void QmlCompositor::surfaceUnmapped()
{
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());
    if (surface == m_fullscreenSurface) {
        m_fullscreenSurface = 0;
        emit fullscreenSurfaceChanged();
      }
}

void QmlCompositor::surfaceDestroyed(QObject *object)
{
    QWaylandQuickSurface *surface = static_cast<QWaylandQuickSurface *>(object);
    if (surface == m_fullscreenSurface) {
        m_fullscreenSurface = 0;
        emit fullscreenSurfaceChanged();
      }
}

void QmlCompositor::sendCallbacks()
{
    if (m_fullscreenSurface)
        sendFrameCallbacks(QList<QWaylandSurface *>() << m_fullscreenSurface);
    else
        sendFrameCallbacks(surfaces());
}

void QmlCompositor::surfaceCreated(QWaylandSurface *surface)
{
    connect(surface, SIGNAL(destroyed(QObject *)), this, SLOT(surfaceDestroyed(QObject *)));
    connect(surface, SIGNAL(mapped()), this, SLOT(surfaceMapped()));
    connect(surface, SIGNAL(unmapped()), this,SLOT(surfaceUnmapped()));
}

