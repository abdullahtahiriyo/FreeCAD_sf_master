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

#if 0

#ifndef SKETCHERGUI_DrawSketchOnViewController_H
#define SKETCHERGUI_DrawSketchOnViewController_H

#include <Base/Tools.h>
#include <Gui/EditableDatumLabel.h>

#include "DrawSketchController.h"

namespace SketcherGui
{

/** @brief Type encapsulating the number of On view parameters*/
template<int... sizes>  // Initial sizes for each mode
class OnViewParameters: public ControlAmount<sizes...>
{
};

/** @brief Class defining handler controller having on-view parameters
 *
 * @details
 * This class is intended as a parent for controller classes making use of on-view parameters. This
 * can handlers without a widget control, with the default widget or with a custom widget.
 */
template<typename HandlerT,           // The final handler class
         typename OnViewParametersT,  // The number of parameter spinboxes in the 3D view
         typename SelectModeT>        // The state machine defining the states of the handler>
class DrawSketchOnViewController: public DrawSketchController<HandlerT, SelectModeT>
{
protected:
    int nOnViewParameter = OnViewParametersT::defaultMethodSize();

    int onViewIndexWithFocus = 0;  // track the index of the label having the focus

    /** @name Named indices for controls of the on-view controls (SketcherToolDefaultWidget) */
    //@{
    enum OnViewParameter
    {
        First,
        Second,
        Third,
        Fourth,
        Fifth,
        Sixth,
        Seventh,
        Eighth,
        Ninth,
        Tenth,
        nOnViewParameters  // Must Always be the last one
    };
    //@}

    std::vector<std::unique_ptr<Gui::EditableDatumLabel>> onViewParameters;

    using DSController = DrawSketchController<HandlerT, SelectModeT>;

public:
    DrawSketchOnViewController(HandlerT* dshandler)
        : DSController(dshandler)
    {}

    ~DrawSketchOnViewController()
    {}

    /** @name functions NOT intended for specialisation */
    //@{
    /** slot triggering when a on view parameter has changed
     * It is intended to remote control the DrawSketchOnViewController
     */
    void OnViewValueChanged(int onviewparameterindex, double value)
    {
        if (isOnViewParameterOfCurrentMode(onviewparameterindex + 1)) {
            setFocusToOnViewParameter(onviewparameterindex + 1);
        }

        /* That is not supported with labels.
        // -> A machine does not forward to a next state when adapting the parameter (though it
        // may forward to a next state if all the parameters are fulfilled, see
        // doChangeDrawSketchHandlerMode). This ensures that the geometry has been defined
        // (either by mouse clicking or by widget). Autoconstraints on point should be picked
        // when the state is reached upon machine state advancement.
        //
        // -> A machine goes back to a previous state if a parameter of a previous state is
        // modified. This ensures that appropriate autoconstraints are picked.*/

        adaptDrawingToOnViewParameterChange(onviewparameterindex, value);

        DSController::finishControlChanged();
    }
    //@}

    /** @name functions which MUST be specialised */
    //@{

    /// Function to specialise to set the correct widget strings and commands
    void configureOnViewParameters()
    {}


    /// Change DSH to reflect a value entered in the view
    void adaptDrawingToOnViewParameterChange(int onviewparameterindex, double value)
    {
        Q_UNUSED(onviewparameterindex);
        Q_UNUSED(value);
    }

    /** @brief Returns the state machine state to which the On-View parameter corresponds in the
     * current construction method*/
    auto getStateOfOnViewParameter(int onviewparameterindex) const
    {
        Q_UNUSED(onviewparameterindex);
        return DSController::handler->getFirstState();
    }
    //@}


    /** function that is called by the handler when the selection mode changed
     *
     * It can be specialised if needed
     */
    void onHandlerModeChanged() override
    {
        setModeLabels();
    }


    /* Activates the correct set of on-view parameters corresponding to current
     * mode. It may be specialized if necessary.*/
    void setModeLabels()
    {
        bool firstOfMode = true;
        for (size_t i = 0; i < onViewParameters.size(); i++) {
            if (!isOnViewParameterOfCurrentMode(i)) {
                onViewParameters[i]->stopEdit();
                if (!onViewParameters[i]->isSet
                    || DSController::handler->state() == SelectModeT::End) {
                    onViewParameters[i]->deactivate();
                }
            }
            else {
                if (firstOfMode) {
                    onViewIndexWithFocus = i;
                    firstOfMode = false;
                }

                onViewParameters[i]->activate();

                // points/value will be overridden by the mouseMove triggered by the mode change.
                onViewParameters[i]->setPoints(Base::Vector3d(), Base::Vector3d());
                onViewParameters[i]->startEdit(0.0, nullptr);  // TODO: Review the nullptr part
            }
        }
    }

    /** function that is called by the handler when the construction mode changed
     *
     * This is just a default implementation for common stateMachines, that may
     * or may not do what you expect. It assumes two parameters per seek state.
     *
     * It MUST be specialised otherwise
     */
    void onConstructionMethodChanged() final
    {

        nOnViewParameter = OnViewParametersT::size(handler->constructionMethod());

        onConstructionMethodChanged();


        handler->updateCursor();

        handler->reset();  // reset of handler to restart.
    }

    /** function that is called by the handler with a Vector2d position to update the widget
     *
     * It MUST be specialised if you want the parameters to update on mouseMove
     */
    void adaptParameters(Base::Vector2d onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    /** function that is called by the handler with a mouse position, enabling the
     * widget to override it having regard to the widget information.
     *
     * It MUST be specialised if you want to override mouse position based on parameters.
     */
    void doEnforceWidgetParameters(Base::Vector2d& onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    /** on first shortcut, it toggles the first checkbox if there is go. Must be specialised if
     * this is not intended */
    void firstKeyShortcut()
    {
        if (nCheckbox >= 1) {
            auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
            toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
        }
    }

    void secondKeyShortcut()
    {
        if (nCheckbox >= 2) {
            auto secondchecked = toolWidget->getCheckboxChecked(WCheckbox::SecondBox);
            toolWidget->setCheckboxChecked(WCheckbox::SecondBox, !secondchecked);
        }
    }


    //@}

    void passFocusToNextLabel()
    {
        unsigned int index = labelIndexWithFocus + 1;

        if (index >= onViewParameters.size()) {
            index = 0;
        }
        while (index < onViewParameters.size()) {
            if (isLabelOfCurrentMode(index)) {
                setFocusToLabel(index);
                break;
            }
            index++;
        }
    }

private:
    /** @name helper functions */
    //@{

    /// function to assist in adaptDrawingToComboboxChange specialisation
    /// assigns the modevalue to the modeenum
    /// it also triggers an update of the cursor
    template<typename T>
    void setMode(T& modeenum, int modevalue)
    {
        auto mode = static_cast<T>(modevalue);

        modeenum = mode;

        handler->updateCursor();

        handler->reset();  // reset of handler to restart.
    }

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

private:
    /** @name Helper functions */
    //@{
    /** @brief Initialises on-screen parameters */
    void initNOnViewParameters(int n)
    {
        Gui::View3DInventorViewer* viewer = DSController::handler->getViewer();
        Base::Placement placement =
            DSController::handler->sketchgui->getSketchObject()->Placement.getValue();

        onViewParameters.clear();

        for (int i = 0; i < n; i++) {

            // the returned is a naked pointer
            auto parameter = onViewParameters
                                 .emplace_back(std::make_unique<Gui::EditableDatumLabel>(
                                     viewer,
                                     placement,
                                     SbColor(0.8f, 0.8f, 0.8f),
                                     /*autoDistance = */ true))
                                 .get();

            QObject::connect(parameter, &Gui::EditableDatumLabel::valueChanged, [=](double value) {
                parameter->setColor(SbColor(1.0f, 0.149f, 0.0f));
                OnViewValueChanged(i, value);
            });
        }
    }

    bool isOnViewParameterOfCurrentMode(unsigned int onviewparameterindex) const
    {
        return onviewparameterindex < onViewParameters.size()
            && getStateOfOnViewParameter(onviewparameterindex) == DSController::handler->state();
    }

    bool isOnViewParameterOfPreviousMode(unsigned int onviewparameterindex) const
    {
        return onviewparameterindex < onViewParameters.size()
            && getStateOfOnViewParameter(onviewparameterindex) < DSController::handler->state();
    }

    /* This function give the focus to a spinbox and register to which it gave it.*/
    void setFocusToOnViewParameter(unsigned int onviewparameterindex)
    {
        if (onviewparameterindex < onViewParameters.size()) {
            onViewParameters[onviewparameterindex]->setFocusToSpinbox();
            onViewIndexWithFocus = onviewparameterindex;
        }
    }
    //@}


    /** @name NVI of DrawSketchController */
    //@{

    /** @brief Not to be executed directly, but via parent initControls() */

    virtual void doInitControls(QWidget* widget) final
    {
        doInitControlsOnViewEnrichedController(widget);

        // NOTE: Here it may come any init code necessary for on-view parameters (as discussed)
    }

    /** @brief Not to be executed directly, but via parent resetController() */
    virtual void doResetController() final
    {
        initNOnViewParameters(nOnViewParameter);

        configureOnViewParameters();
        onViewIndexWithFocus = 0;

        doResetOnViewEnrichedController();  // NVI
    }

    /** @brief function triggered by the handler when the mouse has been moved */
    void onMouseMoved(Base::Vector2d originalSketchPosition) final
    {
        Q_UNUSED(originalSketchPosition)

        if (!DSController::firstMoveInit) {
            setModeLabels();
        }

        // NOTE: Can be extended via NVI if necessary for other derived controller.
    }

    void afterEnforceWidgetParameters() override
    {
        // Give focus to current label. In case user interacted outside of 3dview.
        setFocusToOnViewParameter(onViewIndexWithFocus);
    }
    //@}

    /** @name NVI */
    //@{
    virtual void doResetOnViewEnrichedController()
    {}

    virtual void doInitControlsOnViewEnrichedController(QWidget* widget)
    {
        Q_UNUSED(widget)
    }

    virtual void onConstructionMethodChanged()
    {}
    //@}
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchOnViewController_H

#endif
