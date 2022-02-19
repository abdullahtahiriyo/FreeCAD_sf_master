/***************************************************************************
 *   Copyright (c) 2022 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#include <boost_bind_bind.hpp>
#endif

#include "ui_TaskSketcherTool.h"
#include "TaskSketcherTool.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <QEvent>

#include "ViewProviderSketch.h"

#include "SketcherToolDefaultWidget.h"

using namespace SketcherGui;
using namespace Gui::TaskView;
namespace bp = boost::placeholders;

TaskSketcherTool::TaskSketcherTool(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Tool settings"),true, 0)
    , sketchView(sketchView)
{
    widget = std::make_unique<SketcherToolDefaultWidget> (this, sketchView);

    this->groupLayout()->addWidget(widget.get());

    Gui::Selection().Attach(this);

    Gui::Application* app = Gui::Application::Instance;
    changedSketchView = app->signalChangedObject.connect(boost::bind
        (&TaskSketcherTool::onChangedSketchView, this, bp::_1, bp::_2));
}

TaskSketcherTool::~TaskSketcherTool()
{
    Gui::Selection().Detach(this);
    //Content.erase(Content.begin());
}

void TaskSketcherTool::onChangedSketchView(const Gui::ViewProvider& vp,
                                              const App::Property& prop)
{
    Q_UNUSED(prop)
    if (sketchView == &vp) {
    }
}

void TaskSketcherTool::toolChanged(const std::string & toolname)
{
    // TODO: Implement a factory here to get an appropriate widget from the toolname

    // At this stage, we add a Default tool widget for all tools with a defined name, but this needs to change
    if( toolname != "DSH_None" ) {
        widget = std::make_unique<SketcherToolDefaultWidget> (this, sketchView);

        this->groupLayout()->addWidget(widget.get());

        signalToolWidgetChanged(this->widget.get());
    }
    else {
        signalToolWidgetChanged(nullptr);
    }
}



/// @cond DOXERR
void TaskSketcherTool::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                   Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(Reason);
    //if (Reason.Type == SelectionChanges::AddSelection ||
    //    Reason.Type == SelectionChanges::RmvSelection ||
    //    Reason.Type == SelectionChanges::SetSelection ||
    //    Reason.Type == SelectionChanges::ClrSelection) {
    //}
}
/// @endcond DOXERR


#include "moc_TaskSketcherTool.cpp"
