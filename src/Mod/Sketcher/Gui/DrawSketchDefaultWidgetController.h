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

#ifndef SKETCHERGUI_DrawSketchDefaultWidgetController_H
#define SKETCHERGUI_DrawSketchDefaultWidgetController_H

#include <Base/Tools.h>
#include <Gui/EditableDatumLabel.h>

#include "DrawSketchController.h"

namespace SketcherGui
{

/** @brief Type encapsulating the number of parameters in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetParameters: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of checkboxes in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetCheckboxes: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of comboboxes in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetComboboxes: public ControlAmount<sizes...>
{
};

namespace sp = std::placeholders;

/** @brief Class defining a generic handler controller operable with a DrawSketchControllableHandler
 *
 * @details
 * This class is intended as a parent for controller classes, which control a handler based on
 * additional input provided by the user (beyond the typical mouse-click driven behaviour).
 */
template<typename HandlerT,
         typename SelectModeT,
         int PAutoConstraintSize,     // The initial size of the AutoConstraint vector
         typename OnViewParametersT,  // The number of parameter spinboxes in the 3D view
         typename WidgetParametersT,  // The number of parameter spinboxes in the default widget
         typename WidgetCheckboxesT,  // The number of checkboxes in the default widget
         typename WidgetComboboxesT,  // The number of comboboxes in the default widget
         typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod,
         bool PFirstComboboxIsConstructionMethod =
             false>  // The handler template or class having this as inner class
class DrawSketchDefaultWidgetController: public DrawSketchController<HandlerT,
                                                                     SelectModeT,
                                                                     PAutoConstraintSize,
                                                                     OnViewParametersT,
                                                                     ConstructionMethodT>
{
public:
    using ControllerBase = DrawSketchController<HandlerT,
                                                SelectModeT,
                                                PAutoConstraintSize,
                                                OnViewParametersT,
                                                ConstructionMethodT>;

protected:
    int nParameter = WidgetParametersT::defaultMethodSize();
    int nCheckbox = WidgetCheckboxesT::defaultMethodSize();
    int nCombobox = WidgetComboboxesT::defaultMethodSize();

    SketcherToolDefaultWidget* toolWidget;

    using Connection = boost::signals2::connection;

    Connection connectionParameterValueChanged;
    Connection connectionCheckboxCheckedChanged;
    Connection connectionComboboxSelectionChanged;

    /** @name Named indices for controls of the default widget (SketcherToolDefaultWidget) */
    //@{
    using WParameter = SketcherToolDefaultWidget::Parameter;
    using WCheckbox = SketcherToolDefaultWidget::Checkbox;
    using WCombobox = SketcherToolDefaultWidget::Combobox;
    //@}

    using SelectMode = SelectModeT;
    using ControllerBase::handler;

public:
    DrawSketchDefaultWidgetController(HandlerT* dshandler)
        : ControllerBase(dshandler)
    {}

    ~DrawSketchDefaultWidgetController()
    {
        connectionParameterValueChanged.disconnect();
        connectionCheckboxCheckedChanged.disconnect();
        connectionComboboxSelectionChanged.disconnect();
    }

    /** @name functions NOT intended for specialisation */
    //@{
    /** boost slot triggering when a parameter has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void parameterValueChanged(int parameterindex, double value)
    {
        adaptDrawingToParameterChange(parameterindex, value);

        ControllerBase::finishControlsChanged();
    }

    /** boost slot triggering when a checkbox has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void checkboxCheckedChanged(int checkboxindex, bool value)
    {
        adaptDrawingToCheckboxChange(checkboxindex, value);

        ControllerBase::finishControlsChanged();
    }

    /** boost slot triggering when a combobox has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void comboboxSelectionChanged(int comboboxindex, int value)
    {
        adaptDrawingToComboboxChange(comboboxindex, value);

        ControllerBase::finishControlsChanged();
    }

    //@}

    /** @name functions which MUST be specialised */
    //@{
    /// Change DSH to reflect a value entered in the view
    void adaptDrawingToOnViewParameterChange(int onviewparameterindex, double value)
    {
        Q_UNUSED(onviewparameterindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a value entered in the widget
    void adaptDrawingToParameterChange(int parameterindex, double value)
    {
        Q_UNUSED(parameterindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a checkbox changed in the widget
    void adaptDrawingToCheckboxChange(int checkboxindex, bool value)
    {
        Q_UNUSED(checkboxindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a comboBox changed in the widget
    void adaptDrawingToComboboxChange(int comboboxindex, int value)
    {
        Q_UNUSED(comboboxindex);

        if constexpr (PFirstComboboxIsConstructionMethod == true) {

            if (comboboxindex == WCombobox::FirstCombo && handler->ConstructionMethodsCount() > 1) {
                handler->setConstructionMethod(static_cast<ConstructionMethodT>(value));
            }
        }
    }

    /** Returns the state to which the widget parameter corresponds in the current construction
     * method
     */
    auto getState(int parameterindex) const
    {
        ControllerBase::getState();
    }

    /// function to create constraints based on widget information.
    void addConstraints()
    {}

    /** @name functions which need to be specialised */
    //@{
    void configureToolWidget()
    {}

    /** function that is called by the handler with a Vector2d position to update the widget
     *
     * It MUST be specialised if you want the parameters to update on mouseMove
     */
    void doChangeDrawSketchHandlerMode()
    {}

    void adaptParameters(Base::Vector2d onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    void doEnforceControlParameters(Base::Vector2d& onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }
    //@}


    /** on first shortcut, it toggles the first checkbox if there is go. Must be specialised if
     * this is not intended */
    virtual void firstKeyShortcut() override
    {
        if (nCheckbox >= 1) {
            auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
            toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
        }
    }

    void secondKeyShortcut() override
    {
        if (nCheckbox >= 2) {
            auto secondchecked = toolWidget->getCheckboxChecked(WCheckbox::SecondBox);
            toolWidget->setCheckboxChecked(WCheckbox::SecondBox, !secondchecked);
        }
    }

protected:
    /** @name DrawSketchController NVI */
    //@{
    virtual void doInitControls(QWidget* widget) override
    {
        initDefaultWidget(widget);
        ControllerBase::doInitControls(widget);
    }

    virtual void doResetControls() override
    {
        ControllerBase::doResetControls();
        resetDefaultWidget();
    }

    virtual void doConstructionMethodChanged() override
    {
        nParameter = WidgetParametersT::size(handler->constructionMethod());
        nCheckbox = WidgetCheckboxesT::size(handler->constructionMethod());
        nCombobox = WidgetComboboxesT::size(handler->constructionMethod());

        // update the combobox only if necessary (if the change was not triggered by the
        // combobox)
        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto currentindex = toolWidget->getComboboxIndex(WCombobox::FirstCombo);
            auto methodint = static_cast<int>(handler->constructionMethod());

            if (currentindex != methodint) {
                // avoid triggering of method change
                boost::signals2::shared_connection_block combobox_block(
                    connectionComboboxSelectionChanged);
                toolWidget->setComboboxIndex(WCombobox::FirstCombo, methodint);
            }
        }
    }
    //@}

private:
    void initDefaultWidget(QWidget* widget)
    {
        toolWidget = static_cast<SketcherToolDefaultWidget*>(widget);

        connectionParameterValueChanged = toolWidget->registerParameterValueChanged(
            std::bind(&DrawSketchDefaultWidgetController::parameterValueChanged,
                      this,
                      sp::_1,
                      sp::_2));

        connectionCheckboxCheckedChanged = toolWidget->registerCheckboxCheckedChanged(
            std::bind(&DrawSketchDefaultWidgetController::checkboxCheckedChanged,
                      this,
                      sp::_1,
                      sp::_2));

        connectionComboboxSelectionChanged = toolWidget->registerComboboxSelectionChanged(
            std::bind(&DrawSketchDefaultWidgetController::comboboxSelectionChanged,
                      this,
                      sp::_1,
                      sp::_2));
    }

    void resetDefaultWidget()
    {
        boost::signals2::shared_connection_block parameter_block(connectionParameterValueChanged);
        boost::signals2::shared_connection_block checkbox_block(connectionCheckboxCheckedChanged);
        boost::signals2::shared_connection_block combobox_block(connectionComboboxSelectionChanged);

        toolWidget->initNParameters(nParameter);
        toolWidget->initNCheckboxes(nCheckbox);
        toolWidget->initNComboboxes(nCombobox);

        configureToolWidget();
    }

private:
    /** @name helper functions */
    //@{
    /// returns the status to which the handler was updated
    bool syncHandlerToCheckbox(int checkboxindex, bool& handlerboolean)
    {
        bool status = toolWidget->getCheckboxChecked(checkboxindex);
        handlerboolean = status;

        return status;
    }

    /// returns true if checkbox was changed, and false if no sync was necessary
    bool syncCheckboxToHandler(int checkboxindex, bool handlerboolean)
    {
        bool status = toolWidget->getCheckboxChecked(checkboxindex);
        if (handlerboolean != status) {
            toolWidget->setCheckboxChecked(checkboxindex, handlerboolean);
            return true;
        }

        return false;
    }

    void syncHandlerToConstructionMethodCombobox()
    {

        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

            handler->initConstructionMethod(static_cast<ConstructionMethodT>(constructionmethod));
        }
    }
    void syncConstructionMethodComboboxToHandler()
    {

        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

            auto actualconstructionmethod = static_cast<int>(handler->constructionMethod());

            if (constructionmethod != actualconstructionmethod) {
                toolWidget->setComboboxIndex(WCombobox::FirstCombo, actualconstructionmethod);
            }
        }
    }
    //@}
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchDefaultWidgetController_H
