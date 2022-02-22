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

SketcherToolDefaultWidget::SketcherToolDefaultWidget (QWidget *parent, ViewProviderSketch* sketchView)
  : QWidget(parent), ui(new Ui_TaskSketcherTool), sketchView(sketchView)
{
    ui->setupUi(this);

    // connecting the needed signals
    connect(ui->parameterOne, SIGNAL(valueChanged(double)),
        this, SLOT(parameterOne_valueChanged(double)));
    connect(ui->parameterTwo, SIGNAL(valueChanged(double)),
        this, SLOT(parameterTwo_valueChanged(double)));
    connect(ui->parameterThree, SIGNAL(valueChanged(double)),
        this, SLOT(parameterThree_valueChanged(double)));
    connect(ui->parameterFour, SIGNAL(valueChanged(double)),
        this, SLOT(parameterFour_valueChanged(double)));
    connect(ui->parameterFive, SIGNAL(valueChanged(double)),
        this, SLOT(parameterFive_valueChanged(double)));

    ui->parameterOne->installEventFilter(this);
    ui->parameterTwo->installEventFilter(this);
    ui->parameterThree->installEventFilter(this);
    ui->parameterFour->installEventFilter(this);
    ui->parameterFive->installEventFilter(this);

    reset();
}

SketcherToolDefaultWidget::~SketcherToolDefaultWidget(){}

//pre-select the number of the spinbox when it gets the focus.
bool SketcherToolDefaultWidget::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::FocusIn) {
        for(int i; i < nParameters; i++) {
            auto parameterSpinBox = getParameterSpinBox(i);

            if(object == parameterSpinBox){
                parameterSpinBox->selectNumber();
                break;
            }
        }
    }

    return false;
}

void SketcherToolDefaultWidget::parameterOne_valueChanged(double val)
{
    Q_UNUSED(val);
    isSet[0] = 1;
    setParameterFocus(1);
}
void SketcherToolDefaultWidget::parameterTwo_valueChanged(double val)
{
    Q_UNUSED(val);
    isSet[1] = 1;
    setParameterFocus(2);
}
void SketcherToolDefaultWidget::parameterThree_valueChanged(double val)
{
    Q_UNUSED(val);
    isSet[2] = 1;
    setParameterFocus(3);
}
void SketcherToolDefaultWidget::parameterFour_valueChanged(double val)
{
    Q_UNUSED(val);
    isSet[3] = 1;
    setParameterFocus(4);
}

void SketcherToolDefaultWidget::parameterFive_valueChanged(double val)
{
    Q_UNUSED(val);
    isSet[4] = 1;
}

void SketcherToolDefaultWidget::setParameterVisible(int parameterindex, bool visible)
{
    if(parameterindex < nParameters) {
        getParameterLabel(parameterindex)->setVisible(visible);
        getParameterSpinBox(parameterindex)->setVisible(visible);
    }
}

void SketcherToolDefaultWidget::setParameterLabel(int parameterindex, const QString & string)
{
    if(parameterindex < nParameters)
        getParameterLabel(parameterindex)->setText(string);
}

QLabel * SketcherToolDefaultWidget::getParameterLabel(int parameterindex)
{
    switch(parameterindex) {
        case 0:
            return ui->label;
            break;
        case 1:
            return ui->label2;
            break;
        case 2:
            return ui->label3;
            break;
        case 3:
            return ui->label4;
            break;
        case 4:
            return ui->label5;
            break;
        default:
            return nullptr;
    }
}

Gui::PrefQuantitySpinBox * SketcherToolDefaultWidget::getParameterSpinBox(int parameterindex)
{
    switch(parameterindex) {
        case 0:
            return ui->parameterOne;
            break;
        case 1:
            return ui->parameterTwo;
            break;
        case 2:
            return ui->parameterThree;
            break;
        case 3:
            return ui->parameterFour;
            break;
        case 4:
            return ui->parameterFive;
            break;
        default:
            return nullptr;
    }
}

void SketcherToolDefaultWidget::reset()
{
    for(int i=0; i<nParameters; i++) {
        setParameterVisible(i, false);
        setParameter(i, 0.f);
    }
}

void SketcherToolDefaultWidget::initNParameters(int nparameters)
{
    for(int i=0; i<nParameters; i++) {
        setParameterVisible(i, (i<nparameters)?true:false);
        setParameter(i, 0.f);

    }
}

/*
void SketcherToolDefaultWidget::setSettings(int toolSelected)
{
    //Set names and hide excess parameters
    //For reference unit can be changed with :
    //setUnit(Base::Unit::Length); Base::Unit::Angle
    switch (toolSelected) {
    case 0: //none, reset and hide all settings


        ui->label->setVisible(0);
        ui->label2->setVisible(0);
        ui->label3->setVisible(0);
        ui->label4->setVisible(0);
        ui->label5->setVisible(0);
        setparameter(0, 0);
        setparameter(0, 1);
        setparameter(0, 2);
        setparameter(0, 3);
        setparameter(0, 4);

        ui->parameterOne->setVisible(0);
        ui->parameterTwo->setVisible(0);
        ui->parameterThree->setVisible(0);
        ui->parameterFour->setVisible(0);
        ui->parameterFive->setVisible(0);
        break;
    case 1: //rectangle : DrawSketchHandlerBox
        toolParameters.resize(4, 0);
        isSettingSet.resize(4, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (along x axis)"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (along y axis)"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 2: //Round corner rectangle : DrawSketchHandlerOblong
        toolParameters.resize(5, 0);
        isSettingSet.resize(5, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label5->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (along x axis)"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (along y axis)"));
        ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Oblong", "Corner radius"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);
        ui->parameterFive->setVisible(1);
        ui->parameterFive->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 3: //Circle : DrawSketchHandlerCircle & arc
        toolParameters.resize(3, 0);
        isSettingSet.resize(3, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_circle", "Radius"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 4: //Point : DrawSketchHandlerPoint
        toolParameters.resize(2, 0);
        isSettingSet.resize(2, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_point", "x of point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_point", "y of point"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 5: //Line : DrawSketchHandlerLine & arcby3points & circle by 3 points
        toolParameters.resize(4, 0);
        isSettingSet.resize(4, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "x of 2nd point"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "y of 2nd point"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 6: //PolyLine (from second line)
        toolParameters.resize(4, 0);
        isSettingSet.resize(4, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_polyline", "x of n-1 point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_polyline", "y of n-1 point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_polyline", "x of n point"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_polyline", "y of n point"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(0);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(0);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(1);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(1);

        QMetaObject::invokeMethod(ui->parameterThree, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 7: //Ellipse : DrawSketchHandlerEllipse
        toolParameters.resize(5, 0);
        isSettingSet.resize(5, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label5->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_Ellipse", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_Ellipse", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_Ellipse", "x of 2nd point"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_Ellipse", "y of 2nd point"));
        ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Ellipse", "Second radius"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);
        ui->parameterFive->setVisible(1);
        ui->parameterFive->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    }
}
*/

double SketcherToolDefaultWidget::getParameter(int parameterindex)
{
    if (parameterindex < nParameters) {
        return getParameterSpinBox(parameterindex)->value().getValue();
    }
    return 0;
}

bool SketcherToolDefaultWidget::isParameterSet(int parameterindex)
{
    if (parameterindex < nParameters) {
        return isSet[parameterindex];
    }
    return 0;
}

void SketcherToolDefaultWidget::setParameter(int parameterindex, double val)
{
    if(parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setValue(Base::Quantity(val, Base::Unit::Length));
    }
}

void SketcherToolDefaultWidget::setParameterEnabled(int parameterindex, bool active)
{
    if(parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setEnabled(active);
    }
}

void SketcherToolDefaultWidget::setParameterFocus(int parameterindex)
{
    if(parameterindex < nParameters) {
        auto parameterSpinBox = getParameterSpinBox(parameterindex);
        parameterSpinBox->selectNumber();
        QMetaObject::invokeMethod(parameterSpinBox, "setFocus", Qt::QueuedConnection);
    }
}

void SketcherToolDefaultWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


#include "moc_SketcherToolDefaultWidget.cpp"
