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

#include "ui_SketcherToolDefaultWidget.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Exception.h>

#include <QEvent>

#include "ViewProviderSketch.h"

#include "SketcherToolDefaultWidget.h"

using namespace SketcherGui;
using namespace Gui::TaskView;
namespace bp = boost::placeholders;

SketcherToolDefaultWidget::SketcherToolDefaultWidget (QWidget *parent, ViewProviderSketch* sketchView)
  : QWidget(parent), ui(new Ui_SketcherToolDefaultWidget), sketchView(sketchView), blockParameterSlots(false)
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
    if(!blockParameterSlots) {
        isSet[Parameter::First] = true;
        setParameterFocus(Parameter::Second);
        signalParameterValueChanged(Parameter::First, val);
    }
}
void SketcherToolDefaultWidget::parameterTwo_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Second] = true;
        setParameterFocus(Parameter::Third);
        signalParameterValueChanged(Parameter::Second, val);
    }
}
void SketcherToolDefaultWidget::parameterThree_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Third] = true;
        setParameterFocus(Parameter::Fourth);
        signalParameterValueChanged(Parameter::Third, val);
    }
}
void SketcherToolDefaultWidget::parameterFour_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Fourth] = true;
        setParameterFocus(Parameter::Fifth);
        signalParameterValueChanged(Parameter::Fourth, val);
    }
}

void SketcherToolDefaultWidget::parameterFive_valueChanged(double val)
{
    if(!blockParameterSlots) {
        isSet[Parameter::Fifth] = true;
        signalParameterValueChanged(Parameter::Fifth, val);
    }
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
        case Parameter::First:
            return ui->label;
            break;
        case Parameter::Second:
            return ui->label2;
            break;
        case Parameter::Third:
            return ui->label3;
            break;
        case Parameter::Fourth:
            return ui->label4;
            break;
        case Parameter::Fifth:
            return ui->label5;
            break;
        default:
            return nullptr;
    }
}

Gui::PrefQuantitySpinBox * SketcherToolDefaultWidget::getParameterSpinBox(int parameterindex)
{
    switch(parameterindex) {
        case Parameter::First:
            return ui->parameterOne;
            break;
        case Parameter::Second:
            return ui->parameterTwo;
            break;
        case Parameter::Third:
            return ui->parameterThree;
            break;
        case Parameter::Fourth:
            return ui->parameterFour;
            break;
        case Parameter::Fifth:
            return ui->parameterFive;
            break;
        default:
            return nullptr;
    }
}

void SketcherToolDefaultWidget::reset()
{
    Base::StateLocker lock(blockParameterSlots, true);

    std::fill(isSet.begin(), isSet.end(), false);

    for(int i=0; i<nParameters; i++) {
        setParameterVisible(i, false);
        setParameter(i, 0.f);
    }
}

void SketcherToolDefaultWidget::initNParameters(int nparameters)
{
    Base::StateLocker lock(blockParameterSlots, true);

    isSet.resize(nparameters);

    std::fill(isSet.begin(), isSet.end(), false);

    for(int i=0; i<nParameters; i++) {
        setParameterVisible(i, (i<nparameters)?true:false);
        setParameter(i, 0.f);
    }
}

double SketcherToolDefaultWidget::getParameter(int parameterindex)
{
    if (parameterindex < nParameters) {
        return getParameterSpinBox(parameterindex)->value().getValue();
    }

    THROWM(Base::IndexError, "ToolWidget parameter index out of range");
}

bool SketcherToolDefaultWidget::isParameterSet(int parameterindex)
{
    if (parameterindex < nParameters) {
        return isSet[parameterindex];
    }

    THROWM(Base::IndexError, "ToolWidget parameter index out of range");
}

void SketcherToolDefaultWidget::setParameter(int parameterindex, double val)
{
    if(parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setValue(Base::Quantity(val, Base::Unit::Length));

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions","ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterEnabled(int parameterindex, bool active)
{
    if(parameterindex < nParameters) {
        getParameterSpinBox(parameterindex)->setEnabled(active);

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions","ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::setParameterFocus(int parameterindex)
{
    if(parameterindex < nParameters) {
        auto parameterSpinBox = getParameterSpinBox(parameterindex);
        parameterSpinBox->selectNumber();
        QMetaObject::invokeMethod(parameterSpinBox, "setFocus", Qt::QueuedConnection);

        return;
    }

    THROWM(Base::IndexError, QT_TRANSLATE_NOOP("Exceptions","ToolWidget parameter index out of range"));
}

void SketcherToolDefaultWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


#include "moc_SketcherToolDefaultWidget.cpp"
