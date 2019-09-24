#ifndef __BROWSER_TAB_CONTENT__H_
#define __BROWSER_TAB_CONTENT__H_

/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
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

#include <set>

#include "CaretObject.h"
#include "EventListenerInterface.h"
#include "Model.h"
#include "ModelTypeEnum.h"
#include "Plane.h"
#include "ProjectionViewTypeEnum.h"
#include "SceneableInterface.h"
#include "StructureEnum.h"
#include "TabContentBase.h"
#include "VolumeSliceDrawingTypeEnum.h"
#include "VolumeSliceInterpolationEdgeEffectsMaskingEnum.h"
#include "VolumeSliceProjectionTypeEnum.h"
#include "VolumeSliceViewPlaneEnum.h"
#include "VolumeSliceViewAllPlanesLayoutEnum.h"
#include "YokingGroupEnum.h"

namespace caret {

    class AnnotationBrowserTab;
    class AnnotationColorBar;
    class BrainOpenGLViewportContent;
    class CaretDataFile;
    class CaretMappableDataFile;
    class ChartTwoMatrixDisplayProperties;
    class ChartTwoOverlaySet;
    class ClippingPlaneGroup;
    class EventCaretMappableDataFilesAndMapsInDisplayedOverlays;
    class Matrix4x4;
    class ModelChart;
    class ModelChartTwo;
    class ModelSurface;
    class ModelSurfaceMontage;
    class ModelSurfaceSelector;
    class ModelTransform;
    class ModelVolume;
    class ModelWholeBrain;
    class OverlaySet;
    class Palette;
    class PlainTextStringBuilder;
    class SceneClassAssistant;
    class Surface;
    class ViewingTransformations;
    class ViewingTransformationsCerebellum;
    class ViewingTransformationsVolume;
    class VolumeMappableInterface;
    class VolumeSliceSettings;
    class VolumeSurfaceOutlineSetModel;
    class WholeBrainSurfaceSettings;
    
    /// Maintains content in a brower's tab
    class BrowserTabContent : public TabContentBase, public EventListenerInterface, public SceneableInterface {
        
    public:
        BrowserTabContent(const int32_t tabNumber);
        
        virtual ~BrowserTabContent();
        
        void cloneBrowserTabContent(BrowserTabContent* tabToClone);
        
        virtual void receiveEvent(Event* event);
        
        virtual void getDescriptionOfContent(PlainTextStringBuilder& descriptionOut) const;
        
        virtual AString toString() const;
        
        AString getTabName() const override;
        
        AString getUserTabName() const;
        
        void setUserTabName(const AString& userName);
        
        OverlaySet* getOverlaySet();
        
        const OverlaySet* getOverlaySet() const;
        
        ChartTwoOverlaySet* getChartTwoOverlaySet();
        
        const ChartTwoOverlaySet* getChartTwoOverlaySet() const;
        
        int32_t getTabNumber() const;
        
        ModelTypeEnum::Enum getSelectedModelType() const;
        
        void setSelectedModelType(ModelTypeEnum::Enum selectedModelType);
        
        const Model* getModelForDisplay() const;
        
        Model* getModelForDisplay();
        
        ModelChart* getDisplayedChartOneModel();
        
        const ModelChart* getDisplayedChartOneModel() const;
        
        ModelChartTwo* getDisplayedChartTwoModel();
        
        const ModelChartTwo* getDisplayedChartTwoModel() const;
        
        ModelSurface* getDisplayedSurfaceModel();
        
        const ModelSurface* getDisplayedSurfaceModel() const;
        
        ModelVolume* getDisplayedVolumeModel();
        
        const ModelVolume* getDisplayedVolumeModel() const;
        
        ModelWholeBrain* getDisplayedWholeBrainModel();
        
        ModelSurfaceMontage* getDisplayedSurfaceMontageModel();
        
        const ModelSurfaceMontage* getDisplayedSurfaceMontageModel() const;
        
        const std::vector<ModelSurface*> getAllSurfaceModels() const;
        
        ModelSurfaceSelector* getSurfaceModelSelector();
        
        std::vector<StructureEnum::Enum> getSurfaceStructuresDisplayed();
        
        bool isCerebellumDisplayed() const;
        
        bool isChartOneDisplayed() const;
        
        bool isChartTwoDisplayed() const;
        
        bool isFlatSurfaceDisplayed() const;
        
        bool isVolumeSlicesDisplayed() const;
        
        bool isWholeBrainDisplayed() const;
        
        void getFilesDisplayedInTab(std::vector<CaretDataFile*>& displayedDataFilesOut);
        
        void getFilesAndMapIndicesInOverlays(EventCaretMappableDataFilesAndMapsInDisplayedOverlays* fileAndMapsEvent);

        void update(const std::vector<Model*> models);
        
        bool isChartOneModelValid() const;
        
        bool isChartTwoModelValid() const;
        
        bool isSurfaceModelValid() const;
        
        bool isVolumeSliceModelValid() const;
        
        bool isWholeBrainModelValid() const;

        bool isSurfaceMontageModelValid() const;
        
        void getAnnotationColorBars(std::vector<AnnotationColorBar*>& colorBarsOut);
        
        void getDisplayedPaletteMapFiles(std::vector<CaretMappableDataFile*>& mapFiles,
                                         std::vector<int32_t>& mapIndices);
        
        VolumeSurfaceOutlineSetModel* getVolumeSurfaceOutlineSet();
        
        const VolumeSurfaceOutlineSetModel* getVolumeSurfaceOutlineSet() const;
        
        bool isAspectRatioLocked() const;
        
        void setAspectRatioLocked(const bool locked);
        
        float getAspectRatio() const;
        
        void setAspectRatio(const float aspectRatio);
        

        void getClippingPlaneEnabled(bool& xEnabled,
                                     bool& yEnabled,
                                     bool& zEnabled,
                                     bool& surfaceEnabled,
                                     bool& volumeEnabled,
                                     bool& featuresEnabled) const;
        
        void setClippingPlaneEnabled(const bool xEnabled,
                                     const bool yEnabled,
                                     const bool zEnabled,
                                     const bool surfaceEnabled,
                                     const bool volumeEnabled,
                                     const bool featuresEnabled);
        
        void getClippingPlaneTransformation(float panning[3],
                                            float rotation[3],
                                            float thickness[3],
                                            bool& displayClippingBox) const;
        
        void setClippingPlaneTransformation(const float panning[3],
                                            const float rotation[3],
                                            const float thickness[3],
                                            const bool displayClippingBox);

        const ClippingPlaneGroup* getClippingPlaneGroup() const;

        void resetClippingPlaneTransformation();
        
        const float* getTranslation() const;
        
        void getTranslation(float translationOut[3]) const;
        
        void setTranslation( const float translation[3]);
        
        void setTranslation(const float translationX,
                                    const float translationY,
                                    const float translationZ);

        float getScaling() const;
        
        void setScaling(const float scaling);
        
        Matrix4x4 getRotationMatrix() const;
        
        void setRotationMatrix(const Matrix4x4& rotationMatrix);
        
        Matrix4x4 getObliqueVolumeRotationMatrix() const;
        
        void setObliqueVolumeRotationMatrix(const Matrix4x4& obliqueRotationMatrix);

        void getRightCortexFlatMapOffset(float& offsetX,
                                         float& offsetY) const;
        
        void setRightCortexFlatMapOffset(const float offsetX,
                                         const float offsetY);
        
        float getRightCortexFlatMapZoomFactor() const;
        
        void setRightCortexFlatMapZoomFactor(const float zoomFactor);
        
        ProjectionViewTypeEnum::Enum getProjectionViewType() const;
        
        void resetView();
        
        void rightView();
        
        void leftView();
        
        void anteriorView();
        
        void posteriorView();
        
        void dorsalView();
        
        void ventralView();
        
        void applyMouseVolumeSliceIncrement(BrainOpenGLViewportContent* viewportContent,
                                            const int32_t mousePressX,
                                            const int32_t mousePressY,
                                            const int32_t mouseDY);
        
        void applyMouseRotation(BrainOpenGLViewportContent* viewportContent,
                                const int32_t mousePressX,
                                const int32_t mousePressY,
                                const int32_t mouseX,
                                const int32_t mouseY,
                                const int32_t mouseDeltaX,
                                const int32_t mouseDeltaY);
        
        void applyMouseScaling(const int32_t mouseDX,
                               const int32_t mouseDY);
        
        void applyMouseTranslation(BrainOpenGLViewportContent* viewportContent,
                                   const int32_t mousePressX,
                                   const int32_t mousePressY,
                                   const int32_t mouseDX,
                                   const int32_t mouseDY);
        
        void getTransformationsForOpenGLDrawing(const ProjectionViewTypeEnum::Enum projectionViewType,
                                                float translationOut[3],
                                                double rotationMatrixOut[16],
                                                float& scalingOut) const;
        
        void getTransformationsInModelTransform(ModelTransform& modelTransform) const;
        
        void setTransformationsFromModelTransform(const ModelTransform& modelTransform);
        
        ChartTwoMatrixDisplayProperties* getChartTwoMatrixDisplayProperties();

        const ChartTwoMatrixDisplayProperties* getChartTwoMatrixDisplayProperties() const;
        
        VolumeSliceViewPlaneEnum::Enum getSliceViewPlane() const;
        
        void setSliceViewPlane(VolumeSliceViewPlaneEnum::Enum sliceAxisMode);
        
        VolumeSliceViewAllPlanesLayoutEnum::Enum getSlicePlanesAllViewLayout() const;
        
        void setSlicePlanesAllViewLayout(const VolumeSliceViewAllPlanesLayoutEnum::Enum slicePlanesAllViewLayout);
        
        VolumeSliceDrawingTypeEnum::Enum getSliceDrawingType() const;
        
        void setSliceDrawingType(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType);
        
        VolumeSliceProjectionTypeEnum::Enum getSliceProjectionType() const;
        
        void setSliceProjectionType(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType);
        
        VolumeSliceInterpolationEdgeEffectsMaskingEnum::Enum getVolumeSliceInterpolationEdgeEffectsMaskingType() const;
        
        void setVolumeSliceInterpolationEdgeEffectsMaskingType(const VolumeSliceInterpolationEdgeEffectsMaskingEnum::Enum maskingType);
        
        int32_t getMontageNumberOfColumns() const;
        
        void setMontageNumberOfColumns(const int32_t montageNumberOfColumns);
        
        int32_t getMontageNumberOfRows() const;
        
        void setMontageNumberOfRows(const int32_t montageNumberOfRows);
        
        int32_t getMontageSliceSpacing() const;
        
        void setMontageSliceSpacing(const int32_t montageSliceSpacing);
        
        void setSlicesToOrigin();
        
        float getSliceCoordinateAxial() const;
        
        void setSliceCoordinateAxial(const float x);
        
        float getSliceCoordinateCoronal() const;
        
        void setSliceCoordinateCoronal(const float y);
        
        float getSliceCoordinateParasagittal() const;
        
        void setSliceCoordinateParasagittal(const float z);
        
        int64_t getSliceIndexAxial(const VolumeMappableInterface* volumeFile) const;
        
        void setSliceIndexAxial(const VolumeMappableInterface* volumeFile,
                                const int64_t sliceIndexAxial);
        
        int64_t getSliceIndexCoronal(const VolumeMappableInterface* volumeFile) const;
        
        void setSliceIndexCoronal(const VolumeMappableInterface* volumeFile,
                                  const int64_t sliceIndexCoronal);
        
        int64_t getSliceIndexParasagittal(const VolumeMappableInterface* volumeFile) const;
        
        void setSliceIndexParasagittal(const VolumeMappableInterface* volumeFile,
                                       const int64_t sliceIndexParasagittal);
        
        bool isSliceParasagittalEnabled() const;
        
        void setSliceParasagittalEnabled(const bool sliceEnabledParasagittal);
        
        bool isSliceCoronalEnabled() const;
        
        void setSliceCoronalEnabled(const bool sliceEnabledCoronal);
        
        bool isSliceAxialEnabled() const;
        
        void setSliceAxialEnabled(const bool sliceEnabledAxial);
        
        void updateForVolumeFile(const VolumeMappableInterface* volumeFile);
        
        void selectSlicesAtOrigin();
        
        void selectSlicesAtCoordinate(const float xyz[3]);
        
        bool isIdentificationUpdatesVolumeSlices() const;
        
        void setIdentificationUpdatesVolumeSlices(const bool status);

        bool isVolumeAxesCrosshairsDisplayed() const;
        
        void setVolumeAxesCrosshairsDisplayed(const bool displayed);
        
        bool isVolumeAxesCrosshairLabelsDisplayed() const;
        
        void setVolumeAxesCrosshairLabelsDisplayed(const bool displayed);
        
        bool isVolumeMontageAxesCoordinatesDisplayed() const;
        
        void setVolumeMontageAxesCoordinatesDisplayed(const bool displayed);

        int32_t getVolumeMontageCoordinatePrecision() const;
        
        void setVolumeMontageCoordinatePrecision(const int32_t volumeMontageCoordinatePrecision);
        
        bool isLightingEnabled() const;
        
        void setLightingEnabled(const bool lightingEnabled);
        
        void reset();

        void updateChartModelYokedBrowserTabs();
        
        bool isBrainModelYoked() const;
        
        bool isChartModelYoked() const;
        
        YokingGroupEnum::Enum getBrainModelYokingGroup() const;
        
        void setBrainModelYokingGroup(const YokingGroupEnum::Enum brainModelYokingType);
        
        YokingGroupEnum::Enum getChartModelYokingGroup() const;
        
        void setChartModelYokingGroup(const YokingGroupEnum::Enum chartModelYokingType);
        
        bool isWholeBrainLeftEnabled() const;
        
        void setWholeBrainLeftEnabled(const bool enabled);
        
        bool isWholeBrainRightEnabled() const;
        
        void setWholeBrainRightEnabled(const bool enabled);
        
        bool isWholeBrainCerebellumEnabled() const;
        
        void setWholeBrainCerebellumEnabled(const bool enabled);
        
        float getWholeBrainLeftRightSeparation() const;
        
        void setWholeBrainLeftRightSeparation(const float separation);
        
        float getWholeBrainCerebellumSeparation() const;
        
        void setWholeBrainCerebellumSeparation(const float separation);
        
        ViewingTransformations* getViewingTransformation();
        
        const ViewingTransformations* getViewingTransformation() const;
        
        AnnotationBrowserTab* getManualLayoutBrowserTabAnnotation();
        
        const AnnotationBrowserTab* getManualLayoutBrowserTabAnnotation() const;
        
        bool isDefaultManualTabGeometryBounds() const;
        
        virtual SceneClass* saveToScene(const SceneAttributes* sceneAttributes,
                                        const AString& instanceName);
        
        virtual void restoreFromScene(const SceneAttributes* sceneAttributes,
                                      const SceneClass* sceneClass);
    private:
        class ColorBarFileMap {
        public:
            ColorBarFileMap(AnnotationColorBar* colorBar,
                            CaretMappableDataFile* mapFile,
                            const int32_t mapIndex)
            : m_colorBar(colorBar),
            m_mapFile(mapFile),
            m_mapIndex(mapIndex)
            {
            }
            
            AnnotationColorBar* m_colorBar;
            CaretMappableDataFile* m_mapFile;
            int32_t m_mapIndex;
        };
        
        BrowserTabContent(const BrowserTabContent&);
        
        BrowserTabContent& operator=(const BrowserTabContent&);
        
        void updateBrainModelYokedBrowserTabs();
        
        void updateYokedModelBrowserTabs();
        
        AString getDefaultName() const;
        
        AString getTabNamePrefix() const override;
        
        /** Number of this tab */
        int32_t m_tabNumber;
        
        /** Selected surface model */
        ModelSurfaceSelector* m_surfaceModelSelector;
        
        /** Selected model type */
        ModelTypeEnum::Enum m_selectedModelType;
        
        /** All surface models */
        std::vector<ModelSurface*> m_allSurfaceModels;
        
        /** The volume model */
        ModelVolume* m_volumeModel;
        
        /** The whole brain model */
        ModelWholeBrain* m_wholeBrainModel;
        
        /** The surface montage model */
        ModelSurfaceMontage* m_surfaceMontageModel;
        
        /** The chart model */
        ModelChart* m_chartModel;
        
        /** The chart two model */
        ModelChartTwo* m_chartTwoModel;
        
        /** 
         * Name requested by user interface - reflects contents 
         * such as Surface, Volume Slices, etc
         */
        AString m_guiName;
        
        /**
         * User can set the name of the tab.
         */
        AString m_userName;
        
        /**
         * Clipping planes
         */
        ClippingPlaneGroup* m_clippingPlaneGroup;
        
        /**
         * Rotation matrix for oblique volume viewing
         */
        Matrix4x4* m_obliqueVolumeRotationMatrix;
        
        /** Brain Model Yoking group */
        YokingGroupEnum::Enum m_brainModelYokingGroup;
        
        /** Chart Model Yoking group */
        YokingGroupEnum::Enum m_chartModelYokingGroup;
        
        /** Volume Surface Outlines */
        VolumeSurfaceOutlineSetModel* m_volumeSurfaceOutlineSetModel;
        
        /** Assists with creating/restoring scenes */
        SceneClassAssistant* m_sceneClassAssistant;
        
        /** Transformation for cerebellum viewing */
        ViewingTransformationsCerebellum* m_cerebellumViewingTransformation;
        
        /** Transformation for surface/all viewing */
        ViewingTransformations* m_viewingTransformation;
        
        /** Transformation for surface/all viewing */
        ViewingTransformations* m_flatSurfaceViewingTransformation;
        
        /** Transformation for volume slices viewing */
        ViewingTransformationsVolume* m_volumeSliceViewingTransformation;

        /** Transformation for matrix chart two viewing */
        ViewingTransformations* m_chartTwoMatrixViewingTranformation;
        
        /** Display properties for chart two matrix */
        ChartTwoMatrixDisplayProperties* m_chartTwoMatrixDisplayProperties;
        
        /** Volume slice settings for volume slices */
        VolumeSliceSettings* m_volumeSliceSettings;
        
        /** Whole brain surface settings. */
        WholeBrainSurfaceSettings* m_wholeBrainSurfaceSettings;
        
        /** aspect ratio */
        float m_aspectRatio;
        
        /** aspect ratio locked */
        bool m_aspectRatioLocked;
        
        /** 
         * If true, selected volume slices in tab move to location
         *  of the identification operation.
         */
        bool m_identificationUpdatesVolumeSlices;
        
        /** display crosshairs on volume slices */
        bool m_displayVolumeAxesCrosshairs;
        
        /** display crosshair labels on volume slices */
        bool m_displayVolumeAxesCrosshairLabels;
        
        /** display coordinates on montage */
        bool m_displayVolumeMontageAxesCoordinates;
        
        /** precision for coordinate on montage */
        int32_t m_volumeMontageCoordinatePrecision;
        
        /** enable lighting (shading) added 29 March 2018 */
        bool m_lightingEnabled;
        
        /*
         * True if constructing an instance
         */
        bool isExecutingConstructor;
        
        /** Manual layout brower tab annotation */
        std::unique_ptr<AnnotationBrowserTab> m_manualLayoutBrowserTabAnnotation;
        
        /** Default bounds of manual tab geometry */
        float m_defaultManualTabGeometryBounds[4];
        
        /** Contains all active browser tab content instances */
        static std::set<BrowserTabContent*> s_allBrowserTabContent;
        
    };
    
#ifdef __BROWSER_TAB_CONTENT_DECLARE__
    std::set<BrowserTabContent*> BrowserTabContent::s_allBrowserTabContent;
#endif // __BROWSER_TAB_CONTENT_DECLARE__

} // namespace
#endif  //__BROWSER_TAB_CONTENT__H_
