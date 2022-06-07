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


#ifndef SKETCHERGUI_DrawSketchHandlerEllipse_H
#define SKETCHERGUI_DrawSketchHandlerEllipse_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

namespace ConstructionMethods {

enum class EllipseConstructionMethod {
    Center,
    PeriapsisApoapsisMinorRadius,
    End // Must be the last one
};
}

/* Ellipse ==============================================================================*/
class DrawSketchHandlerEllipse;

using DrawSketchHandlerEllipseBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerEllipse,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5, 5>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::EllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerEllipse : public DrawSketchHandlerEllipseBase
{
    friend DrawSketchHandlerEllipseBase;

public:
    DrawSketchHandlerEllipse(ConstructionMethod constrMethod = ConstructionMethod::Center) :
        DrawSketchHandlerEllipseBase(constrMethod) {}
    virtual ~DrawSketchHandlerEllipse() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            if (constructionMethod() == ConstructionMethod::Center) {
                centerPoint = onSketchPos;
                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            else {
                periapsis = onSketchPos;
                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (constructionMethod() == ConstructionMethod::PeriapsisApoapsisMinorRadius) {
                apoapsis = onSketchPos;
                centerPoint = (apoapsis - periapsis) / 2 + periapsis;
            }
            else {
                periapsis = onSketchPos;
            }

            firstAxis = periapsis - centerPoint;
            firstRadius = firstAxis.Length();

            try {
                CreateAndDrawShapeGeometry();
            }
            catch(const Base::ValueError &) {} // avoid polluting the error console with drawing exceptions.

            SbString text;
            double angle = GetPointAngle(centerPoint, onSketchPos);
            text.sprintf(" (%.1fR,%.1fdeg)", (float)firstRadius, (float)angle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            //recalculate in case widget modified something
            if (constructionMethod() == ConstructionMethod::PeriapsisApoapsisMinorRadius) {
                centerPoint = (apoapsis - periapsis) / 2 + periapsis;
            }
            firstAxis = periapsis - centerPoint;
            firstRadius = firstAxis.Length();

            //Find bPoint For that first we need the distance of onSketchPos to major axis.
            Base::Vector2d projectedPtn;
            projectedPtn.ProjectToLine(onSketchPos - centerPoint, firstAxis);
            projectedPtn = centerPoint + projectedPtn;
            secondAxis = onSketchPos - projectedPtn;
            secondRadius = secondAxis.Length();

            try {
                CreateAndDrawShapeGeometry();
            }
            catch(const Base::ValueError &) {} // avoid polluting the error console with drawing exceptions.

            SbString text;
            text.sprintf(" (%.1fR,%.1fR)", (float)majorRadius, (float)minorRadius);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstraints[2]);
                return;
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch ellipse"));

            ellipseGeoId = getHighestCurveIndex() + 1;

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", ellipseGeoId);

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add ellipse: %s\n", e.what());
            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError, "Tool execution aborted\n") // This prevents constraints from being applied on non existing geometry
        }
    }

    virtual void generateAutoConstraints() override {

        if (constructionMethod() == ConstructionMethod::Center) {

            auto & ac1 = sugConstraints[0];
            auto & ac2 = sugConstraints[1];

            generateAutoConstraintsOnElement(ac1, ellipseGeoId, Sketcher::PointPos::mid);    // add auto constraints for the center point
            generateAutoConstraintsOnElement(ac2, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the edge
        }
        else {

            auto & ac1 = sugConstraints[0];
            auto & ac2 = sugConstraints[1];
            auto & ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(ac1, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the first point
            generateAutoConstraintsOnElement(ac2, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the second point
            generateAutoConstraintsOnElement(ac3, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the edge
        }
        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry parameters are accurate
        // This is particularly important for adding widget mandated constraints.
        removeRedundantAutoConstraints();
    }

    virtual void createAutoConstraints() override {
        // execute python command to create autoconstraints
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
        sugConstraints[2].clear();
    }

    virtual void createShape(bool onlyeditoutline) override {
        Q_UNUSED(onlyeditoutline);

        ShapeGeometry.clear();

        Base::Vector2d majorAxis = firstAxis;
        majorRadius = firstRadius;

        if( state() == SelectMode::SeekSecond) {
            auto ellipse = std::make_unique<Part::GeomEllipse>();
            ellipse->setMajorRadius(majorRadius);
            ellipse->setMinorRadius(majorRadius*0.5);
            ellipse->setMajorAxisDir(toVector3d(majorAxis));
            ellipse->setCenter(toVector3d(centerPoint));
            ShapeGeometry.push_back(std::move(ellipse));
        }
        else { // SelectMode::SeekThird or SelectMode::End
            minorRadius = secondRadius;

            if (secondRadius > firstRadius) {
                majorAxis = secondAxis;
                majorRadius = secondRadius;
                minorRadius = firstRadius;
            }

            if (fabs(firstRadius - secondRadius) < Precision::Confusion()) {
                auto circle = std::make_unique<Part::GeomCircle>();
                circle->setRadius(firstRadius);
                circle->setCenter(toVector3d(centerPoint));
                ShapeGeometry.push_back(std::move(circle));
            }
            else {
                auto ellipse = std::make_unique<Part::GeomEllipse>();
                ellipse->setMajorRadius(majorRadius);
                ellipse->setMinorRadius(minorRadius);
                ellipse->setMajorAxisDir(toVector3d(majorAxis));
                ellipse->setCenter(toVector3d(centerPoint));
                ShapeGeometry.push_back(std::move(ellipse));
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Ellipse";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center)
            return QString::fromLatin1("Sketcher_Pointer_Create_Ellipse");
        else // constructionMethod == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointEllipse");
    }

private:
    Base::Vector2d centerPoint, periapsis, apoapsis, firstAxis, secondAxis;
    double firstRadius, secondRadius, majorRadius, minorRadius;
    int ellipseGeoId;
};

template <> auto DrawSketchHandlerEllipseBase::ToolWidgetManager::getState(int parameterindex) const {

    if (handler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        switch (parameterindex) {
        case WParameter::First:
        case WParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case WParameter::Third:
            return SelectMode::SeekSecond;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
        }
    }
    else { //if (constructionMethod == ConstructionMethod::ThreeRim)
        switch (parameterindex) {
        case WParameter::First:
        case WParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case WParameter::Third:
        case WParameter::Fourth:
            return SelectMode::SeekSecond;
            break;
        case WParameter::Fifth:
        case WParameter::Sixth:
            return SelectMode::SeekThird;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
        }
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::configureToolWidget() {

    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Center"), QStringLiteral("Periapsis, apoapsis, minor radius")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        syncConstructionMethodComboboxToHandler(); // in case the DSH was called with a specific construction method
    }

    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_ellipse", "x of center"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_ellipse", "y of center"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_ellipse", "First radius"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p3_ellipse", "Angle to HAxis"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p3_ellipse", "Second radius"));
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of periapsis"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of periapsis"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of apoapsis"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of apoapsis"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "Minor radius"));
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->centerPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->centerPoint.y = value;
            break;
        case WParameter::Third:
            //change angle?
            break;
        case WParameter::Fourth:
            dHandler->firstRadius = value;
            break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::ThreeRim)
        switch (parameterindex) {
        case WParameter::First:
            dHandler->periapsis.x = value;
            break;
        case WParameter::Second:
            dHandler->periapsis.y = value;
            break;
        case WParameter::Third:
            dHandler->apoapsis.x = value;
            break;
        case WParameter::Fourth:
            dHandler->apoapsis.y = value;
            break;
        }
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            double length = (onSketchPos - dHandler->centerPoint).Length();
            if (toolWidget->isParameterSet(WParameter::Third)) {
                dHandler->firstRadius = toolWidget->getParameter(WParameter::Third);
                if (length != 0.) {
                    onSketchPos.x = dHandler->centerPoint.x + (onSketchPos.x - dHandler->centerPoint.x) * dHandler->firstRadius / length;
                    onSketchPos.y = dHandler->centerPoint.y + (onSketchPos.y - dHandler->centerPoint.y) * dHandler->firstRadius / length;
                }
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double angle = toolWidget->getParameter(WParameter::Fourth);
                onSketchPos.x = dHandler->centerPoint.x + cos(angle * M_PI / 180) * length;
                onSketchPos.y = dHandler->centerPoint.y + sin(angle * M_PI / 180) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third))
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);

            if (toolWidget->isParameterSet(WParameter::Fourth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                dHandler->secondRadius = toolWidget->getParameter(WParameter::Fifth);
                onSketchPos = dHandler->centerPoint + dHandler->secondAxis * dHandler->secondRadius / dHandler->secondAxis.Length();
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth))
                onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

            if (toolWidget->isParameterSet(WParameter::Sixth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, dHandler->firstRadius);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->firstAxis.Angle());
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->secondRadius);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::SeekThird);

            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {

                handler->updateDataAndDrawToPosition(prevCursorPosition);

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth) ||
                toolWidget->isParameterSet(WParameter::Sixth)) {

                handler->updateDataAndDrawToPosition(prevCursorPosition);

                if (toolWidget->isParameterSet(WParameter::Fifth) &&
                    toolWidget->isParameterSet(WParameter::Sixth)) {

                    handler->setState(SelectMode::End);
                    handler->finish();
                }
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::addConstraints() {
    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        int firstCurve = dHandler->ellipseGeoId;

        auto x0 = toolWidget->getParameter(WParameter::First);
        auto y0 = toolWidget->getParameter(WParameter::Second);
        auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;

        auto x0set = toolWidget->isParameterSet(WParameter::First);
        auto y0set = toolWidget->isParameterSet(WParameter::Second);
        auto firstRadiusSet = toolWidget->isParameterSet(WParameter::Third);
        auto angleSet = toolWidget->isParameterSet(WParameter::Fourth);
        auto secondRadiusSet = toolWidget->isParameterSet(WParameter::Fifth);

        using namespace Sketcher;

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
                x0, handler->sketchgui->getObject());
        }
        else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
        }

        int firstLine = firstCurve + 1;
        int secondLine = firstCurve + 2;
        if (dHandler->secondRadius > dHandler->firstRadius)
            std::swap(firstLine, secondLine);


        //this require to show internal geometry.
        if (firstRadiusSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve, 3, firstLine, 1, dHandler->firstRadius);
        }
        //Todo: this makes the ellipse 'jump' because it's doing a 180 degree turn before applying asked angle. Probably because start and end points of line are not in the correct direction.
        if (angleSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                firstLine, angle);
        }

        if (secondRadiusSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve, 3, secondLine, 1, dHandler->secondRadius);
        }
    }
    //No constraint possible for 3 rim ellipse.
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerEllipse_H

