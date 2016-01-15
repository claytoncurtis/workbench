
/*LICENSE_START*/
/*
 *  Copyright (C) 2015 Washington University School of Medicine
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

#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>

#define __ANNOTATION_ARRANGER_EXECUTOR_DECLARE__
#include "AnnotationArrangerExecutor.h"
#undef __ANNOTATION_ARRANGER_EXECUTOR_DECLARE__

#include "AnnotationArrangerInputs.h"
#include "AnnotationCoordinate.h"
#include "AnnotationManager.h"
#include "AnnotationOneDimensionalShape.h"
#include "AnnotationRedoUndoCommand.h"
#include "AnnotationText.h"
#include "BrainOpenGLTextRenderInterface.h"
#include "CaretAssert.h"
#include "CaretException.h"
#include "EventGetViewportSize.h"
#include "EventManager.h"

using namespace caret;



/**
 * \class caret::AnnotationArrangerExecutor
 * \brief Performs alignment, distribution of annotations similar to PowerPoint.
 * \ingroup Brain
 */

/**
 * Constructor.
 */
AnnotationArrangerExecutor::AnnotationArrangerExecutor()
: CaretObject(),
m_mode(MODE_NONE),
m_annotationManager(NULL),
m_alignment(AnnotationAlignmentEnum::ALIGN_BOTTOM),
m_distribute(AnnotationDistributeEnum::HORIZONTALLY),
m_debugFlag(true)
{
    
}

/**
 * Destructor.
 */
AnnotationArrangerExecutor::~AnnotationArrangerExecutor()
{
}

/**
 * Apply alignment modification to selected annotations
 *
 * @param annotationManager
 *     The annotation manager.
 * @param arrangerInputs
 *     Contains information about the alignment.
 * @param alignment
 *     The alignment selection.
 * @param errorMessageOut
 *     Contains description of error.
 * @return
 *     True if the alignment was successful, else false.
 */
bool
AnnotationArrangerExecutor::alignAnnotations(AnnotationManager* annotationManager,
                                             const AnnotationArrangerInputs& arrangerInputs,
                                             const AnnotationAlignmentEnum::Enum alignment,
                                             AString& errorMessageOut)
{
    m_mode = MODE_ALIGN;
    
    m_annotationManager = annotationManager;
    CaretAssert(m_annotationManager);
    
    m_alignment = alignment;
    
    errorMessageOut.clear();
    
    try {
        alignAnnotationsPrivate(arrangerInputs);
    }
    catch (const CaretException& caretException) {
        errorMessageOut = caretException.whatString();
        return false;
    }
    
    return true;
}

/**
 * Apply distribute modification to selected annotations
 *
 * @param annotationManager
 *     The annotation manager.
 * @param arrangerInputs
 *     Contains information about the alignment.
 * @param distribute
 *     The distribute selection.
 * @param errorMessageOut
 *     Contains description of error.
 * @return
 *     True if the alignment was successful, else false.
 */
bool
AnnotationArrangerExecutor::distributeAnnotations(AnnotationManager* annotationManager,
                                                  const AnnotationArrangerInputs& arrangerInputs,
                                                  const AnnotationDistributeEnum::Enum distribute,
                                                  AString& errorMessageOut)
{
    m_mode = MODE_DISTRIBUTE;
    
    m_annotationManager = annotationManager;
    CaretAssert(m_annotationManager);
    
    m_distribute = distribute;
    
    errorMessageOut.clear();
    
    try {
        distributeAnnotationsPrivate(arrangerInputs);
    }
    catch (const CaretException& caretException) {
        errorMessageOut = caretException.whatString();
        return false;
    }
    
    return true;
}

/**
 * \class caret::AnnotationArrangerExecutor::SortForDistributeFunctionObject
 * \brief Performs alignment, distribution of annotations similar to PowerPoint.
 * \ingroup Brain
 */
AnnotationArrangerExecutor::SortForDistributeFunctionObject::SortForDistributeFunctionObject(const AnnotationDistributeEnum::Enum distribute)
: m_distribute(distribute)
{
}

/**
 * Function for sorting by distribution value.
 * 
 * @param ann1
 *     First annotation info.
 * @param ann2 
 *     Second annotation info.
 * @return
 *     True if ann1 less than ann2, else false.
 */
bool
AnnotationArrangerExecutor::SortForDistributeFunctionObject::operator()(const AnnotationInfo& ann1,
                                                                        const AnnotationInfo& ann2) const {
    switch (m_distribute) {
        case AnnotationDistributeEnum::HORIZONTALLY:
            return (ann1.m_windowMidPointXY[0] < ann2.m_windowMidPointXY[0]);
            break;
        case AnnotationDistributeEnum::VERTICALLY:
            return (ann1.m_windowMidPointXY[1] < ann2.m_windowMidPointXY[1]);
            break;
    }
    
    return false;
}

/**
 * Apply distribute modification to selected annotations
 *
 * @param arrangerInputs
 *     Contains information about the distribute.
 * @throw  CaretException
 *     If there is an error.
 */
void
AnnotationArrangerExecutor::distributeAnnotationsPrivate(const AnnotationArrangerInputs& arrangerInputs)
{
    initializeForArranging(arrangerInputs);
    
    printAnnotationInfo("BEFORE");
    
    /*
     * Sort the annotation info by the window mid point value
     */
    std::sort(m_annotationInfo.begin(),
              m_annotationInfo.end(),
              SortForDistributeFunctionObject(m_distribute));
    
    printAnnotationInfo("AFTER SORT FOR DISTRIBUTE");
    
    float distMinValue = 0.0;
    float distMaxValue = 0.0;
    float distributeSum = 0.0;
    
    const int32_t numAnn = static_cast<int32_t>(m_annotationInfo.size());
    for (int32_t i = 0; i < numAnn; i++) {
        CaretAssertVectorIndex(m_annotationInfo, i);
        AnnotationInfo& annInfo = m_annotationInfo[i];
        
        switch (m_distribute) {
            case AnnotationDistributeEnum::HORIZONTALLY:
                if (i == 0) {
                    distMinValue = annInfo.m_windowBoundingBox.getMaxX();
                }
                else if (i == (numAnn - 1)) {
                    distMaxValue = annInfo.m_windowBoundingBox.getMinX();
                }
                else {
                    distributeSum += annInfo.m_windowBoundingBox.getDifferenceX();
                }
                break;
            case AnnotationDistributeEnum::VERTICALLY:
                if (i == 0) {
                    distMinValue = annInfo.m_windowBoundingBox.getMaxY();
                }
                else if (i == (numAnn - 1)) {
                    distMaxValue = annInfo.m_windowBoundingBox.getMinY();
                }
                else {
                    distributeSum += annInfo.m_windowBoundingBox.getDifferenceY();
                }
                break;
        }
    }
    
    /*
     * Space between max edge of first and min edge of last
     */
    const float availableSpace = distMaxValue - distMinValue;
    
    /*
     * Space to put between annotations
     */
    const float numberOfSpacesBetweenAnnotations = numAnn - 1;
    const float spaceBetweenAnnotations = ((availableSpace - distributeSum)
                                        / numberOfSpacesBetweenAnnotations);
    
    if (m_debugFlag) {
        std::cout << "Dist min max: ("
        << distMinValue << ", " << distMaxValue << ") availSpace="
        << availableSpace << " delta-space=" << spaceBetweenAnnotations << std::endl;
    }
    
    if (spaceBetweenAnnotations < 0.0) {
        throw CaretException("There is insufficient space between the left (bottom) most "
                             "and right (top) most annotation to perform distribution.");
        return;
    }
    
    std::vector<Annotation*> beforeMoving;
    std::vector<Annotation*> afterMoving;
    
    /*
     * Initialize to the right/top edge of the first annotation
     */
    float newCoordinate = distMinValue;
    
    for (int32_t i = 0; i < numAnn; i++) {
        CaretAssertVectorIndex(m_annotationInfo, i);
        AnnotationInfo& annInfo = m_annotationInfo[i];
        
        Annotation* annotationModified = annInfo.m_annotation->clone();
        
        if (i == 0) {
            /* first annotation does not move */
        }
        else if (i == (numAnn - 1)) {
            /* last annotation does not move */
        }
        else {
            /*
             * newCoordinate is now at the location of the
             * left/bottom edge of the annotation to be moved.
             */
            newCoordinate += spaceBetweenAnnotations;
            
            /*
             * For most annotations, the annotation's coordinate is at the
             * center of the annotation.  However, an exception is text annotation
             * due to their alignment properties.  So, we calculate how much the
             * left or bottom edge should move and apply that difference to the annotation's
             * coordinate.
             */
            float dx = 0.0;
            float dy = 0.0;
            float annotationWidthHeight = 0.0;
            switch (m_distribute) {
                case AnnotationDistributeEnum::HORIZONTALLY:
                {
                    dx = newCoordinate - annInfo.m_windowBoundingBox.getMinX();
                    annotationWidthHeight = annInfo.m_windowBoundingBox.getDifferenceX();
                }
                    break;
                case AnnotationDistributeEnum::VERTICALLY:
                {
                    dy = newCoordinate - annInfo.m_windowBoundingBox.getMinY();
                    annotationWidthHeight = annInfo.m_windowBoundingBox.getDifferenceY();
                }
                    break;
            }
            
            annInfo.moveAnnotationByXY(annotationModified,
                                       dx,
                                       dy);
            newCoordinate += annotationWidthHeight;
        }
        
        beforeMoving.push_back(annInfo.m_annotation);
        afterMoving.push_back(annotationModified);
    }
    
    /*
     * If any annotations were moved, use a redo command
     * so that the user may undo the alignment.
     */
    if ( ! beforeMoving.empty()) {
        CaretAssert(beforeMoving.size() == afterMoving.size());
        AnnotationRedoUndoCommand* undoCommand = new AnnotationRedoUndoCommand();
        undoCommand->setModeLocationAndSize(beforeMoving,
                                            afterMoving);
        undoCommand->setDescription(AnnotationDistributeEnum::toGuiName(m_distribute));
        
        m_annotationManager->applyCommand(undoCommand);
    }
    
    /*
     * The undo command copies these annotations so we need
     * to delete them.
     */
    for (std::vector<Annotation*>::iterator annIter = afterMoving.begin();
         annIter != afterMoving.end();
         annIter++) {
        Annotation* ann = *annIter;
        CaretAssert(ann);
        delete ann;
    }
}

///**
// * Apply distribute modification to selected annotations
// *
// * @param arrangerInputs
// *     Contains information about the distribute.
// * @throw  CaretException
// *     If there is an error.
// */
//void
//AnnotationArrangerExecutor::distributeAnnotationsPrivate(const AnnotationArrangerInputs& arrangerInputs)
//{
//    initializeForArranging(arrangerInputs);
//    
//    printAnnotationInfo("BEFORE");
//    
//    /*
//     * Sort the annotation info by the window mid point value
//     */
//    std::sort(m_annotationInfo.begin(),
//              m_annotationInfo.end(),
//              SortForDistributeFunctionObject(m_distribute));
//    
//    printAnnotationInfo("AFTER SORT FOR DISTRIBUTE");
//    
//    float distMinValue = 0.0;
//    float distMaxValue = 0.0;
//    float distributeSum = 0.0;
//    
//    const int32_t numAnn = static_cast<int32_t>(m_annotationInfo.size());
//    for (int32_t i = 0; i < numAnn; i++) {
//        CaretAssertVectorIndex(m_annotationInfo, i);
//        AnnotationInfo& annInfo = m_annotationInfo[i];
//        
//        switch (m_distribute) {
//            case AnnotationDistributeEnum::HORIZONTALLY:
//                if (i == 0) {
//                    distMinValue = annInfo.m_windowBoundingBox.getMaxX();
//                }
//                else if (i == (numAnn - 1)) {
//                    distMaxValue = annInfo.m_windowBoundingBox.getMinX();
//                }
//                else {
//                    distributeSum += annInfo.m_windowBoundingBox.getDifferenceX();
//                }
//                break;
//            case AnnotationDistributeEnum::VERTICALLY:
//                if (i == 0) {
//                    distMinValue = annInfo.m_windowBoundingBox.getMaxY();
//                }
//                else if (i == (numAnn - 1)) {
//                    distMaxValue = annInfo.m_windowBoundingBox.getMinY();
//                }
//                else {
//                    distributeSum += annInfo.m_windowBoundingBox.getDifferenceY();
//                }
//                break;
//        }
//    }
//    
//    /*
//     * Space between max edge of first and min edge of last
//     */
//    const float availableSpace = distMaxValue - distMinValue;
//    
//    /*
//     * Space to put between annotations
//     */
//    const float numberOfSpacesBetweenAnnotations = numAnn - 1;
//    const float annDeltaSpace = ((availableSpace - distributeSum)
//                                 / numberOfSpacesBetweenAnnotations);
//    
//    if (m_debugFlag) {
//        std::cout << "Dist min max: ("
//        << distMinValue << ", " << distMaxValue << ") availSpace="
//        << availableSpace << " delta-space=" << annDeltaSpace << std::endl;
//    }
//    
//    if (annDeltaSpace < 0.0) {
//        throw CaretException("There is insufficient space between the left (bottom) most "
//                             "and right (top) most annotation to perform distribution.");
//        return;
//    }
//    
//    std::vector<Annotation*> beforeMoving;
//    std::vector<Annotation*> afterMoving;
//    
//    /*
//     * Initialize to the right/top edge of the first annotation
//     */
//    float newCoordinate = distMinValue;
//    
//    for (int32_t i = 0; i < numAnn; i++) {
//        CaretAssertVectorIndex(m_annotationInfo, i);
//        AnnotationInfo& annInfo = m_annotationInfo[i];
//        
//        Annotation* annotationModified = annInfo.m_annotation->clone();
//        AnnotationOneDimensionalShape* oneDimAnn = dynamic_cast<AnnotationOneDimensionalShape*>(annotationModified);
//        AnnotationTwoDimensionalShape* twoDimAnn = dynamic_cast<AnnotationTwoDimensionalShape*>(annotationModified);
//        
//        if (i == 0) {
//            /* first annotation does not move */
//        }
//        else if (i == (numAnn - 1)) {
//            /* second annotation does not move */
//        }
//        else {
//            /*
//             * newCoordinate is now at the location of the
//             * left/bottom edge of the annotation to be moved.
//             */
//            newCoordinate += annDeltaSpace;
//            
//            switch (m_distribute) {
//                case AnnotationDistributeEnum::HORIZONTALLY:
//                {
//                    /*
//                     * For most annotations, the annotation's coordinate is at the 
//                     * center of the annotation.  However, an exception is text annotation
//                     * due to their alignment properties.  So, we calculate how much the
//                     * left edge should move and apply that difference to the annotation's
//                     * coordinate.
//                     */
//                    const float dx = newCoordinate - annInfo.m_windowBoundingBox.getMinX();
//                    const float annWidth = annInfo.m_windowBoundingBox.getDifferenceX();
//                    
//                    if (twoDimAnn != NULL) {
//                        const float newWindowX = annInfo.m_windowPixelOneXY[0] + dx;
//                        const float vpOneX       = newWindowX - annInfo.m_viewport[0];
//                        const float relativeOneX = (vpOneX / annInfo.m_viewport[2]) * 100.0;
//                        
//                        if ((relativeOneX >= 0.0)
//                            && (relativeOneX <= 100.0)) {
//                            float xyz[3];
//                            twoDimAnn->getCoordinate()->getXYZ(xyz);
//                            xyz[0] = relativeOneX;
//                            twoDimAnn->getCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    else if (oneDimAnn != NULL) {
//                        float windowOneX = annInfo.m_windowPixelOneXY[0] + dx;
//                        float windowTwoX = annInfo.m_windowPixelTwoXY[0] + dx;
//                        
//                        const float vpOneX       = windowOneX - annInfo.m_viewport[0];
//                        const float relativeOneX = (vpOneX / annInfo.m_viewport[2]) * 100.0;
//                        const float vpTwoX       = windowTwoX - annInfo.m_viewport[0];
//                        
//                        const float relativeTwoX = (vpTwoX / annInfo.m_viewport[2]) * 100.0;
//                        if ((relativeOneX >= 0.0)
//                            && (relativeOneX <= 100.0)
//                            && (relativeTwoX >= 0.0)
//                            && (relativeTwoX <= 100.0)) {
//                            float xyz[3];
//                            oneDimAnn->getStartCoordinate()->getXYZ(xyz);
//                            xyz[0] = relativeOneX;
//                            oneDimAnn->getStartCoordinate()->setXYZ(xyz);
//                            
//                            oneDimAnn->getEndCoordinate()->getXYZ(xyz);
//                            xyz[0] = relativeTwoX;
//                            oneDimAnn->getEndCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    
//                    /*
//                     * Move X-coordinate to right edge of this annotation
//                     */
//                    newCoordinate += annWidth;
//                }
//                    break;
//                case AnnotationDistributeEnum::VERTICALLY:
//                {
//                    const float dy = newCoordinate - annInfo.m_windowBoundingBox.getMinY();
//                    const float annHeight = annInfo.m_windowBoundingBox.getDifferenceY();
//                    if (twoDimAnn != NULL) {
//                        const float newWindowY = annInfo.m_windowPixelOneXY[1] + dy;
//                        const float vpOneY       = newWindowY - annInfo.m_viewport[1];
//                        const float relativeOneY = (vpOneY / annInfo.m_viewport[3]) * 100.0;
//                        if ((relativeOneY >= 0.0)
//                            && (relativeOneY <= 100.0)) {
//                            float xyz[3];
//                            twoDimAnn->getCoordinate()->getXYZ(xyz);
//                            xyz[1] = relativeOneY;
//                            twoDimAnn->getCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    else if (oneDimAnn != NULL) {
//                        float windowOneY = annInfo.m_windowPixelOneXY[1] + dy;
//                        float windowTwoY = annInfo.m_windowPixelTwoXY[1] + dy;
//                        
//                        const float vpOneY       = windowOneY - annInfo.m_viewport[1];
//                        const float relativeOneY = (vpOneY / annInfo.m_viewport[3]) * 100.0;
//                        const float vpTwoY       = windowTwoY - annInfo.m_viewport[1];
//                        const float relativeTwoY = (vpTwoY / annInfo.m_viewport[3]) * 100.0;
//                        if ((relativeOneY >= 0.0)
//                            && (relativeOneY <= 100.0)
//                            && (relativeTwoY >= 0.0)
//                            && (relativeTwoY <= 100.0)) {
//                            float xyz[3];
//                            oneDimAnn->getStartCoordinate()->getXYZ(xyz);
//                            xyz[1] = relativeOneY;
//                            oneDimAnn->getStartCoordinate()->setXYZ(xyz);
//                            
//                            oneDimAnn->getEndCoordinate()->getXYZ(xyz);
//                            xyz[1] = relativeTwoY;
//                            oneDimAnn->getEndCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    
//                    /*
//                     * Move Y-coordinate to top edge of this annotation
//                     */
//                    newCoordinate += annHeight;
//                }
//                    break;
//            }
//        }
//        
//        beforeMoving.push_back(annInfo.m_annotation);
//        afterMoving.push_back(annotationModified);
//    }
//    
//    /*
//     * If any annotations were moved, use a redo command
//     * so that the user may undo the alignment.
//     */
//    if ( ! beforeMoving.empty()) {
//        CaretAssert(beforeMoving.size() == afterMoving.size());
//        AnnotationRedoUndoCommand* undoCommand = new AnnotationRedoUndoCommand();
//        undoCommand->setModeLocationAndSize(beforeMoving,
//                                            afterMoving);
//        undoCommand->setDescription(AnnotationDistributeEnum::toGuiName(m_distribute));
//        
//        m_annotationManager->applyCommand(undoCommand);
//    }
//    
//    /*
//     * The undo command copies these annotations so we need
//     * to delete them.
//     */
//    for (std::vector<Annotation*>::iterator annIter = afterMoving.begin();
//         annIter != afterMoving.end();
//         annIter++) {
//        Annotation* ann = *annIter;
//        CaretAssert(ann);
//        delete ann;
//    }
//}

///**
// * Apply distribute modification to selected annotations
// *
// * @param arrangerInputs
// *     Contains information about the distribute.
// * @throw  CaretException
// *     If there is an error.
// */
//void
//AnnotationArrangerExecutor::distributeAnnotationsPrivate(const AnnotationArrangerInputs& arrangerInputs)
//{
//    initializeForArranging(arrangerInputs);
//
//    printAnnotationInfo("BEFORE");
//
//    /*
//     * Sort the annotation info by the window mid point value
//     */
//    std::sort(m_annotationInfo.begin(),
//              m_annotationInfo.end(),
//              SortForDistributeFunctionObject(m_distribute));
//    
//    printAnnotationInfo("AFTER SORT FOR DISTRIBUTE");
//    
//    float distMinValue = 0.0;
//    float distMaxValue = 0.0;
//    float distributeSum = 0.0;
//    
//    const int32_t numAnn = static_cast<int32_t>(m_annotationInfo.size());
//    for (int32_t i = 0; i < numAnn; i++) {
//        CaretAssertVectorIndex(m_annotationInfo, i);
//        AnnotationInfo& annInfo = m_annotationInfo[i];
//
//        switch (m_distribute) {
//            case AnnotationDistributeEnum::HORIZONTALLY:
//                if (i == 0) {
//                    distMinValue = annInfo.m_windowBoundingBox.getMaxX();
//                }
//                else if (i == (numAnn - 1)) {
//                    distMaxValue = annInfo.m_windowBoundingBox.getMinX();
//                }
//                else {
//                    distributeSum += annInfo.m_windowBoundingBox.getDifferenceX();
//                }
//                break;
//            case AnnotationDistributeEnum::VERTICALLY:
//                if (i == 0) {
//                    distMinValue = annInfo.m_windowBoundingBox.getMaxY();
//                }
//                else if (i == (numAnn - 1)) {
//                    distMaxValue = annInfo.m_windowBoundingBox.getMinY();
//                }
//                else {
//                    distributeSum += annInfo.m_windowBoundingBox.getDifferenceY();
//                }
//                break;
//        }
//    }
//    
//    /*
//     * Space between max edge of first and min edge of last
//     */
//    const float availableSpace = distMaxValue - distMinValue;
//    
//    /*
//     * Space to put between annotations
//     */
//    const float numberOfSpacesBetweenAnnotations = numAnn - 1;
//    const float annDeltaSpace = ((availableSpace - distributeSum)
//                                 / numberOfSpacesBetweenAnnotations);
//    
//    if (m_debugFlag) {
//        std::cout << "Dist min max: ("
//        << distMinValue << ", " << distMaxValue << ") availSpace="
//        << availableSpace << " delta-space=" << annDeltaSpace << std::endl;
//    }
//    
//    if (annDeltaSpace < 0.0) {
//        throw CaretException("There is insufficient space between the left (bottom) most "
//                             "and right (top) most annotation to perform distribution.");
//        return;
//    }
//    
//    std::vector<Annotation*> beforeMoving;
//    std::vector<Annotation*> afterMoving;
//    
//    float newCoordinate = distMinValue;
//    
//    for (int32_t i = 0; i < numAnn; i++) {
//        CaretAssertVectorIndex(m_annotationInfo, i);
//        AnnotationInfo& annInfo = m_annotationInfo[i];
//        
//        Annotation* annotationModified = annInfo.m_annotation->clone();
//        AnnotationOneDimensionalShape* oneDimAnn = dynamic_cast<AnnotationOneDimensionalShape*>(annotationModified);
//        AnnotationTwoDimensionalShape* twoDimAnn = dynamic_cast<AnnotationTwoDimensionalShape*>(annotationModified);
//        
//        if (i == 0) {
//            /* first annotation does not move */
//        }
//        else if (i == (numAnn - 1)) {
//            /* second annotation does not move */
//        }
//        else {
//            newCoordinate += annDeltaSpace;
//
//            switch (m_distribute) {
//                case AnnotationDistributeEnum::HORIZONTALLY:
//                {
//                    
//                    const float annWidth = annInfo.m_windowBoundingBox.getDifferenceX();
//                    if (twoDimAnn != NULL) {
//                        float textOffsetX = 0.0;
//                        const float halfAnnWidth = (annWidth / 2.0);
//                        const float windowOneX   = (newCoordinate
//                                                    + halfAnnWidth);
//                        if (twoDimAnn->getType() == AnnotationTypeEnum::TEXT) {
//                            const AnnotationText* textAnn = dynamic_cast<const AnnotationText*>(annInfo.m_annotation);
//                            switch (textAnn->getHorizontalAlignment()) {
//                                case AnnotationTextAlignHorizontalEnum::CENTER:
//                                    break;
//                                case AnnotationTextAlignHorizontalEnum::LEFT:
//                                    textOffsetX = -halfAnnWidth;
//                                    break;
//                                case AnnotationTextAlignHorizontalEnum::RIGHT:
//                                    textOffsetX = halfAnnWidth;
//                                    break;
//                            }
//                            std::cout << "Text offset: " << textOffsetX << std::endl;
//                        }
//                        const float vpOneX       = (windowOneX + textOffsetX) - annInfo.m_viewport[0];
//                        const float relativeOneX = (vpOneX / annInfo.m_viewport[2]) * 100.0;
//                        
//                        if ((relativeOneX >= 0.0)
//                            && (relativeOneX <= 100.0)) {
//                            float xyz[3];
//                            twoDimAnn->getCoordinate()->getXYZ(xyz);
//                            xyz[0] = relativeOneX;
//                            twoDimAnn->getCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    else if (oneDimAnn != NULL) {
//                        float windowOneX = 0.0;
//                        float windowTwoX = 0.0;
//                        if (annInfo.m_windowPixelOneXY[0] < annInfo.m_windowPixelTwoXY[0]) {
//                            windowOneX = newCoordinate;
//                            windowTwoX = newCoordinate + annWidth;
//                        }
//                        else {
//                            windowOneX = newCoordinate + annWidth;
//                            windowTwoX = newCoordinate;
//                        }
//                        
//                        const float vpOneX       = windowOneX - annInfo.m_viewport[0];
//                        const float relativeOneX = (vpOneX / annInfo.m_viewport[2]) * 100.0;
//                        const float vpTwoX       = windowTwoX - annInfo.m_viewport[0];
//                        
//                        const float relativeTwoX = (vpTwoX / annInfo.m_viewport[2]) * 100.0;
//                        if ((relativeOneX >= 0.0)
//                            && (relativeOneX <= 100.0)
//                            && (relativeTwoX >= 0.0)
//                            && (relativeTwoX <= 100.0)) {
//                            float xyz[3];
//                            oneDimAnn->getStartCoordinate()->getXYZ(xyz);
//                            xyz[0] = relativeOneX;
//                            oneDimAnn->getStartCoordinate()->setXYZ(xyz);
//                            
//                            oneDimAnn->getEndCoordinate()->getXYZ(xyz);
//                            xyz[0] = relativeTwoX;
//                            oneDimAnn->getEndCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    
//                    /*
//                     * Move X-coordinate to right edge of this annotation
//                     */
//                    newCoordinate += annWidth;
//                }
//                    break;
//                case AnnotationDistributeEnum::VERTICALLY:
//                {
//                    const float annHeight = annInfo.m_windowBoundingBox.getDifferenceY();
//                    if (twoDimAnn != NULL) {
//                        const float halfAnnHeight = (annHeight / 2.0);
//                        const float windowOneY   = (newCoordinate
//                                                    + halfAnnHeight);
//                        const float vpOneY       = windowOneY - annInfo.m_viewport[1];
//                        const float relativeOneY = (vpOneY / annInfo.m_viewport[3]) * 100.0;
//                        if ((relativeOneY >= 0.0)
//                            && (relativeOneY <= 100.0)) {
//                            float xyz[3];
//                            twoDimAnn->getCoordinate()->getXYZ(xyz);
//                            xyz[1] = relativeOneY;
//                            twoDimAnn->getCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    else if (oneDimAnn != NULL) {
//                        float windowOneY = 0.0;
//                        float windowTwoY = 0.0;
//                        if (annInfo.m_windowPixelOneXY[1] < annInfo.m_windowPixelTwoXY[1]) {
//                            windowOneY = newCoordinate;
//                            windowTwoY = newCoordinate + annHeight;
//                        }
//                        else {
//                            windowOneY = newCoordinate + annHeight;
//                            windowTwoY = newCoordinate;
//                        }
//                        const float vpOneY       = windowOneY - annInfo.m_viewport[1];
//                        const float relativeOneY = (vpOneY / annInfo.m_viewport[3]) * 100.0;
//                        const float vpTwoY       = windowTwoY - annInfo.m_viewport[1];
//                        const float relativeTwoY = (vpTwoY / annInfo.m_viewport[3]) * 100.0;
//                        if ((relativeOneY >= 0.0)
//                            && (relativeOneY <= 100.0)
//                            && (relativeTwoY >= 0.0)
//                            && (relativeTwoY <= 100.0)) {
//                            float xyz[3];
//                            oneDimAnn->getStartCoordinate()->getXYZ(xyz);
//                            xyz[1] = relativeOneY;
//                            oneDimAnn->getStartCoordinate()->setXYZ(xyz);
//                            
//                            oneDimAnn->getEndCoordinate()->getXYZ(xyz);
//                            xyz[1] = relativeTwoY;
//                            oneDimAnn->getEndCoordinate()->setXYZ(xyz);
//                        }
//                    }
//                    
//                    /*
//                     * Move Y-coordinate to top edge of this annotation
//                     */
//                    newCoordinate += annHeight;
//                }
//                    break;
//            }
//        }
//        
//        beforeMoving.push_back(annInfo.m_annotation);
//        afterMoving.push_back(annotationModified);
//    }
//    
//    /*
//     * If any annotations were moved, use a redo command
//     * so that the user may undo the alignment.
//     */
//    if ( ! beforeMoving.empty()) {
//        CaretAssert(beforeMoving.size() == afterMoving.size());
//        AnnotationRedoUndoCommand* undoCommand = new AnnotationRedoUndoCommand();
//        undoCommand->setModeLocationAndSize(beforeMoving,
//                                            afterMoving);
//        undoCommand->setDescription(AnnotationDistributeEnum::toGuiName(m_distribute));
//        
//        m_annotationManager->applyCommand(undoCommand);
//    }
//    
//    /*
//     * The undo command copies these annotations so we need
//     * to delete them.
//     */
//    for (std::vector<Annotation*>::iterator annIter = afterMoving.begin();
//         annIter != afterMoving.end();
//         annIter++) {
//        Annotation* ann = *annIter;
//        CaretAssert(ann);
//        delete ann;
//    }
//}

/**
 * Apply alignment modification to selected annotations
 *
 * @param arrangerInputs
 *     Contains information about the alignment.
 * @throw  CaretException
 *     If there is an error.
 */
void
AnnotationArrangerExecutor::alignAnnotationsPrivate(const AnnotationArrangerInputs& arrangerInputs)
{
    switch (m_mode) {
        case MODE_NONE:
            CaretAssertMessage(0, "Mode has not been set !!!");
            throw CaretException("Mode has not been set !!!");
            break;
        case MODE_ALIGN:
            break;
        case MODE_DISTRIBUTE:
            break;
    }
    
    initializeForArranging(arrangerInputs);
    
    printAnnotationInfo("BEFORE");
    
    /*
     * Set the coordinate value to which annotations are aligned
     * based upon the selected alignment.
     */
    float alignToCoordinateValue = 0.0;
    switch (m_alignment) {
        case AnnotationAlignmentEnum::ALIGN_BOTTOM:
            alignToCoordinateValue = m_allAnnotationsBoundingBox.getMinY();
            break;
        case AnnotationAlignmentEnum::ALIGN_CENTER:
            alignToCoordinateValue = (m_allAnnotationsBoundingBox.getMinX()
                               + m_allAnnotationsBoundingBox.getMaxX()) / 2.0;
            break;
        case AnnotationAlignmentEnum::ALIGN_LEFT:
            alignToCoordinateValue = m_allAnnotationsBoundingBox.getMinX();
            break;
        case AnnotationAlignmentEnum::ALIGN_MIDDLE:
            alignToCoordinateValue = (m_allAnnotationsBoundingBox.getMinY()
                               + m_allAnnotationsBoundingBox.getMaxY()) / 2.0;
            break;
        case AnnotationAlignmentEnum::ALIGN_RIGHT:
            alignToCoordinateValue = m_allAnnotationsBoundingBox.getMaxX();
            break;
        case AnnotationAlignmentEnum::ALIGN_TOP:
            alignToCoordinateValue = m_allAnnotationsBoundingBox.getMaxY();
            break;
    }
    
    if (m_debugFlag) {
        std::cout << "Bounding box for all: " << qPrintable(m_allAnnotationsBoundingBox.toString()) << std::endl;
        std::cout << "New Align to value: " << alignToCoordinateValue << std::endl;
    }
    
    /*
     * Move each of the annotations to align them.
     */
    std::vector<Annotation*> beforeMoving;
    std::vector<Annotation*> afterMoving;
    for (std::vector<AnnotationInfo>::iterator annInfoIter = m_annotationInfo.begin();
         annInfoIter != m_annotationInfo.end();
         annInfoIter++) {
        
        AnnotationInfo& annInfo = *annInfoIter;
        alignAnnotationToValue(alignToCoordinateValue,
                               annInfo,
                               beforeMoving,
                               afterMoving);
    }
    
    /*
     * If any annotations were moved, use a redo command
     * so that the user may undo the alignment.
     */
    if ( ! beforeMoving.empty()) {
        CaretAssert(beforeMoving.size() == afterMoving.size());
        AnnotationRedoUndoCommand* undoCommand = new AnnotationRedoUndoCommand();
        undoCommand->setModeLocationAndSize(beforeMoving,
                                            afterMoving);
        undoCommand->setDescription(AnnotationAlignmentEnum::toGuiName(m_alignment));
        
        m_annotationManager->applyCommand(undoCommand);
    }
    
    /*
     * The undo command copies these annotations so we need
     * to delete them.
     */
    for (std::vector<Annotation*>::iterator annIter = afterMoving.begin();
         annIter != afterMoving.end();
         annIter++) {
        Annotation* ann = *annIter;
        CaretAssert(ann);
        delete ann;
    }
}

/**
 * Initialize members in preparation for arranging annotations.
 *
 * @param arrangerInputs
 *     Inputs for arranging annotations.
 */
void
AnnotationArrangerExecutor::initializeForArranging(const AnnotationArrangerInputs& arrangerInputs)
{
    /*
     * Get the window's viewport.
     */
    EventGetViewportSize vpEvent(EventGetViewportSize::MODE_WINDOW_INDEX,
                                 arrangerInputs.getWindowIndex());
    EventManager::get()->sendEvent(vpEvent.getPointer());
    if (vpEvent.isViewportSizeValid()) {
        vpEvent.getViewportSize(m_windowViewport);
    }
    else {
        throw CaretException("Failed to get viewport size for window index "
                             + QString::number(arrangerInputs.getWindowIndex() + 1));
    }
    
    /*
     * Verify that selected annotations can be arranged.
     */
    std::vector<Annotation*> annotations;
    getAnnotationsForArranging(arrangerInputs,
                               annotations);
    
    /**
     * Setup information to assist with arranging each annotation.
     */
    setupAnnotationInfo(arrangerInputs,
                        annotations);
    
    /**
     * Setup a bounding box that contains the bounds 
     * of all annotations.
     */
    m_allAnnotationsBoundingBox.resetForUpdate();
    
    for (std::vector<AnnotationInfo>::iterator annInfoIter = m_annotationInfo.begin();
         annInfoIter != m_annotationInfo.end();
         annInfoIter++) {
        const AnnotationInfo& annInfo = *annInfoIter;
        
        m_allAnnotationsBoundingBox.update(annInfo.m_windowBoundingBox.getMinXYZ());
        m_allAnnotationsBoundingBox.update(annInfo.m_windowBoundingBox.getMaxXYZ());
    }
}

/**
 * Get the annotations for arranging.  Not all coordinate spaces are supported.
 *
 * @param arrangerInputs
 *     Inputs for arranging annotations.
 * @param annotationsOut
 *    On output contains annotations that will be arranged.
 * @throw CaretException
 *    If there are input annotations that cannot be aligned.
 */
void
AnnotationArrangerExecutor::getAnnotationsForArranging(const AnnotationArrangerInputs& arrangerInputs,
                                                       std::vector<Annotation*>& annotationsOut) const
{
    annotationsOut.clear();
    
    std::vector<AnnotationCoordinateSpaceEnum::Enum> spaces;
    spaces.push_back(AnnotationCoordinateSpaceEnum::TAB);
    spaces.push_back(AnnotationCoordinateSpaceEnum::WINDOW);
    
    
    const std::vector<Annotation*> allSpaceAnnotations = m_annotationManager->getSelectedAnnotations(arrangerInputs.getWindowIndex());
    annotationsOut = m_annotationManager->getSelectedAnnotationsInSpaces(arrangerInputs.getWindowIndex(),
                                                                                               spaces);
    if (allSpaceAnnotations.size() != annotationsOut.size()) {
        QString spaceString;
        for (std::vector<AnnotationCoordinateSpaceEnum::Enum>::iterator spaceIter = spaces.begin();
             spaceIter != spaces.end();
             spaceIter++) {
            spaceString += ("\n   " + AnnotationCoordinateSpaceEnum::toGuiName(*spaceIter) + " ");
        }
        
        throw CaretException("Annotations are selected that cannot be aligned due to their coordinate space.  "
                             "Annotations MUST BE in one of these spaces:"
                             + spaceString);
    }
    
    const int32_t numAnnotations = static_cast<int32_t>(annotationsOut.size());
    if (numAnnotations < 1) {
        throw CaretException("No annotations are selected.");
    }
    
    switch (m_mode) {
        case MODE_NONE:
            CaretAssert(0);
            break;
        case MODE_ALIGN:
            if (numAnnotations < 2) {
                throw CaretException("At least two annotations must be selected for alignment.");
            }
            break;
        case MODE_DISTRIBUTE:
            if (numAnnotations < 3) {
                throw CaretException("At least three annotations must be selected for distributing.");
            }
            break;
    }
}


/**
 * Setup the annotation information that contains the bounds of
 * the annotation in window coordinates and other information.
 *
 * @param arrangerInputs
 *     Inputs for arranging annotations.
 * @param annotations
 *     Contains annotations that will be arranged.
 * @throw CaretException
 *    If there are input annotations that cannot be aligned.
 */
void
AnnotationArrangerExecutor::setupAnnotationInfo(const AnnotationArrangerInputs& arrangerInputs,
                                                std::vector<Annotation*>& annotations)
{
    m_annotationInfo.clear();

    for (std::vector<Annotation*>::iterator annIter = annotations.begin();
         annIter != annotations.end();
         annIter++) {
        Annotation* annotation = *annIter;
        CaretAssert(annotation);
        
        /*
         * Viewport containing the annotation.
         */
        int32_t annViewport[4] = { 0, 0, 0, 0 };
        switch (annotation->getCoordinateSpace()) {
            case AnnotationCoordinateSpaceEnum::PIXELS:
                CaretAssert(0);
                break;
            case AnnotationCoordinateSpaceEnum::STEREOTAXIC:
                CaretAssert(0);
                break;
            case AnnotationCoordinateSpaceEnum::SURFACE:
                CaretAssert(0);
                break;
            case AnnotationCoordinateSpaceEnum::TAB:
                getTabViewport(annotation->getTabIndex(),
                               annViewport);
                break;
            case AnnotationCoordinateSpaceEnum::WINDOW:
                annViewport[0] = m_windowViewport[0];
                annViewport[1] = m_windowViewport[1];
                annViewport[2] = m_windowViewport[2];
                annViewport[3] = m_windowViewport[3];
                break;
        }
        
        const AnnotationOneDimensionalShape* oneDimAnn = dynamic_cast<const AnnotationOneDimensionalShape*>(annotation);
        const AnnotationTwoDimensionalShape* twoDimAnn = dynamic_cast<const AnnotationTwoDimensionalShape*>(annotation);
        
        float viewportPixelOneXYZ[3] = { 0.0, 0.0, 0.0 };
        float viewportPixelTwoXYZ[3] = { 0.0, 0.0, 0.0 };
        bool viewportPixelTwoXYZValidFlag = false;
        
        /*
         * Bounding box for the annotation.
         */
        BoundingBox windowBoundingBox;
        windowBoundingBox.resetForUpdate();
        
        if (oneDimAnn != NULL) {
            float xyzRel1[3];
            oneDimAnn->getStartCoordinate()->getXYZ(xyzRel1);
            float xyzRel2[3];
            oneDimAnn->getEndCoordinate()->getXYZ(xyzRel2);
            
            /*
             * Convert range [0.0, 100.0] to viewport XY.
             */
            Annotation::relativeXYZToViewportXYZ(xyzRel1,
                                                 annViewport[2],
                                                 annViewport[3],
                                                 viewportPixelOneXYZ);
            Annotation::relativeXYZToViewportXYZ(xyzRel2,
                                                 annViewport[2],
                                                 annViewport[3],
                                                 viewportPixelTwoXYZ);
            
            windowBoundingBox.update(annViewport[0] + viewportPixelOneXYZ[0],
                                     annViewport[1] + viewportPixelOneXYZ[1],
                                     viewportPixelOneXYZ[2]);
            windowBoundingBox.update(annViewport[0] + viewportPixelTwoXYZ[0],
                                     annViewport[1] + viewportPixelTwoXYZ[1],
                                     viewportPixelTwoXYZ[2]);
            viewportPixelTwoXYZValidFlag = true;
        }
        else if (twoDimAnn != NULL) {
            float xyz[3];
            twoDimAnn->getCoordinate()->getXYZ(xyz);
            
            /*
             * Convert range [0.0, 100.0] to viewport XY.
             */
            Annotation::relativeXYZToViewportXYZ(xyz,
                                                 annViewport[2],
                                                 annViewport[3],
                                                 viewportPixelOneXYZ);
            
            float bottomLeft[3];
            float bottomRight[3];
            float topRight[3];
            float topLeft[3];
            
            bool boundsValidFlag = false;
            
            if (twoDimAnn->getType() == AnnotationTypeEnum::TEXT) {
                const AnnotationText* textAnn = dynamic_cast<const AnnotationText*>(twoDimAnn);
                CaretAssert(textAnn);
                arrangerInputs.getTextRender()->getBoundsForTextAtViewportCoords(*textAnn,
                                                                                 viewportPixelOneXYZ[0],
                                                                                 viewportPixelOneXYZ[1],
                                                                                 viewportPixelOneXYZ[2],
                                                                                 annViewport[3],
                                                                                 bottomLeft,
                                                                                 bottomRight,
                                                                                 topRight,
                                                                                 topLeft);
                boundsValidFlag = true;
            }
            else {
                boundsValidFlag = twoDimAnn->getShapeBounds(annViewport[2],
                                                            annViewport[3],
                                                            viewportPixelOneXYZ,
                                                            bottomLeft,
                                                            bottomRight,
                                                            topRight,
                                                            topLeft);
            }
            
            if (boundsValidFlag) {
                windowBoundingBox.update(annViewport[0] + bottomLeft[0],
                                         annViewport[1] + bottomLeft[1],
                                         bottomLeft[2]);
                windowBoundingBox.update(annViewport[0] + bottomRight[0],
                                         annViewport[1] + bottomRight[1],
                                         bottomRight[2]);
                windowBoundingBox.update(annViewport[0] + topRight[0],
                                         annViewport[1] + topRight[1],
                                         topRight[2]);
                windowBoundingBox.update(annViewport[0] + topLeft[0],
                                         annViewport[1] + topLeft[1],
                                         topLeft[2]);
            }
            else {
                throw CaretException("Failed to get bounds for an annotation: "
                                     + twoDimAnn->toString());
            }
        }
        
        AnnotationInfo annInfo(annotation,
                               annViewport,
                               windowBoundingBox,
                               viewportPixelOneXYZ,
                               viewportPixelTwoXYZ,
                               viewportPixelTwoXYZValidFlag);
        m_annotationInfo.push_back(annInfo);
    }
    
    CaretAssert(annotations.size() == m_annotationInfo.size());
}

/**
 * Print the annotation info.
 * 
 * @param title
 *     Title that is printed before the annotation
 *     information is printed.
 */
void
AnnotationArrangerExecutor::printAnnotationInfo(const QString& title)
{
    if ( ! m_debugFlag) {
        return;
    }
    
    std::cout << std::endl;
    std::cout << qPrintable(title) << std::endl;
    for (std::vector<AnnotationInfo>::iterator annInfoIter = m_annotationInfo.begin();
         annInfoIter != m_annotationInfo.end();
         annInfoIter++) {
        const AnnotationInfo& annInfo = *annInfoIter;
        annInfo.print();
    }
    std::cout << std::endl;
}


/**
 * Align an annotation to the given window coordinate value.
 *
 * @param alignToWindowCoordinateValue
 *     Align to window coordinate value.
 * @param annotationInfo
 *     Information for annotation that is aligned.
 * @param annotationsBeforeMoving
 *     If an annotation is moved, upon exit this will
 *     contain the annotation before it is moved and
 *     is used by the redo/undo command.
 * @param annotationsAfterMoving
 *     If an annotation is moved, upon exit this will
 *     contain the annotation after it is moved and
 *     is used by the redo/undo command.
 * @throw  CaretException
 *     If there is an error.
 */
void
AnnotationArrangerExecutor::alignAnnotationToValue(const float alignToWindowCoordinateValue,
                                                   AnnotationInfo& annotationInfo,
                                                   std::vector<Annotation*>& annotationsBeforeMoving,
                                                   std::vector<Annotation*>& annotationsAfterMoving)
{
    /*
     * Set amount of movement needed to align the annotation.
     */
    float dx = 0.0;
    float dy = 0.0;
    switch (m_alignment) {
        case AnnotationAlignmentEnum::ALIGN_BOTTOM:
            dy = alignToWindowCoordinateValue - annotationInfo.m_windowBoundingBox.getMinY();
            break;
        case AnnotationAlignmentEnum::ALIGN_CENTER:
            dx = alignToWindowCoordinateValue - (annotationInfo.m_windowBoundingBox.getMinX()
                                       + annotationInfo.m_windowBoundingBox.getMaxX()) / 2.0;
            break;
        case AnnotationAlignmentEnum::ALIGN_LEFT:
            dx = alignToWindowCoordinateValue - annotationInfo.m_windowBoundingBox.getMinX();
            break;
        case AnnotationAlignmentEnum::ALIGN_MIDDLE:
            dy = alignToWindowCoordinateValue - (annotationInfo.m_windowBoundingBox.getMinY()
                                       + annotationInfo.m_windowBoundingBox.getMaxY()) / 2.0;
            break;
        case AnnotationAlignmentEnum::ALIGN_RIGHT:
            dx = alignToWindowCoordinateValue - annotationInfo.m_windowBoundingBox.getMaxX();
            break;
        case AnnotationAlignmentEnum::ALIGN_TOP:
            dy = alignToWindowCoordinateValue - annotationInfo.m_windowBoundingBox.getMaxY();
            break;
    }
    
    if (m_debugFlag) {
        std::cout << "Moving " << qPrintable(annotationInfo.m_annotation->toString()) << " by dx=" << dx << ", dy=" << dy << std::endl;
    }
    
    Annotation* annotationModified = annotationInfo.m_annotation->clone();
    AnnotationOneDimensionalShape* oneDimAnn = dynamic_cast<AnnotationOneDimensionalShape*>(annotationModified);
    AnnotationTwoDimensionalShape* twoDimAnn = dynamic_cast<AnnotationTwoDimensionalShape*>(annotationModified);
    
    bool modifiedFlag = false;
    
    /*
     * Move the annotation.
     */
    if (dy != 0.0) {
        const float windowOneY   = dy + annotationInfo.m_windowPixelOneXY[1];
        const float vpOneY       = windowOneY - annotationInfo.m_viewport[1];
        const float relativeOneY = (vpOneY / annotationInfo.m_viewport[3]) * 100.0;
        if (twoDimAnn != NULL) {
            if ((relativeOneY >= 0.0)
                && (relativeOneY <= 100.0)) {
                float xyz[3];
                twoDimAnn->getCoordinate()->getXYZ(xyz);
                xyz[1] = relativeOneY;
                twoDimAnn->getCoordinate()->setXYZ(xyz);
                modifiedFlag = true;
            }
        }
        else if (oneDimAnn != NULL) {
            const float windowTwoY   = dy + annotationInfo.m_windowPixelTwoXY[1];
            const float vpTwoY       = windowTwoY - annotationInfo.m_viewport[1];
            const float relativeTwoY = (vpTwoY / annotationInfo.m_viewport[3]) * 100.0;
            if ((relativeOneY >= 0.0)
                && (relativeOneY <= 100.0)
                && (relativeTwoY >= 0.0)
                && (relativeTwoY <= 100.0)) {
                float xyz[3];
                oneDimAnn->getStartCoordinate()->getXYZ(xyz);
                xyz[1] = relativeOneY;
                oneDimAnn->getStartCoordinate()->setXYZ(xyz);

                oneDimAnn->getEndCoordinate()->getXYZ(xyz);
                xyz[1] = relativeTwoY;
                oneDimAnn->getEndCoordinate()->setXYZ(xyz);
                modifiedFlag = true;
            }
        }
    }
    
    if (dx != 0.0) {
        const float windowOneX   = dx + annotationInfo.m_windowPixelOneXY[0];
        const float vpOneX       = windowOneX - annotationInfo.m_viewport[0];
        const float relativeOneX = (vpOneX / annotationInfo.m_viewport[2]) * 100.0;
        
        if (twoDimAnn != NULL) {
            if ((relativeOneX >= 0.0)
                && (relativeOneX <= 100.0)) {
                float xyz[3];
                twoDimAnn->getCoordinate()->getXYZ(xyz);
                xyz[0] = relativeOneX;
                twoDimAnn->getCoordinate()->setXYZ(xyz);
                modifiedFlag = true;
            }
        }
        else if (oneDimAnn != NULL) {
            const float windowTwoX   = dx + annotationInfo.m_windowPixelTwoXY[0];
            const float vpTwoX       = windowTwoX - annotationInfo.m_viewport[0];
            const float relativeTwoX = (vpTwoX / annotationInfo.m_viewport[2]) * 100.0;
            if ((relativeOneX >= 0.0)
                && (relativeOneX <= 100.0)
                && (relativeTwoX >= 0.0)
                && (relativeTwoX <= 100.0)) {
                float xyz[3];
                oneDimAnn->getStartCoordinate()->getXYZ(xyz);
                xyz[0] = relativeOneX;
                oneDimAnn->getStartCoordinate()->setXYZ(xyz);
                
                oneDimAnn->getEndCoordinate()->getXYZ(xyz);
                xyz[0] = relativeTwoX;
                oneDimAnn->getEndCoordinate()->setXYZ(xyz);
                modifiedFlag = true;
            }
        }
    }
    
    if (modifiedFlag) {
        annotationsBeforeMoving.push_back(annotationInfo.m_annotation);
        annotationsAfterMoving.push_back(annotationModified);
    }
    else {
        delete annotationModified;
    }
}

/**
 * Get the viewport for the given tab.  Viewports are cached for efficiency.
 *
 * @param tabIndex
 *     Index of the tab.
 * @param tabViewportOut
 *     Viewport of the tab.
 * @throw  CaretException
 *     If there is an error.
 */
void
AnnotationArrangerExecutor::getTabViewport(const int32_t tabIndex,
                                           int32_t tabViewportOut[4])
{
    
    std::map<int32_t, ViewportArray>::iterator iter = m_tabViewports.find(tabIndex);
    if (iter != m_tabViewports.end()) {
        tabViewportOut[0] = iter->second.m_viewport[0];
        tabViewportOut[1] = iter->second.m_viewport[1];
        tabViewportOut[2] = iter->second.m_viewport[2];
        tabViewportOut[3] = iter->second.m_viewport[3];
        return;
    }
    
    EventGetViewportSize vpEvent(EventGetViewportSize::MODE_TAB_AFTER_MARGINS_INDEX,
                                 tabIndex);
    EventManager::get()->sendEvent(vpEvent.getPointer());
    if (vpEvent.isViewportSizeValid()) {
        ViewportArray viewportArray;
        vpEvent.getViewportSize(tabViewportOut);
        
        viewportArray.m_viewport[0] = tabViewportOut[0];
        viewportArray.m_viewport[1] = tabViewportOut[1];
        viewportArray.m_viewport[2] = tabViewportOut[2];
        viewportArray.m_viewport[3] = tabViewportOut[3];
        m_tabViewports.insert(std::make_pair(tabIndex, viewportArray));
    }
    else {
        throw CaretException("Failed to get viewport size for tab index "
                             + QString::number(tabIndex + 1));
    }
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString
AnnotationArrangerExecutor::toString() const
{
    return "AnnotationArrangerExecutor";
}



/**
 * \class caret::AnnotationInfo
 * \brief Contains information used to arrange annotations.
 * \ingroup Brain
 */

/**
 * Constructor.
 *
 * @param annotation
 *     The annotation.
 * @param viewport
 *     Viewport coordinates containing the annotation.
 * @param windowBoundingBox
 *     Bounding box for the annotation in window coordinates.
 * @param viewportPixelOneXY
 *     XY coordinate of the annotation in its viewport.
 * @param viewportPixelTwoXY
 *     optional second XY coordinate of the annotation in its viewport.
 * @param viewportPixelTwoXYValidValid
 *     True if optional second XY coordinate is valid.
 */
AnnotationArrangerExecutor::AnnotationInfo::AnnotationInfo(Annotation* annotation,
                                                           const int32_t viewport[4],
                                                           const BoundingBox windowBoundingBox,
                                                           float viewportPixelOneXY[2],
                                                           float viewportPixelTwoXY[2],
                                                           const bool viewportPixelTwoXYValidValid)
: m_annotation(annotation)
{
    m_viewport[0] = viewport[0];
    m_viewport[1] = viewport[1];
    m_viewport[2] = viewport[2];
    m_viewport[3] = viewport[3];
    
    m_viewportPixelOneXY[0] = viewportPixelOneXY[0];
    m_viewportPixelOneXY[1] = viewportPixelOneXY[1];
    
    m_viewportPixelTwoXY[0] = viewportPixelTwoXY[0];
    m_viewportPixelTwoXY[1] = viewportPixelTwoXY[1];
    
    m_windowBoundingBox = windowBoundingBox;
    
    m_windowPixelOneXY[0] = m_viewport[0] + viewportPixelOneXY[0];
    m_windowPixelOneXY[1] = m_viewport[1] + viewportPixelOneXY[1];
    
    m_windowPixelTwoXY[0] = m_viewport[0] + viewportPixelTwoXY[0];
    m_windowPixelTwoXY[1] = m_viewport[1] + viewportPixelTwoXY[1];
    m_windowPixelTwoXYValidFlag = viewportPixelTwoXYValidValid;
    
    if (m_windowPixelTwoXYValidFlag) {
        m_windowMidPointXY[0] = (m_windowPixelOneXY[0] + m_windowPixelTwoXY[0]) / 2.0;
        m_windowMidPointXY[1] = (m_windowPixelOneXY[1] + m_windowPixelTwoXY[1]) / 2.0;
    }
    else {
        m_windowMidPointXY[0] = m_windowPixelOneXY[0];
        m_windowMidPointXY[1] = m_windowPixelOneXY[1];
    }
}

/**
 * Is this XY coordinate a valid relative coordinate,
 * in the range [0.0, 100.0]?
 *
 * @return x
 *     The x-coordinate
 * @param y
 *     The y-coordinate
 * @return
 *     True if valid relative coordinate, else false.
 */
bool
AnnotationArrangerExecutor::AnnotationInfo::isValidRelativeCoord(const float x, const float y) const
{
    if ((x >= 0.0)
        && (x <= 100.0)
        && (y >= 0.0)
        && (y <= 100.0)) {
        return true;
    }
    return false;
}

/**
 * Move the given annotation by the given amounts.
 * The annotation is moved only if its new coordinates
 * remain in the viewport.
 *
 * @return x
 *     The x-coordinate
 * @param y
 *     The y-coordinate
 * @return
 *     True if annotation was moved, else false.
 */bool
AnnotationArrangerExecutor::AnnotationInfo::moveAnnotationByXY(Annotation* annotation,
                                                               const float dx,
                                                               const float dy) const
{
    CaretAssert(m_annotation);
    AnnotationOneDimensionalShape* oneDimAnn = dynamic_cast<AnnotationOneDimensionalShape*>(annotation);
    AnnotationTwoDimensionalShape* twoDimAnn = dynamic_cast<AnnotationTwoDimensionalShape*>(annotation);
    
    const float newPixelOneX = m_viewportPixelOneXY[0] + dx;
    const float relativeOneX = (newPixelOneX / m_viewport[2]) * 100.0;
    const float newPixelOneY = m_viewportPixelOneXY[1] + dy;
    const float relativeOneY = (newPixelOneY / m_viewport[3]) * 100.0;
    
    if (isValidRelativeCoord(relativeOneX,
                             relativeOneY)) {
        if (twoDimAnn != NULL) {
            float xyz[3];
            twoDimAnn->getCoordinate()->getXYZ(xyz);
            xyz[0] = relativeOneX;
            xyz[1] = relativeOneY;
            twoDimAnn->getCoordinate()->setXYZ(xyz);
            
            return true;
        }
        else if (oneDimAnn) {
            const float newPixelTwoX = m_viewportPixelTwoXY[0] + dx;
            const float relativeTwoX = (newPixelTwoX / m_viewport[2]) * 100.0;
            const float newPixelTwoY = m_viewportPixelTwoXY[1] + dy;
            const float relativeTwoY = (newPixelTwoY / m_viewport[3]) * 100.0;
            
            if (isValidRelativeCoord(relativeTwoX, relativeTwoY)) {
                float xyz[3];
                oneDimAnn->getStartCoordinate()->getXYZ(xyz);
                xyz[0] = relativeOneX;
                xyz[1] = relativeOneY;
                oneDimAnn->getStartCoordinate()->setXYZ(xyz);
                
                oneDimAnn->getEndCoordinate()->getXYZ(xyz);
                xyz[0] = relativeTwoX;
                xyz[1] = relativeTwoY;
                oneDimAnn->getEndCoordinate()->setXYZ(xyz);
                
                return true;
            }
        }
    }
    return false;
}


/**
 * Print information used to arrange an annotation.
 */
void
AnnotationArrangerExecutor::AnnotationInfo::print() const
{
    QString msg(m_annotation->toString()
                + "\n   Window One XY: "
                + AString::fromNumbers(m_windowPixelOneXY, 2, ",")
                + "\n   Window Two XY: "
                + AString::fromNumbers(m_windowPixelTwoXY, 2, ",")
                + "\n   Window Mid Point XY: "
                + AString::fromNumbers(m_windowMidPointXY, 2, ",")
                + "\n   Viewport XY: "
                + AString::fromNumbers(m_viewportPixelOneXY, 2, ",")
                + "\n   Viewport: "
                + AString::fromNumbers(m_viewport, 4, ",")
                + "\n   Bounds: "
                + m_windowBoundingBox.toString());
    std::cout << qPrintable(msg) << std::endl;
}