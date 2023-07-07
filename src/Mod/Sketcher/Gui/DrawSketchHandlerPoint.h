/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_DrawSketchHandlerPoint_H
#define SKETCHERGUI_DrawSketchHandlerPoint_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"

#include "DrawSketchController.h"
#include "DrawSketchControllableHandler.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPoint;

using DrawSketchHandlerPointController =
    DrawSketchController<DrawSketchHandlerPoint,
                         StateMachines::OneSeekEnd,
                         /*PAutoConstraintSize =*/1,
                         /*OnViewParametersT =*/OnViewParameters<2>,
                         /*WidgetParametersT =*/WidgetParameters<0>,
                         /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,
                         /*WidgetComboboxesT =*/WidgetComboboxes<0>>;


using DrawSketchHandlerPointBase = DrawSketchControllableHandler<DrawSketchHandlerPointController>;

class DrawSketchHandlerPoint: public DrawSketchHandlerPointBase
{
    friend DrawSketchHandlerPointController;  // allow controller specialisations
                                              // access DrawSketchHandlerRectangle private members
public:
    DrawSketchHandlerPoint() = default;
    virtual ~DrawSketchHandlerPoint() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                drawPositionAtCursor(onSketchPos);

                editPoint = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            } break;
            default:
                break;
        }
    }

    virtual void executeCommands() override
    {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch point"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                                  editPoint.x,
                                  editPoint.y);

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add point"));

            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override
    {

        if (!sugConstraints[0].empty()) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0],
                                                     getHighestCurveIndex(),
                                                     Sketcher::PointPos::start);
            sugConstraints[0].clear();
        }
    }

    virtual std::string getToolName() const override
    {
        return "DSH_Point";
    }

    virtual QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Point");
    }

    virtual std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

private:
    Base::Vector2d editPoint;
};

template<>
auto DrawSketchHandlerPointController::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DrawSketchHandlerPointController::configureToolWidget()
{
    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
}

template<>
void DrawSketchHandlerPointController::adaptDrawingToOnViewParameterChange(int labelindex,
                                                                           double value)
{
    switch (labelindex) {
        case OnViewParameter::First:
            handler->editPoint.x = value;
            break;
        case OnViewParameter::Second:
            handler->editPoint.y = value;
            break;
    }
    onViewParameters[OnViewParameter::First]->setPoints(
        Base::Vector3d(0., 0., 0.),
        Base::Vector3d(handler->editPoint.x, handler->editPoint.y, 0.));
    onViewParameters[OnViewParameter::Second]->setPoints(
        Base::Vector3d(0., 0., 0.),
        Base::Vector3d(handler->editPoint.x, handler->editPoint.y, 0.));
}

template<>
void DrawSketchHandlerPointController::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::First]->getValue();
            }

            if (onViewParameters[OnViewParameter::Second]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Second]->getValue();
            }
        } break;
        default:
            break;
    }
}

template<>
void DrawSketchHandlerPointController::adaptParameters(Base::Vector2d onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (!onViewParameters[OnViewParameter::First]->isSet) {
                onViewParameters[OnViewParameter::First]->setSpinboxValue(onSketchPos.x);
            }

            if (!onViewParameters[OnViewParameter::Second]->isSet) {
                onViewParameters[OnViewParameter::Second]->setSpinboxValue(onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            onViewParameters[OnViewParameter::First]->setLabelAutoDistanceReverse(!sameSign);
            onViewParameters[OnViewParameter::Second]->setLabelAutoDistanceReverse(sameSign);
            onViewParameters[OnViewParameter::First]->setPoints(
                Base::Vector3d(0., 0., 0.),
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
            onViewParameters[OnViewParameter::Second]->setPoints(
                Base::Vector3d(0., 0., 0.),
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));
        } break;
        default:
            break;
    }
}

template<>
void DrawSketchHandlerPointController::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet
                && onViewParameters[OnViewParameter::Second]->isSet) {

                handler->setState(SelectMode::End);
                // handler->finish(); // Called by the change of mode
            }
        } break;
        default:
            break;
    }
}

template<>
void DrawSketchHandlerPointController::addConstraints()
{
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start),
                               GeoElementId::RtPnt,
                               x0,
                               handler->sketchgui->getObject());
    }
    else {
        if (x0set) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start),
                                   GeoElementId::VAxis,
                                   x0,
                                   handler->sketchgui->getObject());
        }

        if (y0set) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start),
                                   GeoElementId::HAxis,
                                   y0,
                                   handler->sketchgui->getObject());
        }
    }
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerPoint_H
