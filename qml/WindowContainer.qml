/****************************************************************************
**
** Copyright (C) 2017-2020 Elros https://github.com/elros34
**               2020-2021 Rinigus https://github.com/rinigus
**               2012 Digia Plc and/or its subsidiary(-ies).
**
** This file is part of Waydroid Runner.
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

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtQuick.Window 2.0
import QtCompositor 1.0

MouseArea {
    id: container
    objectName: "windowContainer"
    anchors.fill: parent
    z: 1

    property variant child: null // qwaylandsurfaceitem
    property bool popup: false

    Component.onCompleted: {
        child.smooth = true
        if (popup) {
            child.transformOrigin = Item.TopLeft
            child.scale = 1.0 / compositor.renderScale
            child.parent = popupContainer;
            popupContainer.width = child.width * child.scale
            popupContainer.height = child.height * child.scale
        } else {
            // QWaylandSurfaceItem resizes itself to every committed buffer;
            // only anchors keep it pinned, so the scale lives on the parent.
            child.parent = scaler;
            child.resizeSurfaceToItem = true
            child.anchors.fill = scaler
            child.touchEventsEnabled = true
            child.takeFocus()
        }
        console.log("Window added: popup", popup, "surface", child.surface.size.width, "x", child.surface.size.height,
                    "scaler", scaler.width, "x", scaler.height, "@", scaler.scale, "container", container.width, "x", container.height,
                    "item", child.width, "x", child.height, "scale", compositor.renderScale)
    }

    function close() {
        visible = false
        container.parent.removeWindow(container)
    }

    onPressed: {
        if (popup) {
            close()
        }
    }

    onFocusChanged: {
        if (popup && !focus) {
            close()
        }
    }

    Connections {
        target: container.child ? container.child.surface : null

        onUnmapped: close()
        onSurfaceDestroyed: close()
        onSizeChanged: console.log("Surface size", child.surface.size.width, "x", child.surface.size.height,
                                   "item", child.width, "x", child.height, "parent", child.parent, "scale", child.scale)
        onDamaged: {
            if (popup) {
                popupContainer.width = child.width * child.scale
                popupContainer.height = child.height * child.scale
            }
        }
    }

    Item {
        id: scaler
        transformOrigin: Item.TopLeft
        scale: 1.0 / compositor.renderScale
        width: Math.round(container.width * compositor.renderScale)
        height: Math.round(container.height * compositor.renderScale)
    }

    Item {
        id: popupContainer
        anchors.centerIn: parent
        width: 100
        height: 100
    }
}
