
/*LICENSE_START*/
/*
 *  Copyright (C) 2016 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#define __ANNOTATION_DEPTH_WIDGET_DECLARE__
#include "AnnotationDepthWidget.h"
#undef __ANNOTATION_DEPTH_WIDGET_DECLARE__

#include <QAction>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>

#include "AnnotationPolyhedron.h"
#include "AnnotationManager.h"
#include "AnnotationRedoUndoCommand.h"
#include "Brain.h"
#include "BrainBrowserWindow.h"
#include "CaretAssert.h"
#include "BrowserTabContent.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "GuiManager.h"
#include "NumericTextFormatting.h"
#include "Overlay.h"
#include "OverlaySet.h"
#include "VolumeMappableInterface.h"
#include "WuQMessageBox.h"
#include "WuQtUtilities.h"

using namespace caret;
    
/**
 * \class caret::AnnotationDepthWidget
 * \brief Widget for deleting annotations.
 * \ingroup GuiQt
 */

/**
 * Constructor.
 * @param userInputMode
 *    The input mode
 * @param browserWindowIndex
 *    Index of browser window
 * @param parent
 *    The parent widget
 */
AnnotationDepthWidget::AnnotationDepthWidget(const UserInputModeEnum::Enum userInputMode,
                                             const int32_t browserWindowIndex,
                                             QWidget* parent)
: QWidget(parent),
m_userInputMode(userInputMode),
m_browserWindowIndex(browserWindowIndex)
{
    QLabel* depthLabel = new QLabel("Depth");
    QLabel* mmLabel = new QLabel("(mm)");
    m_depthSpinBox = new QDoubleSpinBox();
    m_depthSpinBox->setMinimum(-500.0);
    m_depthSpinBox->setMaximum(500.0);
    m_depthSpinBox->setDecimals(2);
    m_depthSpinBox->setValue(3.0);
    QObject::connect(m_depthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     this, &AnnotationDepthWidget::depthValueChanged);

    m_sliceThicknessLabel = new QLabel("Slice=X.XXmm");
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    WuQtUtilities::setLayoutSpacingAndMargins(layout, 2, 2);
    layout->addWidget(depthLabel, 0, Qt::AlignHCenter);
    layout->addWidget(mmLabel, 0, Qt::AlignHCenter);
    layout->addWidget(m_depthSpinBox, 0, Qt::AlignHCenter);
    layout->addWidget(m_sliceThicknessLabel, 0, Qt::AlignLeft);
    layout->addStretch();
    
    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);
}

/**
 * Destructor.
 */
AnnotationDepthWidget::~AnnotationDepthWidget()
{
}

/**
 * Update the content.
 */
void
AnnotationDepthWidget::updateContent(std::vector<Annotation*>& annotationsIn)
{
    /*
     * This depth DOES NOT affect the annotation being drawn
     */
    AnnotationPolyhedron* firstPolyhedron(NULL);
    m_annotations.clear();
    for (auto& ann : annotationsIn) {
        CaretAssert(ann);
        AnnotationPolyhedron* ap(ann->castToPolyhedron());
        if (ap != NULL) {
            if (firstPolyhedron == NULL) {
                firstPolyhedron = ap;
            }
            m_annotations.push_back(ap);
        }
    }

    float sliceThickness(1.0);
    const BrainBrowserWindow* bbw(GuiManager::get()->getBrowserWindowByWindowIndex(m_browserWindowIndex));
    if (bbw != NULL) {
        const BrowserTabContent* tabContent(bbw->getBrowserTabContent());
        if (tabContent != NULL) {
            const OverlaySet* overlaySet(tabContent->getOverlaySet());
            if (overlaySet != NULL) {
                const VolumeMappableInterface* volumeInterface(tabContent->getOverlaySet()->getUnderlayVolume());
                if (volumeInterface != NULL) {
                    sliceThickness = volumeInterface->getMaximumVoxelSpacing();
                }
            }
        }
    }
    
    float numberOfSlices(1.0);
    float depthValue(3.0);
    if (firstPolyhedron != NULL) {
        depthValue = firstPolyhedron->getDepthMillimeters();
        if (sliceThickness != 0.0) {
            numberOfSlices = firstPolyhedron->getDepthSlices(sliceThickness);
        }
    }
    
    AString sliceCountText(NumericTextFormatting::formatValue(numberOfSlices));
    if (sliceCountText.endsWith(".0")) {
        sliceCountText.resize(sliceCountText.length() - 2);
    }
    m_sliceThicknessLabel->setText(sliceCountText
                                   + " Slices");
    
    m_depthSpinBox->setSingleStep(sliceThickness);
    
    QSignalBlocker blocker(m_depthSpinBox);
    m_depthSpinBox->setValue(depthValue);
    
    setEnabled( ! m_annotations.empty());
}

/**
 * Called when new sample depth value is changed
 */
void
AnnotationDepthWidget::depthValueChanged(double value)
{
    QSignalBlocker blocker(m_depthSpinBox);
    
    if ( ! m_annotations.empty()) {
        AnnotationRedoUndoCommand* undoCommand = new AnnotationRedoUndoCommand();
        undoCommand->setModePolyhedronDepth(value,
                                            m_annotations);
        
        AString errorMessage;
        AnnotationManager* annotationManager = GuiManager::get()->getBrain()->getAnnotationManager(m_userInputMode);
        if ( ! annotationManager->applyCommand(undoCommand,
                                               errorMessage)) {
            WuQMessageBox::errorOk(this,
                                   errorMessage);
        }
        EventManager::get()->sendSimpleEvent(EventTypeEnum::EVENT_ANNOTATION_TOOLBAR_UPDATE);
        EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
    }
}
