/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef SketcherGui_DrawSketchKeyboardManager_H
#define SketcherGui_DrawSketchKeyboardManager_H


#include <QEvent>
#include <QKeyEvent>

#include <QTimer>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>


namespace Gui
{
class ViewProvider;
}  // namespace Gui

namespace SketcherGui
{

class ViewProviderSketch;

/** Class to decide which control is responsible of handling an key event
 *using timers, type of entered event, ...
 */
class DrawSketchKeyboardManager: public QObject
{
    Q_OBJECT

public:
    DrawSketchKeyboardManager();


    /// Indicates whether the widget should handle keyboard input or should signal it via boost
    enum class KeyboardEventHandlingMode
    {
        Widget,
        ViewProvider
    };

    bool isMode(KeyboardEventHandlingMode mode);
    KeyboardEventHandlingMode getMode();

    bool eventFilter(QObject* object, QEvent* event);

private:
    /// This function decides whether events should be send to the ViewProvider
    /// or to the UI control of the Default widget.
    void detectKeyboardEventHandlingMode(QKeyEvent* keyEvent);

    void onTimeOut();

private:
    /// Viewer responsible for the active document
    Gui::View3DInventorViewer* vpViewer = nullptr;
    KeyboardEventHandlingMode keyMode;

    QTimer timer;

    const int timeOut = 1000;
};

}  // namespace SketcherGui

#endif  // SketcherGui_DrawSketchKeyboardManager_H
