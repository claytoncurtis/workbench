
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

#define __CHART_TWO_OVERLAY_VIEW_CONTROLLER_DECLARE__
#include "ChartTwoOverlayViewController.h"
#undef __CHART_TWO_OVERLAY_VIEW_CONTROLLER_DECLARE__

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QSpinBox>
#include <QToolButton>

#include "CaretAssert.h"
using namespace caret;

#include "AnnotationColorBar.h"
#include "CaretMappableDataFile.h"
#include "ChartTwoOverlay.h"
#include "ChartableTwoFileDelegate.h"
#include "ChartableTwoFileHistogramChart.h"
#include "ChartableTwoFileLineSeriesChart.h"
#include "ChartableTwoFileMatrixChart.h"
#include "EventDataFileReload.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventGraphicsUpdateOneWindow.h"
#include "EventManager.h"
#include "EventMapYokingSelectMap.h"
#include "EventOverlaySettingsEditorDialogRequest.h"
#include "EventSurfaceColoringInvalidate.h"
#include "EventUserInterfaceUpdate.h"
#include "FileInformation.h"
#include "FilePathNamePrefixCompactor.h"
#include "GuiManager.h"
#include "MapYokingGroupComboBox.h"
#include "UsernamePasswordWidget.h"
#include "WuQFactory.h"
#include "WuQMessageBox.h"
#include "WuQtUtilities.h"

/**
 * \class caret::ChartTwoOverlayViewController 
 * \brief View controller for a chart overlay
 * \ingroup GuiQt
 */

/**
 * Constructor.
 *
 * @param orientation
 *    Orientation of view controller.
 * @param browserWindowIndex
 *    Index of browser window in which this view controller resides.
 * @param chartOverlayIndex
 *    Index of this overlay view controller.
 * @param parent
 *    The parent widget.
 */
ChartTwoOverlayViewController::ChartTwoOverlayViewController(const Qt::Orientation orientation,
                                                       const int32_t browserWindowIndex,
                                                       const int32_t chartOverlayIndex,
                                                       QObject* parent)
: QObject(parent),
m_browserWindowIndex(browserWindowIndex),
m_chartOverlayIndex(chartOverlayIndex),
m_chartOverlay(NULL)
{
    int minComboBoxWidth = 200;
    int maxComboBoxWidth = 100000; //400;
    if (orientation == Qt::Horizontal) {
        minComboBoxWidth = 50;
        maxComboBoxWidth = 100000;
    }
    const QComboBox::SizeAdjustPolicy comboSizePolicy = QComboBox::AdjustToContentsOnFirstShow; //QComboBox::AdjustToContents;
    
    /*
     * Enabled Check Box
     */
    const QString checkboxText = ((orientation == Qt::Horizontal) ? " " : " ");
    m_enabledCheckBox = new QCheckBox(checkboxText);
    QObject::connect(m_enabledCheckBox, SIGNAL(clicked(bool)),
                     this, SLOT(enabledCheckBoxClicked(bool)));
    m_enabledCheckBox->setToolTip("Enables display of this layer");
    
    /*
     * Settings Tool Button
     */
    QIcon settingsIcon;
    const bool settingsIconValid = WuQtUtilities::loadIcon(":/LayersPanel/wrench.png",
                                                           settingsIcon);
    
    m_settingsAction = WuQtUtilities::createAction("S",
                                                   "Edit settings for this chart",
                                                   this,
                                                   this,
                                                   SLOT(settingsActionTriggered()));
    if (settingsIconValid) {
        m_settingsAction->setIcon(settingsIcon);
    }
    m_settingsToolButton = new QToolButton();
    m_settingsToolButton->setDefaultAction(m_settingsAction);
    
    /*
     * ColorBar Tool Button
     */
    QIcon colorBarIcon;
    const bool colorBarIconValid = WuQtUtilities::loadIcon(":/LayersPanel/colorbar.png",
                                                           colorBarIcon);
    m_colorBarAction = WuQtUtilities::createAction("CB",
                                                       "Display color bar for this overlay",
                                                       this,
                                                       this,
                                                       SLOT(colorBarActionTriggered(bool)));
    m_colorBarAction->setCheckable(true);
    if (colorBarIconValid) {
        m_colorBarAction->setIcon(colorBarIcon);
    }
    m_colorBarToolButton = new QToolButton();
    m_colorBarToolButton->setDefaultAction(m_colorBarAction);
    
    /*
     * Construction Tool Button
     */
    QIcon constructionIcon;
    const bool constructionIconValid = WuQtUtilities::loadIcon(":/LayersPanel/construction.png",
                                                               constructionIcon);
    m_constructionAction = WuQtUtilities::createAction("C",
                                                           "Add/Move/Remove Layers",
                                                           this);
    if (constructionIconValid) {
        m_constructionAction->setIcon(constructionIcon);
    }
    m_constructionToolButton = new QToolButton();
    QMenu* constructionMenu = createConstructionMenu(m_constructionToolButton);
    m_constructionAction->setMenu(constructionMenu);
    m_constructionToolButton->setDefaultAction(m_constructionAction);
    m_constructionToolButton->setPopupMode(QToolButton::InstantPopup);
    
    /*
     * History button
     */
    m_historyToolButton = new QToolButton();
    m_historyAction = WuQtUtilities::createAction("H",
                                                  "Show history for line chart file",
                                                  this,
                                                  this,
                                                  SLOT(historyActionTriggered(bool)));
    QPixmap historyPixmap = createHistoryPixmap(m_historyToolButton);
    m_historyAction->setIcon(historyPixmap);
    m_historyToolButton->setDefaultAction(m_historyAction);
    //m_historyToolButton->setIconSize(historyPixmap.size());
    
    /*
     * Matrix triangular view mode button
     */
    m_matrixTriangularViewModeAction = WuQtUtilities::createAction("M",
                                                       "Matrix triangular view",
                                                       this);
    m_matrixTriangularViewModeToolButton = new QToolButton();
    QMenu* matrixTriangularViewModeMenu = createMatrixTriangularViewModeMenu(m_matrixTriangularViewModeToolButton);
    m_matrixTriangularViewModeAction->setMenu(matrixTriangularViewModeMenu);
    m_matrixTriangularViewModeToolButton->setDefaultAction(m_matrixTriangularViewModeAction);
    m_matrixTriangularViewModeToolButton->setPopupMode(QToolButton::InstantPopup);
    

    /*
     * Axis location button
     */
    m_axisLocationAction = WuQtUtilities::createAction("A",
                                                       "Vertical Axis Location",
                                                       this);
    m_axisLocationToolButton = new QToolButton();
    QMenu* axisLocationMenu = createAxisLocationMenu(m_axisLocationToolButton);
    m_axisLocationAction->setMenu(axisLocationMenu);
    m_axisLocationToolButton->setDefaultAction(m_axisLocationAction);
    m_axisLocationToolButton->setPopupMode(QToolButton::InstantPopup);
    
    /*
     * Map file Selection Check Box
     */
    m_mapFileComboBox = WuQFactory::newComboBox();
    m_mapFileComboBox->setMinimumWidth(minComboBoxWidth);
    m_mapFileComboBox->setMaximumWidth(maxComboBoxWidth);
    QObject::connect(m_mapFileComboBox, SIGNAL(activated(int)),
                     this, SLOT(fileComboBoxSelected(int)));
    m_mapFileComboBox->setToolTip("Selects file for this overlay");
    m_mapFileComboBox->setSizeAdjustPolicy(comboSizePolicy);
    
    /*
     * Yoking Group
     */
    const AString yokeToolTip = ("Select a yoking group.\n"
                                 "\n"
                                 "When files with more than one map are yoked,\n"
                                 "the seleted maps are synchronized by map index.\n"
                                 "\n"
                                 "If the SAME FILE is in yoked in multiple overlays,\n"
                                 "the overlay enabled statuses are synchronized.\n");
    m_mapRowOrColumnYokingGroupComboBox = new MapYokingGroupComboBox(this);
    m_mapRowOrColumnYokingGroupComboBox->getWidget()->setStatusTip("Synchronize enabled status and map indices)");
    m_mapRowOrColumnYokingGroupComboBox->getWidget()->setToolTip("Yoke to Overlay Mapped Files");
#ifdef CARET_OS_MACOSX
    m_mapRowOrColumnYokingGroupComboBox->getWidget()->setFixedWidth(m_mapRowOrColumnYokingGroupComboBox->getWidget()->sizeHint().width() - 20);
#endif // CARET_OS_MACOSX
    QObject::connect(m_mapRowOrColumnYokingGroupComboBox, SIGNAL(itemActivated()),
                     this, SLOT(yokingGroupActivated()));
    
    /*
     * All maps check box
     */
    m_allMapsCheckBox = new QCheckBox("All Maps");
    m_allMapsCheckBox->setToolTip("Show histogram of all maps");
    if (orientation == Qt::Horizontal) {
        m_allMapsCheckBox->setText(" ");
    }
    QObject::connect(m_allMapsCheckBox, SIGNAL(clicked(bool)),
                     this, SLOT(allMapsCheckBoxClicked(bool)));
    
    /*
     * Map/Row/Column Index Spin Box
     */
    m_mapRowOrColumnIndexSpinBox = WuQFactory::newSpinBox();
    QObject::connect(m_mapRowOrColumnIndexSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(mapRowOrColumnIndexSpinBoxValueChanged(int)));
    m_mapRowOrColumnIndexSpinBox->setToolTip("Select map/row/column by its index");
    m_mapRowOrColumnIndexSpinBox->setRange(0, 9999); // fix size for 4 digits
    m_mapRowOrColumnIndexSpinBox->setFixedSize(m_mapRowOrColumnIndexSpinBox->sizeHint());
//    m_mapRowOrColumnIndexSpinBox->setSizePolicy(QSizePolicy::Fixed,
//                                                QSizePolicy::Fixed);
    m_mapRowOrColumnIndexSpinBox->setRange(0, 0);
    
    /*
     * Map/Row/Column Name Combo Box
     */
    m_mapRowOrColumnNameComboBox = WuQFactory::newComboBox();
    m_mapRowOrColumnNameComboBox->setMinimumWidth(minComboBoxWidth);
    m_mapRowOrColumnNameComboBox->setMaximumWidth(maxComboBoxWidth);
    QObject::connect(m_mapRowOrColumnNameComboBox, SIGNAL(activated(int)),
                     this, SLOT(mapRowOrColumnNameComboBoxSelected(int)));
    m_mapRowOrColumnNameComboBox->setToolTip("Select map/row/column by its name");
    m_mapRowOrColumnNameComboBox->setSizeAdjustPolicy(comboSizePolicy);
}

/**
 * Destructor.
 */
ChartTwoOverlayViewController::~ChartTwoOverlayViewController()
{
}

/*
 * If this overlay ins an overlay settings editor, update its content
 */
void
ChartTwoOverlayViewController::updateOverlaySettingsEditor()
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    CaretMappableDataFile* mapFile = NULL;
    int32_t mapIndex = -1;
    m_chartOverlay->getSelectionData(mapFile,
                              mapIndex);
    
    if ((mapFile != NULL)
        && (mapIndex >= 0)) {
        EventOverlaySettingsEditorDialogRequest pcme(EventOverlaySettingsEditorDialogRequest::MODE_OVERLAY_MAP_CHANGED,
                                                     m_browserWindowIndex,
                                                     m_chartOverlay,
                                                     mapFile,
                                                     mapIndex);
        EventManager::get()->sendEvent(pcme.getPointer());
    }
}

/**
 * Called when a selection is made from the file combo box.
 * @parm indx
 *    Index of selection.
 */
void
ChartTwoOverlayViewController::fileComboBoxSelected(int indx)
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    void* pointer = m_mapFileComboBox->itemData(indx).value<void*>();
    CaretMappableDataFile* file = (CaretMappableDataFile*)pointer;
    m_chartOverlay->setSelectionData(file, -1);
    
    // DO NOT DO HERE validateYokingSelection();
    
    //validateYokingSelection(overlay->getYokingGroup());
    // not needed with call to validateYokingSelection: this->updateViewController(this->overlay);
    
    // called inside validateYokingSelection();  this->updateUserInterfaceAndGraphicsWindow();
    
    updateViewController(m_chartOverlay);
    m_mapRowOrColumnYokingGroupComboBox->validateYokingChange(m_chartOverlay);
    updateUserInterfaceAndGraphicsWindow();
    updateOverlaySettingsEditor();
    
//    if (file != NULL) {
//        if (file->isVolumeMappable()) {
//            /*
//             * Need to update slice indices/coords in toolbar.
//             */
//            EventManager::get()->sendEvent(EventUserInterfaceUpdate().setWindowIndex(m_browserWindowIndex).addToolBar().getPointer());
//        }
//    }
}

/**
 * Called when a selection is made from the map index spin box.
 * @parm indxIn
 *    Index of selection.
 */
void
ChartTwoOverlayViewController::mapRowOrColumnIndexSpinBoxValueChanged(int indxIn)
{
    if (m_chartOverlay == NULL)
    {
        //TSC: not sure how to put the displayed integer back to 0 where it starts when opening without data files
        return;
    }
    /*
     * Get the file that is selected from the file combo box
     */
    const int32_t fileIndex = m_mapFileComboBox->currentIndex();
    void* pointer = m_mapFileComboBox->itemData(fileIndex).value<void*>();
    CaretMappableDataFile* file = (CaretMappableDataFile*)pointer;
    
    /*
     * Spin box may range [0, N-1] or [1, N] but in source code
     * indices are always [0, N-1]
     */
    const int32_t indx = indxIn - m_mapRowOrColumnIndexSpinBox->minimum();
    m_chartOverlay->setSelectionData(file, indx);
    
    const MapYokingGroupEnum::Enum mapYoking = m_chartOverlay->getMapYokingGroup();
    if (mapYoking != MapYokingGroupEnum::MAP_YOKING_GROUP_OFF) {
        EventMapYokingSelectMap selectMapEvent(mapYoking,
                                               file,
                                               indx,
                                               m_chartOverlay->isEnabled());
        EventManager::get()->sendEvent(selectMapEvent.getPointer());
    }
    
    /*
     * Need to update map name combo box.
     */
    m_mapRowOrColumnNameComboBox->blockSignals(true);
    if ((indx >= 0)
        && (indx < m_mapRowOrColumnNameComboBox->count())) {
        m_mapRowOrColumnNameComboBox->setCurrentIndex(indx);
    }
    m_mapRowOrColumnNameComboBox->blockSignals(false);
    
    this->updateUserInterfaceAndGraphicsWindow();
    
    updateOverlaySettingsEditor();
}

/**
 * Called when a selection is made from the map name combo box.
 * @parm indx
 *    Index of selection.
 */
void
ChartTwoOverlayViewController::mapRowOrColumnNameComboBoxSelected(int indx)
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    /*
     * Get the file that is selected from the file combo box
     */
    const int32_t fileIndex = m_mapFileComboBox->currentIndex();
    void* pointer = m_mapFileComboBox->itemData(fileIndex).value<void*>();
    CaretMappableDataFile* file = (CaretMappableDataFile*)pointer;
    
    m_chartOverlay->setSelectionData(file, indx);
    
    const MapYokingGroupEnum::Enum mapYoking = m_chartOverlay->getMapYokingGroup();
    if (mapYoking != MapYokingGroupEnum::MAP_YOKING_GROUP_OFF) {
        EventMapYokingSelectMap selectMapEvent(mapYoking,
                                               file,
                                               indx,
                                               m_chartOverlay->isEnabled());
        EventManager::get()->sendEvent(selectMapEvent.getPointer());
    }
    
    /*
     * Need to update map index spin box.
     * Spin box may range [0, N-1] or [1, N] but in source code
     * indices are always [0, N-1]
     */
    const int spinBoxIndex = indx + m_mapRowOrColumnIndexSpinBox->minimum();
    m_mapRowOrColumnIndexSpinBox->blockSignals(true);
    m_mapRowOrColumnIndexSpinBox->setValue(spinBoxIndex);
    m_mapRowOrColumnIndexSpinBox->blockSignals(false);
    
    this->updateUserInterfaceAndGraphicsWindow();
    
    updateOverlaySettingsEditor();
}

/**
 * Called when enabled checkbox state is changed
 * @parm checked
 *    Checked status
 */
void
ChartTwoOverlayViewController::enabledCheckBoxClicked(bool checked)
{
    if (m_chartOverlay == NULL) {
        return;
    }
    m_chartOverlay->setEnabled(checked);
    
    const MapYokingGroupEnum::Enum mapYoking = m_chartOverlay->getMapYokingGroup();
    if (mapYoking != MapYokingGroupEnum::MAP_YOKING_GROUP_OFF) {
        CaretMappableDataFile* myFile = NULL;
        int32_t myIndex = -1;
        m_chartOverlay->getSelectionData(myFile,
                                        myIndex);
        
        EventMapYokingSelectMap selectMapEvent(mapYoking,
                                               myFile,
                                               myIndex,
                                               m_chartOverlay->isEnabled());
        EventManager::get()->sendEvent(selectMapEvent.getPointer());
    }
    
    this->updateUserInterfaceAndGraphicsWindow();
}

/**
 * Called when colorbar toolbutton is toggled.
 * @param status
 *    New status.
 */
void
ChartTwoOverlayViewController::colorBarActionTriggered(bool status)
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    m_chartOverlay->getColorBar()->setDisplayed(status);
    
    this->updateGraphicsWindow();
}

/**
 * Called when colorbar toolbutton is toggled.
 * @param status
 *    New status.
 */
void
ChartTwoOverlayViewController::allMapsCheckBoxClicked(bool status)
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    m_chartOverlay->setAllMapsSelected(status);
    
    this->updateGraphicsWindow();
}

/**
 * Validate yoking when there are changes made to the overlay.
 */
void
ChartTwoOverlayViewController::validateYokingSelection()
{
    m_mapRowOrColumnYokingGroupComboBox->validateYokingChange(m_chartOverlay);
    updateViewController(m_chartOverlay);
    updateUserInterfaceAndGraphicsWindow();
    //EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}

/**
 * Called when the yoking group is changed.
 */
void
ChartTwoOverlayViewController::yokingGroupActivated()
{
    MapYokingGroupEnum::Enum yokingGroup = m_mapRowOrColumnYokingGroupComboBox->getMapYokingGroup();
    
    /*
     * Has yoking group changed?
     * TSC: overlay can be null when opened without loaded files
     */
    if (m_chartOverlay != NULL && yokingGroup != m_chartOverlay->getMapYokingGroup()) {
        validateYokingSelection();
    }
}


/**
 * Called when the settings action is selected.
 */
void
ChartTwoOverlayViewController::settingsActionTriggered()
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    CaretMappableDataFile* mapFile;
    int32_t mapIndex = -1;
    m_chartOverlay->getSelectionData(mapFile,
                                    mapIndex);
    if (mapFile != NULL) {
        EventOverlaySettingsEditorDialogRequest pcme(EventOverlaySettingsEditorDialogRequest::MODE_SHOW_EDITOR,
                                                     m_browserWindowIndex,
                                                     m_chartOverlay,
                                                     mapFile,
                                                     mapIndex);
        EventManager::get()->sendEvent(pcme.getPointer());
    }
}

/**
 * Called when the history action is selected.
 */
void
ChartTwoOverlayViewController::historyActionTriggered(bool)
{
    if (m_chartOverlay == NULL) {
        return;
    }
    
    CaretMappableDataFile* mapFile;
    int32_t mapIndex = -1;
    m_chartOverlay->getSelectionData(mapFile,
                                     mapIndex);
    
    std::cout << "History button clicked.  Enabled="
    << qPrintable(AString::fromBool(m_chartOverlay->isHistorySupported())) << std::endl;
    
    CaretAssertToDoWarning();
//    if (mapFile != NULL) {
//        EventOverlaySettingsEditorDialogRequest pcme(EventOverlaySettingsEditorDialogRequest::MODE_SHOW_EDITOR,
//                                                     m_browserWindowIndex,
//                                                     m_chartOverlay,
//                                                     mapFile,
//                                                     mapIndex);
//        EventManager::get()->sendEvent(pcme.getPointer());
//    }
}


/**
 * Update this view controller using the given overlay.
 * @param overlay
 *   Overlay that is used in this view controller.
 */
void
ChartTwoOverlayViewController::updateViewController(ChartTwoOverlay* chartOverlay)
{
    m_chartOverlay = chartOverlay;
    
    
    /*
     * Get the selection information for the overlay.
     */
    std::vector<CaretMappableDataFile*> dataFiles;
    CaretMappableDataFile* selectedFile = NULL;
    std::vector<AString> selectedFileMapNames;
    int32_t selectedMapIndex = -1;
    if (m_chartOverlay != NULL) {
        m_chartOverlay->getSelectionData(dataFiles,
                                         selectedFile,
                                         selectedFileMapNames,
                                         selectedMapIndex);
    }
    
    /*
     * Setup names of file for display in combo box
     */
    std::vector<AString> displayNames;
    FilePathNamePrefixCompactor::removeMatchingPathPrefixFromCaretDataFiles(dataFiles,
                                                                            displayNames);
    CaretAssert(dataFiles.size() == displayNames.size());
    
    /*
     * Update tooltips with full path to file and name of map
     * as names may be too long to fit into combo boxes
     */
    AString fileComboBoxToolTip("Select file for this overlay");
    AString nameComboBoxToolTip("Select map by its name");
    if (selectedFile != NULL) {
        FileInformation fileInfo(selectedFile->getFileName());
        fileComboBoxToolTip.append(":\n"
                                   + fileInfo.getFileName()
                                   + "\n"
                                   + fileInfo.getPathName()
                                   + "\n\n"
                                   + "Copy File Name/Path to Clipboard with Construction Menu");
        
        nameComboBoxToolTip.append(":\n"
                                   + m_mapRowOrColumnNameComboBox->currentText());
    }
    m_mapFileComboBox->setToolTip(fileComboBoxToolTip);
    m_mapRowOrColumnNameComboBox->setToolTip(nameComboBoxToolTip);
    
    /*
     * Load the file selection combo box.
     */
    m_mapFileComboBox->clear();
    int32_t selectedFileIndex = -1;
    const int32_t numFiles = static_cast<int32_t>(dataFiles.size());
    for (int32_t i = 0; i < numFiles; i++) {
        CaretMappableDataFile* dataFile = dataFiles[i];
        
        AString dataTypeName = DataFileTypeEnum::toOverlayTypeName(dataFile->getDataFileType());
        CaretAssertVectorIndex(displayNames, i);
        m_mapFileComboBox->addItem(displayNames[i],
                                    qVariantFromValue((void*)dataFile));
        if (dataFile == selectedFile) {
            selectedFileIndex = i;
        }
    }
    if (selectedFileIndex >= 0) {
        m_mapFileComboBox->setCurrentIndex(selectedFileIndex);
    }
    
    /*
     * Load the map name selection combo box and map index spin box
     */
    m_mapRowOrColumnNameComboBox->setEnabled(false);
    m_mapRowOrColumnNameComboBox->blockSignals(true);
    m_mapRowOrColumnNameComboBox->clear();
    m_mapRowOrColumnIndexSpinBox->setEnabled(false);
    m_mapRowOrColumnIndexSpinBox->blockSignals(true);
    m_mapRowOrColumnIndexSpinBox->setRange(0, 0);
    m_mapRowOrColumnIndexSpinBox->setValue(0);
    const int32_t numberOfMaps = static_cast<int32_t>(selectedFileMapNames.size());
    if (numberOfMaps > 0) {
        m_mapRowOrColumnNameComboBox->setEnabled(true);
        m_mapRowOrColumnIndexSpinBox->setEnabled(true);

        for (int32_t i = 0; i < numberOfMaps; i++) {
            CaretAssertVectorIndex(selectedFileMapNames, i);
            m_mapRowOrColumnNameComboBox->addItem(selectedFileMapNames[i]);
        }
        m_mapRowOrColumnNameComboBox->setCurrentIndex(selectedMapIndex);
        
        /*
         * Spin box ranges [0, N-1] or [1, N] depending upon data
         */
        int32_t mapIndexMinimumValue = 1;
        int32_t mapIndexMaximumValue = numberOfMaps;
        switch (m_chartOverlay->getChartTwoDataType()) {
            case ChartTwoDataTypeEnum::CHART_DATA_TYPE_INVALID:
                break;
            case ChartTwoDataTypeEnum::CHART_DATA_TYPE_HISTOGRAM:
                break;
            case ChartTwoDataTypeEnum::CHART_DATA_TYPE_LINE_SERIES:
                break;
            case ChartTwoDataTypeEnum::CHART_DATA_TYPE_MATRIX:
                mapIndexMinimumValue = 0;
                mapIndexMaximumValue = numberOfMaps - 1;
                break;
        }
        
        m_mapRowOrColumnIndexSpinBox->setRange(mapIndexMinimumValue, mapIndexMaximumValue);
        const int spinBoxIndex = selectedMapIndex + m_mapRowOrColumnIndexSpinBox->minimum();
        m_mapRowOrColumnIndexSpinBox->setValue(spinBoxIndex);
    }
    m_mapRowOrColumnNameComboBox->blockSignals(false);
    m_mapRowOrColumnIndexSpinBox->blockSignals(false);
    
    const bool validOverlayAndFileFlag = ((m_chartOverlay != NULL)
                                          && (selectedFile != NULL));
    
    /*
     * Update enabled checkbox
     */
    m_enabledCheckBox->setEnabled(false);
    m_enabledCheckBox->setChecked(false);
    if (validOverlayAndFileFlag) {
        /*
         * First overlay is ALWAYS ENABLED and HIDDEN
         */
        if (m_chartOverlayIndex == 0) {
            m_enabledCheckBox->setVisible(false);
        }
        else {
            m_enabledCheckBox->setEnabled(true);
            m_enabledCheckBox->setChecked(m_chartOverlay->isEnabled());
        }
    }
    
    
    /*
     * Update yoking
     */
    m_mapRowOrColumnYokingGroupComboBox->getWidget()->setEnabled(false);
    m_mapRowOrColumnYokingGroupComboBox->setMapYokingGroup(MapYokingGroupEnum::MAP_YOKING_GROUP_OFF);
    if (validOverlayAndFileFlag) {
        m_mapRowOrColumnYokingGroupComboBox->getWidget()->setEnabled(true);
        if (m_chartOverlay->isMapYokingSupported()) {
            m_mapRowOrColumnYokingGroupComboBox->setMapYokingGroup(m_chartOverlay->getMapYokingGroup());
        }
    }
    
    /*
     * Update all maps
     */
    m_allMapsCheckBox->setEnabled(false);
    m_allMapsCheckBox->setChecked(false);
    if (validOverlayAndFileFlag) {
        m_allMapsCheckBox->setEnabled(m_chartOverlay->isAllMapsSupported());
        if (m_chartOverlay->isAllMapsSupported()) {
            m_allMapsCheckBox->setChecked(m_chartOverlay->isAllMapsSelected());
        }
    }
    
    /*
     * Update settings (wrench) button
     */
    m_settingsAction->setEnabled(validOverlayAndFileFlag);
    
    /*
     * Update color bar button
     */
    m_colorBarAction->blockSignals(true);
    m_colorBarAction->setEnabled(false);
    m_colorBarAction->setChecked(false);
    if (validOverlayAndFileFlag) {
        if (selectedFile->isMappedWithPalette()) {
            m_colorBarAction->setEnabled(true);
            m_colorBarAction->setChecked(m_chartOverlay->getColorBar()->isDisplayed());
        }
    }
    m_colorBarAction->blockSignals(false);
    
    /*
     * Update construction button
     */
    m_constructionAction->setEnabled(true);
    
    /*
     * Update history button
     */
    m_historyAction->setEnabled(false);
    if (validOverlayAndFileFlag) {
        m_historyAction->setEnabled(m_chartOverlay->isHistorySupported());
    }
    
    /*
     * Update matrix triangular view mode
     */
    m_matrixTriangularViewModeAction->setEnabled(false);
    if (validOverlayAndFileFlag) {
        const ChartTwoMatrixTriangularViewingModeEnum::Enum viewMode = m_chartOverlay->getMatrixTriangularViewingMode();
        
        for (auto& mvmd : m_matrixViewMenuData) {
            if (std::get<0>(mvmd) == viewMode) {
                std::get<1>(mvmd)->setChecked(true);
                updateMatrixTriangularViewModeAction(viewMode);
                break;
            }
        }
        
        if (m_chartOverlay->isMatrixTriangularViewingModeSupported()) {
            m_matrixTriangularViewModeAction->setEnabled(true);
        }
    }
    
    /*
     * Update vertical axis location
     */
    m_axisLocationAction->setEnabled(false);
    if (validOverlayAndFileFlag) {
        if (m_chartOverlay->isCartesianVerticalAxisLocationSupported()) {
            m_axisLocationAction->setEnabled(true);
            const ChartAxisLocationEnum::Enum axisLocation = m_chartOverlay->getCartesianVerticalAxisLocation();
            for (auto& almd : m_axisLocationMenuData) {
                if (std::get<0>(almd) == axisLocation) {
                    updateAxisLocationAction(axisLocation);
                    break;
                }
            }
        }
    }
}

/**
 * Update the matrix triangular view mode button.
 *
 * @param matrixViewMode
 *     Matrix triangular view mode.
 */
void
ChartTwoOverlayViewController::updateMatrixTriangularViewModeAction(const ChartTwoMatrixTriangularViewingModeEnum::Enum matrixViewMode)
{
    CaretAssert(m_matrixTriangularViewModeAction);
    m_matrixTriangularViewModeAction->blockSignals(true);
    for (auto& mvmd : m_matrixViewMenuData) {
        if (std::get<0>(mvmd) == matrixViewMode) {
            QPixmap pixmap = std::get<2>(mvmd);
            if ( ! pixmap.isNull()) {
                m_matrixTriangularViewModeAction->setIcon(pixmap);
                m_matrixTriangularViewModeAction->setText("");
            }
            else {
                m_matrixTriangularViewModeAction->setText("M");
            }
            break;
        }
    }
    m_matrixTriangularViewModeAction->blockSignals(false);
}

/**
 * Update the axis location button.
 *
 * @param axisLocation
 *     Axis location.
 */
void
ChartTwoOverlayViewController::updateAxisLocationAction(const ChartAxisLocationEnum::Enum axisLocation)
{
    CaretAssert(m_axisLocationAction);
    m_axisLocationAction->blockSignals(true);
    for (auto& almd : m_axisLocationMenuData) {
        if (std::get<0>(almd) == axisLocation) {
            QPixmap pixmap = std::get<2>(almd);
            if ( ! pixmap.isNull()) {
                m_axisLocationAction->setIcon(pixmap);
                m_axisLocationAction->setText("");
            }
            else {
                m_axisLocationAction->setText("A");
            }
            break;
        }
    }
    m_axisLocationAction->blockSignals(false);
}


/**
 * Update graphics and GUI after selections made
 */
void
ChartTwoOverlayViewController::updateUserInterfaceAndGraphicsWindow()
{
    updateUserInterface();
    updateGraphicsWindow();
}

/**
 * Update graphics and GUI after selections made
 */
void
ChartTwoOverlayViewController::updateUserInterface()
{
    EventManager::get()->sendEvent(EventUserInterfaceUpdate().getPointer());
}

/**
 * Update graphics after selections made
 */
void
ChartTwoOverlayViewController::updateGraphicsWindow()
{
    EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}

/**
 * Create the matrix triangular view mode menu.
 * @param parent
 *    Parent widget.
 */
QMenu*
ChartTwoOverlayViewController::createMatrixTriangularViewModeMenu(QWidget* parent)
{
    std::vector<ChartTwoMatrixTriangularViewingModeEnum::Enum> allViewModes;
    ChartTwoMatrixTriangularViewingModeEnum::getAllEnums(allViewModes);
    
    QMenu* menu = new QMenu(parent);
    QObject::connect(menu, &QMenu::triggered,
                     this, &ChartTwoOverlayViewController::menuMatrixTriangularViewModeTriggered);

    QActionGroup* actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);
    
    for (auto viewMode: allViewModes) {
        QAction* action = menu->addAction(ChartTwoMatrixTriangularViewingModeEnum::toGuiName(viewMode));
        action->setCheckable(true);
        action->setData((int)ChartTwoMatrixTriangularViewingModeEnum::toIntegerCode(viewMode));
        QPixmap pixmap = createMatrixTriangularViewModePixmap(menu, viewMode);
        action->setIcon(pixmap);
        actionGroup->addAction(action);
        
        m_matrixViewMenuData.push_back(std::make_tuple(viewMode, action, pixmap));
    }
    
    return menu;
}

/**
 * Called when an item is selected on matrix triangular view mode menu.
 *
 * @action
 *     Action of menu item selected.
 */
void
ChartTwoOverlayViewController::menuMatrixTriangularViewModeTriggered(QAction* action)
{
    const QVariant itemData = action->data();
    CaretAssert(itemData.isValid());
    bool valid = false;
    ChartTwoMatrixTriangularViewingModeEnum::Enum viewMode = ChartTwoMatrixTriangularViewingModeEnum::fromIntegerCode(itemData.toInt(), &valid);
    
    if (valid) {
        m_chartOverlay->setMatrixTriangularViewingMode(viewMode);
        updateMatrixTriangularViewModeAction(viewMode);
        this->updateGraphicsWindow();
    }
}

/**
 * Create the axis location menu.
 * @param parent
 *    Parent widget.
 */
QMenu*
ChartTwoOverlayViewController::createAxisLocationMenu(QWidget* widget)
{
    std::vector<ChartAxisLocationEnum::Enum> axisLocations;
    axisLocations.push_back(ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT);
    axisLocations.push_back(ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT);
    
    QMenu* menu = new QMenu(widget);
    QObject::connect(menu, &QMenu::triggered,
                     this, &ChartTwoOverlayViewController::menuAxisLocationTriggered);
    
    QActionGroup* actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);
    
    for (auto axis: axisLocations) {
        QAction* action = menu->addAction(ChartAxisLocationEnum::toGuiName(axis));
        action->setCheckable(true);
        action->setData((int)ChartAxisLocationEnum::toIntegerCode(axis));
        QPixmap pixmap = createAxisLocationPixmap(menu, axis);
        action->setIcon(pixmap);
        actionGroup->addAction(action);
        
        m_axisLocationMenuData.push_back(std::make_tuple(axis, action, pixmap));
    }
    
    return menu;
}

/**
 * Called when an item is selected on axis location menu.
 *
 * @action
 *     Action of menu item selected.
 */
void
ChartTwoOverlayViewController::menuAxisLocationTriggered(QAction* action)
{
    const QVariant itemData = action->data();
    CaretAssert(itemData.isValid());
    bool valid = false;
    ChartAxisLocationEnum::Enum axisLocation = ChartAxisLocationEnum::fromIntegerCode(itemData.toInt(), &valid);
    
    if (valid) {
        m_chartOverlay->setCartesianVerticalAxisLocation(axisLocation);
        updateAxisLocationAction(axisLocation);
        this->updateGraphicsWindow();
    }
}

/**
 * Create the construction menu.
 * @param parent
 *    Parent widget.
 */
QMenu*
ChartTwoOverlayViewController::createConstructionMenu(QWidget* parent)
{
    QMenu* menu = new QMenu(parent);
    QObject::connect(menu, SIGNAL(aboutToShow()),
                     this, SLOT(menuConstructionAboutToShow()));
    
    menu->addAction("Add Overlay Above",
                    this,
                    SLOT(menuAddOverlayAboveTriggered()));
    
    menu->addAction("Add Overlay Below",
                    this,
                    SLOT(menuAddOverlayBelowTriggered()));
    
    menu->addSeparator();
    
    menu->addAction("Move Overlay Up",
                    this,
                    SLOT(menuMoveOverlayUpTriggered()));
    
    menu->addAction("Move Overlay Down",
                    this,
                    SLOT(menuMoveOverlayDownTriggered()));
    
    menu->addSeparator();
    
    menu->addAction("Remove This Overlay",
                    this,
                    SLOT(menuRemoveOverlayTriggered()));
    
    menu->addSeparator();
    
    m_constructionReloadFileAction = menu->addAction("Reload Selected File",
                                                     this,
                                                     SLOT(menuReloadFileTriggered()));
    
    menu->addSeparator();
    
    menu->addAction("Copy Path and File Name to Clipboard",
                    this,
                    SLOT(menuCopyFileNameToClipBoard()));
    
    menu->addAction("Copy Map Name to Clipboard",
                    this,
                    SLOT(menuCopyMapNameToClipBoard()));
    
    return menu;
    
}

/**
 * Called when construction menu is about to be displayed.
 */
void
ChartTwoOverlayViewController::menuConstructionAboutToShow()
{
    if (m_chartOverlay != NULL) {
        CaretMappableDataFile* caretDataFile = NULL;
        int32_t mapIndex = -1;
        m_chartOverlay->getSelectionData(caretDataFile,
                                        mapIndex);
        
        QString menuText = "Reload Selected File";
        if (caretDataFile != NULL) {
            if (caretDataFile->isModified()) {
                QString suffix = " (MODIFIED)";
                if (caretDataFile->isModifiedPaletteColorMapping()) {
                    if ( ! caretDataFile->isModifiedExcludingPaletteColorMapping()) {
                        suffix = " (MODIFIED PALETTE)";
                    }
                }
                menuText += suffix;
            }
        }
        m_constructionReloadFileAction->setText(menuText);
    }
}

/**
 * Add an overlay above this overlay.
 */
void
ChartTwoOverlayViewController::menuAddOverlayAboveTriggered()
{
    emit requestAddOverlayAbove(m_chartOverlayIndex);
}

/**
 * Add an overlay below this overlay.
 */
void
ChartTwoOverlayViewController::menuAddOverlayBelowTriggered()
{
    emit requestAddOverlayBelow(m_chartOverlayIndex);
}

/**
 * Remove this overlay.
 */
void
ChartTwoOverlayViewController::menuRemoveOverlayTriggered()
{
    emit requestRemoveOverlay(m_chartOverlayIndex);
}

/**
 * Move this overlay down.
 */
void
ChartTwoOverlayViewController::menuMoveOverlayDownTriggered()
{
    emit requestMoveOverlayDown(m_chartOverlayIndex);
}

/**
 * Move this overlay down.
 */
void
ChartTwoOverlayViewController::menuMoveOverlayUpTriggered()
{
    emit requestMoveOverlayUp(m_chartOverlayIndex);
}

/**
 * Copy the file name to the clip board.
 */
void
ChartTwoOverlayViewController::menuCopyFileNameToClipBoard()
{
    if (m_chartOverlay != NULL) {
        CaretMappableDataFile* caretDataFile = NULL;
        int32_t mapIndex = -1;
        m_chartOverlay->getSelectionData(caretDataFile,
                                        mapIndex);
        
        if (caretDataFile != NULL) {
            QApplication::clipboard()->setText(caretDataFile->getFileName().trimmed(),
                                               QClipboard::Clipboard);
        }
    }
}

/**
 * Copy the map name to the clip board.
 */
void
ChartTwoOverlayViewController::menuCopyMapNameToClipBoard()
{
    if (m_chartOverlay != NULL) {
        CaretMappableDataFile* caretDataFile = NULL;
        int32_t mapIndex = -1;
        m_chartOverlay->getSelectionData(caretDataFile,
                                        mapIndex);
        
        if (caretDataFile != NULL) {
            if ((mapIndex >= 0)
                && (mapIndex < caretDataFile->getNumberOfMaps())) {
                QApplication::clipboard()->setText(caretDataFile->getMapName(mapIndex).trimmed(),
                                                   QClipboard::Clipboard);
            }
        }
    }
}

/**
 * Reload the file in the overlay.
 */
void
ChartTwoOverlayViewController::menuReloadFileTriggered()
{
    if (m_chartOverlay != NULL) {
        CaretMappableDataFile* caretDataFile = NULL;
        int32_t mapIndex = -1;
        m_chartOverlay->getSelectionData(caretDataFile,
                                        mapIndex);
        
        if (caretDataFile != NULL) {
            AString username;
            AString password;
            
            if (DataFile::isFileOnNetwork(caretDataFile->getFileName())) {
                const QString msg("This file is on the network.  "
                                  "If accessing the file requires a username and "
                                  "password, enter it here.  Otherwise, remove any "
                                  "text from the username and password fields.");
                
                
                if (UsernamePasswordWidget::getUserNameAndPasswordInDialog(m_constructionToolButton,
                                                                           "Username and Password",
                                                                           msg,
                                                                           username,
                                                                           password)) {
                    /* nothing */
                }
                else {
                    return;
                }
            }
            
            EventDataFileReload reloadEvent(GuiManager::get()->getBrain(),
                                            caretDataFile);
            reloadEvent.setUsernameAndPassword(username,
                                               password);
            EventManager::get()->sendEvent(reloadEvent.getPointer());
            
            if (reloadEvent.isError()) {
                WuQMessageBox::errorOk(m_constructionToolButton,
                                       reloadEvent.getErrorMessage());
            }
            
            updateOverlaySettingsEditor();
            
            updateUserInterfaceAndGraphicsWindow();
        }
    }
}

/**
 * Create a axis location mode pixmap.
 *
 * @param widget
 *    To color the pixmap with backround and foreground,
 *    the palette from the given widget is used.
 * @param axisLocation
 *    Axis location represented by the icon.
 * @return
 *    Pixmap for matrix view mode.
 */
QPixmap
ChartTwoOverlayViewController::createAxisLocationPixmap(QWidget* widget,
                                                        const ChartAxisLocationEnum::Enum axisLocation)
{
    CaretAssert(widget);
    
    /*
     * Create a small, square pixmap that will contain
     * the foreground color around the pixmap's perimeter.
     */
    const qreal iconSize = 24.0;
    const qreal minValue = 2.0;
    const qreal maxValue = iconSize - minValue;
    
    QPixmap pixmap(static_cast<int>(iconSize),
                   static_cast<int>(iconSize));
    QSharedPointer<QPainter> painter = WuQtUtilities::createPixmapWidgetPainterOriginBottomLeft(widget,
                                                                                                pixmap);
    
    QPen pen = painter->pen();
    pen.setWidthF(2.0);
    painter->setPen(pen);
    
    const int offsetFromEdge = 3;
    switch (axisLocation) {
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_BOTTOM:
            painter->drawLine(QPointF(minValue, minValue + offsetFromEdge),
                              QPointF(maxValue, minValue + offsetFromEdge));
            break;
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_LEFT:
            painter->drawLine(QPointF(minValue + offsetFromEdge, minValue),
                              QPointF(minValue + offsetFromEdge, maxValue));
            break;
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_RIGHT:
            painter->drawLine(QPointF(maxValue - offsetFromEdge, minValue),
                              QPointF(maxValue - offsetFromEdge, maxValue));
            break;
        case ChartAxisLocationEnum::CHART_AXIS_LOCATION_TOP:
            painter->drawLine(QPointF(minValue, maxValue - offsetFromEdge),
                              QPointF(maxValue, maxValue - offsetFromEdge));
            break;
    }
    
    return pixmap;
}

/**
 * Create a matrix view mode pixmap.
 *
 * @param widget
 *    To color the pixmap with backround and foreground,
 *    the palette from the given widget is used.
 * @param matrixViewMode
 *    Matrix view mode represented by the icon.
 * @return
 *    Pixmap for matrix view mode.
 */
QPixmap
ChartTwoOverlayViewController::createMatrixTriangularViewModePixmap(QWidget* widget,
                                                                 const ChartTwoMatrixTriangularViewingModeEnum::Enum matrixViewMode)
{
    CaretAssert(widget);
    
    /*
     * Create a small, square pixmap that will contain
     * the foreground color around the pixmap's perimeter.
     */
    const qreal iconSize = 24.0;
    const qreal minValue = 2.0;
    const qreal maxValue = iconSize - minValue;
    const QPointF bottomLeft(minValue, minValue);
    const QPointF bottomRight(maxValue, minValue);
    const QPointF topRight(maxValue, maxValue);
    const QPointF topLeft(minValue, maxValue);
    
    QPixmap pixmap(static_cast<int>(iconSize),
                   static_cast<int>(iconSize));
    QSharedPointer<QPainter> painter = WuQtUtilities::createPixmapWidgetPainterOriginBottomLeft(widget,
                                                                                                pixmap);

    QPen pen = painter->pen();
    pen.setWidthF(2.0);
    painter->setPen(pen);

    QPolygonF polygon;
    switch (matrixViewMode) {
        case ChartTwoMatrixTriangularViewingModeEnum::MATRIX_VIEW_FULL:
        {
            polygon.push_back(bottomLeft);
            polygon.push_back(bottomRight);
            polygon.push_back(topRight);
            polygon.push_back(topLeft);
        }
            break;
        case ChartTwoMatrixTriangularViewingModeEnum::MATRIX_VIEW_FULL_NO_DIAGONAL:
        {
            polygon.push_back(bottomLeft);
            polygon.push_back(bottomRight);
            polygon.push_back(topRight);
            polygon.push_back(topLeft);
            
            painter->drawLine(topLeft, bottomRight);
        }
            break;
        case ChartTwoMatrixTriangularViewingModeEnum::MATRIX_VIEW_LOWER_NO_DIAGONAL:
        {
            polygon.push_back(bottomLeft);
            polygon.push_back(bottomRight);
            polygon.push_back(topLeft);
        }
            break;
        case ChartTwoMatrixTriangularViewingModeEnum::MATRIX_VIEW_UPPER_NO_DIAGONAL:
        {
            polygon.push_back(bottomRight);
            polygon.push_back(topRight);
            polygon.push_back(topLeft);
        }
            break;
    }
    

    painter->drawPolygon(polygon);
    
    return pixmap;
}


/**
 * Create a history pixmap.
 *
 * @param widget
 *    To color the pixmap with backround and foreground,
 *    the palette from the given widget is used.
 * @return
 *    Pixmap for history.
 */
QPixmap
ChartTwoOverlayViewController::createHistoryPixmap(QWidget* widget)
{
    CaretAssert(widget);
    
    /*
     * Create a small, square pixmap that will contain
     * the foreground color around the pixmap's perimeter.
     */
    const qreal iconSize = 24.0;
    const qreal centerX = iconSize / 2.0;
    const qreal centerY = iconSize / 2.0;
    
    QPixmap pixmap(static_cast<int>(iconSize),
                   static_cast<int>(iconSize));
    QSharedPointer<QPainter> painter = WuQtUtilities::createPixmapWidgetPainter(widget,
                                                                                pixmap);
    
    QPen pen = painter->pen();
    pen.setWidthF(2.0);
    painter->setPen(pen);
    
    const QPointF circleCenterXY(centerX, centerY);
    const qreal   circleRadius = iconSize * 0.45;
    
    painter->drawEllipse(circleCenterXY,
                         circleRadius, circleRadius);
    
    const qreal nineOClockX = circleRadius * 0.5; //0.25 * iconSize;
    const qreal twelveOClockY = 0.15 * iconSize;
    painter->drawLine(QPointF(centerX, centerY),
                      QPointF(nineOClockX, centerY));
    painter->drawLine(QPointF(centerX, centerY),
                      QPointF(centerY, twelveOClockY));
    
    return pixmap;
}



