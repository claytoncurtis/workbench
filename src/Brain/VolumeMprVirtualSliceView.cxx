
/*LICENSE_START*/
/*
 *  Copyright (C) 2023 Washington University School of Medicine
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

#define __VOLUME_MPR_VIRTUAL_SLICE_VIEW_DECLARE__
#include "VolumeMprVirtualSliceView.h"
#undef __VOLUME_MPR_VIRTUAL_SLICE_VIEW_DECLARE__

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "Matrix4x4.h"
#include "Plane.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"
#include "VolumePlaneIntersection.h"
#include "VolumeTextureCoordinateMapper.h"

using namespace caret;


    
/**
 * \class caret::VolumeMprVirtualSliceView
 * \brief Camera for MPR  volume slice viewing
 * \ingroup Brain
 */

/**
 * @return View type for drawing volume slices in VOLUME view
 */
VolumeMprVirtualSliceView::ViewType
VolumeMprVirtualSliceView::getViewTypeForVolumeSliceView()
{
    return VolumeMprVirtualSliceView::ViewType::VOLUME_VIEW_FIXED_CAMERA;
    
}

/**
 * @return View type for drawing volume slices in ALL view
 */
VolumeMprVirtualSliceView::ViewType
VolumeMprVirtualSliceView::getViewTypeForAllView()
{
    return VolumeMprVirtualSliceView::ViewType::ALL_VIEW_SLICES;
}

/**
 * Constructor.
 */
VolumeMprVirtualSliceView::VolumeMprVirtualSliceView()
: CaretObject()
{
}

/**
 * Constructor.
 * @param viewType
 *    Type of view
 * @param volumeCenterXYZ
 *    Center of the volume
 * @param selectedSlicesXYZ
 *    Coordinates of the selected slices
 * @param sliceWidthHeight
 *    Max coordinates dimension of volume
 * @param sliceViewPlane
 *    The slice plane selected (axial, coronal, parasagittal)
 * @param mprOrientationMode
 *    Orientation (neurological or radiological)
 * @param rotationMatrix
 *    The current rotation matrix
 */
VolumeMprVirtualSliceView::VolumeMprVirtualSliceView(const ViewType viewType,
                                                     const Vector3D& volumeCenterXYZ,
                                                     const Vector3D& selectedSlicesXYZ,
                                                     const float sliceWidthHeight,
                                                     const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                     const VolumeMprOrientationModeEnum::Enum& mprOrientationMode,
                                                     const Matrix4x4& rotationMatrix)
: CaretObject(),
m_viewType(viewType),
m_volumeCenterXYZ(volumeCenterXYZ),
m_selectedSlicesXYZ(selectedSlicesXYZ),
m_sliceWidthHeight(sliceWidthHeight),
m_sliceViewPlane(sliceViewPlane),
m_mprOrientationMode(mprOrientationMode),
m_rotationMatrix(rotationMatrix)
{
    m_transformationMatrix.identity();
    m_cameraXYZ.set(0.0, 0.0, 1.0);
    m_cameraLookAtXYZ.set(0.0, 0.0, 0.0);
    m_cameraUpVector.set(0.0, 1.0, 0.0);
    m_planeRightVector.set(1.0, 0.0, 0.0);
    m_planeUpVector.set(0.0, 1.0, 0.0);
    m_preLookAtTranslation.fill(0.0);
    m_postLookAtTranslation.fill(0.0);

    switch (m_mprOrientationMode) {
        case VolumeMprOrientationModeEnum::NEUROLOGICAL:
            m_neurologicalOrientationFlag = true;
            m_radiologicalOrientationFlag = false;
            break;
        case VolumeMprOrientationModeEnum::RADIOLOGICAL:
            m_neurologicalOrientationFlag = false;
            m_radiologicalOrientationFlag = true;
            break;
    }

    m_sceneAssistant = std::unique_ptr<SceneClassAssistant>(new SceneClassAssistant());

    switch (getViewType()) {
        case ViewType::VOLUME_VIEW_FIXED_CAMERA:
            initializeModeVolumeViewFixedCamera();
            break;
        case ViewType::ALL_VIEW_SLICES:
            initializeModeAllViewSlices();
            break;
    }
}

/**
 * Initialize for ALL view mode slices
 */
void
VolumeMprVirtualSliceView::initializeModeAllViewSlices()
{
    switch (m_sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            if (m_radiologicalOrientationFlag) {
                m_planeRightVector.set(-1.0, 0.0, 0.0);
            }
            else {
                m_planeRightVector.set(1.0, 0.0, 0.0);
            }
            m_planeUpVector.set(0.0, 1.0, 0.0);
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            if (m_radiologicalOrientationFlag) {
                m_planeRightVector.set(-1.0, 0.0, 0.0);
            }
            else {
                m_planeRightVector.set(1.0, 0.0, 0.0);
            }
            m_planeUpVector.set(0.0, 0.0, 1.0);
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            m_planeRightVector.set(0.0, -1.0, 0.0);
            m_planeUpVector.set(0.0, 0.0, 1.0);
            break;
    }
    
    Vector3D v(m_volumeCenterXYZ);
    Vector3D vRight(m_volumeCenterXYZ + m_planeRightVector);
    Vector3D vUp(m_volumeCenterXYZ + m_planeUpVector);

    switch (getViewType()) {
        case ViewType::VOLUME_VIEW_FIXED_CAMERA:
            CaretAssertToDoFatal();
            break;
        case ViewType::ALL_VIEW_SLICES:
            m_rotationMatrix.multiplyPoint3(v);
            m_rotationMatrix.multiplyPoint3(vRight);
            m_rotationMatrix.multiplyPoint3(vUp);
            break;
    }
    
    m_planeRightVector = (vRight - v).normal();
    m_planeUpVector    = (vUp - v).normal();
    Vector3D virtualSlicePlaneVector(m_planeRightVector.cross(m_planeUpVector));
    
    /*
     * Create virtual slice plane with camera look at on plane
     */
    m_virtualSlicePlane = Plane(virtualSlicePlaneVector,
                                m_selectedSlicesXYZ);
    
    /*
     * Does increasing slice coordinate direction face to the
     * user or away from the user
     */
    bool sameDirectionFlag(false);
    switch (m_sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            /*
             * In an axial view, the viewing vector that points to user
             * is inferior to superior and so is increasing Z if in
             * neurological orientation
             */
            sameDirectionFlag = true;
            
            /*
             * Radiological orientation flips viewing vector
             */
            if (m_radiologicalOrientationFlag) {
                sameDirectionFlag = ( ! sameDirectionFlag);
            }
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            /*
             * In coronal view, the viewing vector that points to user
             * is posterior to anterior and so is DECREASING Y if in
             * neurological orientation
             */
            sameDirectionFlag = false;
            
            /*
             * Radiological orientation flips viewing vector
             */
            if (m_radiologicalOrientationFlag) {
                sameDirectionFlag = ( ! sameDirectionFlag);
            }
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            /*
             * In parasagittal view, viewing vector that points to user
             * is right to left and so is INCREASING X if in
             * neurological orientation
             */
            sameDirectionFlag = true;
            break;
    }
    
    if (m_radiologicalOrientationFlag) {
        /*
         * Need to flip sign of normal vector
         */
        m_montageVirutalSliceIncreasingDirectionPlane = Plane(-virtualSlicePlaneVector,
                                                              m_selectedSlicesXYZ);
    }
    else {
        m_montageVirutalSliceIncreasingDirectionPlane = Plane(virtualSlicePlaneVector,
                                                              m_selectedSlicesXYZ);
    }
}

/**
 * Initialize for volume view with camera in a fixed position
 */
void
VolumeMprVirtualSliceView::initializeModeVolumeViewFixedCamera()
{
    const float cameraOffsetDistance(m_sliceWidthHeight * 2.0);
    
    m_cameraXYZ.set(0.0, 0.0, 0.0);
    m_cameraUpVector.set(0.0, 0.0, 0.0);
    
    m_planeUpVector.set(0.0, 0.0, 0.0);
    m_planeRightVector.set(0.0, 0.0, 0.0);
    
    switch (m_sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            /*
             * Neurological:
             *   Looking towards negative Z axis from positive Z
             *   Right is +X
             *   Up    is +Y
             * Radiological:
             */
            m_cameraUpVector.set(0.0, 1.0, 0.0);
            
            if (m_radiologicalOrientationFlag) {
                m_planeRightVector.set(-1.0, 0.0, 0.0);
            }
            else {
                m_planeRightVector.set(1.0, 0.0, 0.0);
            }
            m_planeUpVector.set(0.0, 1.0, 0.0);
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            /*
             * Neurological:
             *   Looking towards positive Y from negative Y
             *   Right is +X
             *   Up    is +Z
             * Radiological:
             */
            m_cameraUpVector.set(0.0, 0.0, 1.0);
            
            if (m_radiologicalOrientationFlag) {
                m_planeRightVector.set(-1.0, 0.0, 0.0);
            }
            else {
                m_planeRightVector.set(1.0, 0.0, 0.0);
            }
            m_planeUpVector.set(0.0, 0.0, 1.0);
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            /*
             * Neurological:
             *   Looking towards positive X from negative X
             *   Right is -Y
             *   Up    is +Z
             * Radiological:
             *   None
             */
            m_cameraUpVector.set(0.0, 0.0, 1.0);
            
            m_planeRightVector.set(0.0, -1.0, 0.0);
            m_planeUpVector.set(0.0, 0.0, 1.0);
            break;
    }
    
    
    /*
     * Set transformation matrix
     */
    const Vector3D transXYZ(m_selectedSlicesXYZ);
    
    m_transformationMatrix.translate(-transXYZ);
    m_transformationMatrix.postmultiply(m_rotationMatrix);
    m_transformationMatrix.translate(transXYZ);

    
    const Vector3D virtualSlicePlaneVector(m_planeRightVector.cross(m_planeUpVector));
    
    /*
     * Virtual slice plane is placed at the selected slice coordinates
     */
    m_virtualSlicePlane = Plane(virtualSlicePlaneVector,
                                m_selectedSlicesXYZ);
    CaretAssert(m_virtualSlicePlane.isValidPlane());
    
    /*
     * Camera "look at" is always center of the volume
     */
    m_cameraLookAtXYZ = m_volumeCenterXYZ;
    
    /*
     * Set camera position
     */
    m_cameraXYZ = (m_cameraLookAtXYZ
                   + (virtualSlicePlaneVector * cameraOffsetDistance));
    
    /*
     * Does increasing slice coordinate direction face to the
     * user or away from the user
     */
    bool sameDirectionFlag(false);
    switch (m_sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            /*
             * In an axial view, the viewing vector that points to user
             * is inferior to superior and so is increasing Z if in
             * neurological orientation
             */
            sameDirectionFlag = true;
            
            /*
             * Radiological orientation flips viewing vector
             */
            if (m_radiologicalOrientationFlag) {
                sameDirectionFlag = ( ! sameDirectionFlag);
            }
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            /*
             * In coronal view, the viewing vector that points to user
             * is posterior to anterior and so is DECREASING Y if in
             * neurological orientation
             */
            sameDirectionFlag = false;
            
            /*
             * Radiological orientation flips viewing vector
             */
            if (m_radiologicalOrientationFlag) {
                sameDirectionFlag = ( ! sameDirectionFlag);
            }
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            /*
             * In parasagittal view, viewing vector that points to user
             * is right to left and so is INCREASING X if in
             * neurological orientation
             */
            sameDirectionFlag = true;
            break;
    }
    
    if (m_radiologicalOrientationFlag) {
        /*
         * Need to flip sign of normal vector
         */
        m_montageVirutalSliceIncreasingDirectionPlane = Plane(-virtualSlicePlaneVector,
                                                              m_selectedSlicesXYZ);
    }
    else {
        m_montageVirutalSliceIncreasingDirectionPlane = Plane(virtualSlicePlaneVector,
                                                              m_selectedSlicesXYZ);
    }
    
    m_preLookAtTranslation.fill(0.0);
    m_postLookAtTranslation.fill(0.0);
}

/**
 * Destructor.
 */
VolumeMprVirtualSliceView::~VolumeMprVirtualSliceView()
{
}

/**
 * @return The viewtype of this instance
 */
VolumeMprVirtualSliceView::ViewType
VolumeMprVirtualSliceView::getViewType() const
{
    return m_viewType;
}

/**
 * Get coordinates for drawing the volume primitive with triangles
 * @param volume
 *    Volume that will be drawn and has coordinates generated
 * @param stereotaxicXyzOut
 *    Output stereotaxic coordinates for drawing the virtual slice
 * @param textureStrOut
 *    Output texture coordinates for drawing the virtual slice
 * @param layersDrawingPlaneOut
 *    Output plane for drawing layers
 */
bool
VolumeMprVirtualSliceView::getTrianglesCoordinates(const VolumeMappableInterface* volume,
                                                   std::vector<Vector3D>& stereotaxicXyzOut,
                                                   std::vector<Vector3D>& textureStrOut,
                                                   Plane& layersDrawingPlaneOut) const
{
    CaretAssert(volume);
    stereotaxicXyzOut.clear();
    textureStrOut.clear();
    layersDrawingPlaneOut = Plane();
    
    
    stereotaxicXyzOut = createVirtualSliceTriangles(volume);
    textureStrOut     = mapTextureCoordinates(volume,
                                              stereotaxicXyzOut);
    CaretAssert(stereotaxicXyzOut.size() == textureStrOut.size());

    if (stereotaxicXyzOut.size() >= 3) {
        layersDrawingPlaneOut = Plane(stereotaxicXyzOut[0],
                                      stereotaxicXyzOut[1],
                                      stereotaxicXyzOut[2]);
    }
    
    return (stereotaxicXyzOut.size() >= 3);
}

/**
 * Map the volume stereotaxic coordinates to texture coordinates
 * @param volume
 *    The volume
 * @param stereotaxicXYZ
 *    Stereotaxic coordiantes that are mapped to texture coordinates
 * @return
 *    The texture coordinates
 */
std::vector<Vector3D>
VolumeMprVirtualSliceView::mapTextureCoordinates(const VolumeMappableInterface* volume,
                                    const std::vector<Vector3D>& stereotaxicXyz) const
{
    std::vector<Vector3D> textureStrOut;

    VolumeTextureCoordinateMapper mapper(volume);
    
    for (auto& volXYZ : stereotaxicXyz) {
        Vector3D xyz(volXYZ);
        textureStrOut.push_back(mapper.mapXyzToStr(xyz));
    }
    
    CaretAssert(textureStrOut.size() == stereotaxicXyz.size());
    return textureStrOut;
}

/**
 * @param volume
 *   The volume
 * @return
 *   A triangle fan for drawing the virtual volume slice that intersects with the volume
 */
std::vector<Vector3D>
VolumeMprVirtualSliceView::createVirtualSliceTriangleFan(const VolumeMappableInterface* volume) const
{
    const Vector3D cameraLookAtVector((m_cameraLookAtXYZ - m_cameraXYZ).normal());

    const Plane plane(cameraLookAtVector,
                      m_selectedSlicesXYZ);
    
    std::vector<Vector3D> triangleFanVertices;

    Matrix4x4 rotationMatrix;
    switch (getViewType()) {
        case ViewType::VOLUME_VIEW_FIXED_CAMERA:
            CaretAssertToDoFatal();
            break;
        case ViewType::ALL_VIEW_SLICES:
            break;
    }
    VolumePlaneIntersection vpi(volume,
                                rotationMatrix);
    Vector3D intersectionCenterXYZ;
    std::vector<Vector3D> intersectionPoints;
    AString errorMessage;
    if (vpi.intersectWithPlane(plane,
                               intersectionCenterXYZ,
                               intersectionPoints,
                               errorMessage)) {
        
        int32_t numPoints(static_cast<int32_t>(intersectionPoints.size()));
        if (numPoints > 6) {
            numPoints = 6;
            CaretAssertMessage(0, "There were more than 6 intersections.  This should never happen");
        }
        
        Vector3D firstPoint;
        if (numPoints > 0) {
            CaretAssertVectorIndex(intersectionPoints, 0);
            firstPoint = intersectionPoints[0];
        }
        
        triangleFanVertices.push_back(intersectionCenterXYZ);
        
        /*
         * Add the intersections
         */
        for (int32_t i = 0; i < numPoints; i++) {
            CaretAssertVectorIndex(intersectionPoints, i);
            triangleFanVertices.push_back(intersectionPoints[i]);
        }
        
        /*
         * Need to add first intersection to create triangle with last intersection
         * since we draw with a triangle fan.
         */
        if (numPoints > 0) {
            CaretAssertVectorIndex(intersectionPoints, 0);
            triangleFanVertices.push_back(intersectionPoints[0]);
        }
        
        /*
         * The maximum number of intersections is 6.
         * We also add the first vertex one more time to
         * create the last triagle so that makes 7.
         * The center is the first fan vertex so that makes 8.
         * When less than 8 fan vertices, add the first vertex
         * to fill in the rest so that there are always
         * 8 fan vertices (the primitive for drawing always
         * uses 8 vertices).
         * Adding the first vertex additional times will create
         * degenerate triangles that OpenGL will cull.
         */
        const int32_t numFanVertices(triangleFanVertices.size());
        for (int32_t i = numFanVertices; i < 8; i++) {
            triangleFanVertices.push_back(firstPoint);
        }
        CaretAssert(triangleFanVertices.size() == 8);
    }
    
    return triangleFanVertices;
}

/**
 * @param volume
 *   The volume
 * @return
 *   Triangle vertices for drawing the virtual volume slice that intersects with the volume
 *   One triangle for each three vertices
 */
std::vector<Vector3D>
VolumeMprVirtualSliceView::createVirtualSliceTriangles(const VolumeMappableInterface* volume) const
{
    std::vector<Vector3D> trianglesVertices;
    
    Matrix4x4 rotationMatrix;
    switch (getViewType()) {
        case ViewType::VOLUME_VIEW_FIXED_CAMERA:
            rotationMatrix = m_transformationMatrix;
            break;
        case ViewType::ALL_VIEW_SLICES:
            rotationMatrix = m_rotationMatrix;
            break;
    }

    VolumePlaneIntersection vpi(volume,
                                rotationMatrix);
    Vector3D intersectionCenterXYZ;
    std::vector<Vector3D> intersectionPoints;
    AString errorMessage;
    if (vpi.intersectWithPlane(m_virtualSlicePlane,
                               intersectionCenterXYZ,
                               intersectionPoints,
                               errorMessage)) {
        
        int32_t numPoints(static_cast<int32_t>(intersectionPoints.size()));
        
        if (numPoints == 0) {
            /*
             * No intersection, OK.
             */
            return trianglesVertices;
        }
        else if (numPoints < 3) {
            /*
             * Edge (2) or vertex (1) intersection
             */
            const AString msg("VolumePlaneIntersection produced "
                              + AString::number(numPoints)
                              + " intersections.  Intersection ignored.");
            CaretLogWarning(msg);
            return trianglesVertices;
        }
        
        if (numPoints > 6) {
            const AString msg("VolumePlaneIntersection produced "
                              + AString::number(numPoints)
                              + " intersections.  There should never be more than 6 intersections");
            CaretLogSevere(msg);
            
            /*
             * Limit to 6 but continue
             */
            numPoints = 6;
        }
        
        if (numPoints >= 3) {
            if (m_sliceViewPlane == VolumeSliceViewPlaneEnum::AXIAL) {
                const bool printIntersectionFlag(false);
                if (printIntersectionFlag) {
                    bool firstFlag(true);
                    for (auto& p : intersectionPoints) {
                        if ( ! firstFlag) std::cout << "   ";
                        std::cout << p.toString() << std::endl;
                        if (firstFlag) firstFlag = false;
                    }
                }
            }
            Vector3D firstPoint;
            if (numPoints > 0) {
                CaretAssertVectorIndex(intersectionPoints, 0);
                firstPoint = intersectionPoints[0];
            }
            
            /*
             * Create the triangles
             */
            for (int32_t i = 0; i < numPoints; i++) {
                /*
                 * Each triangle consists of center and two
                 * consecutive intersection points
                 */
                trianglesVertices.push_back(intersectionCenterXYZ);
                
                CaretAssertVectorIndex(intersectionPoints, i);
                trianglesVertices.push_back(intersectionPoints[i]);
                
                const int32_t nextIndex((i == numPoints - 1)
                                        ? 0
                                        : i + 1);
                CaretAssertVectorIndex(intersectionPoints, nextIndex);
                trianglesVertices.push_back(intersectionPoints[nextIndex]);
            }
        }
        
        /*
         * Maximum number of triangle formed by intersection is 6
         * But we must always have 6 triangles.  So, if needed
         * create degenerate triangles (OpenGL will not draw them).
         */
        const int32_t numIntersectionTriangles(trianglesVertices.size() / 3);
        const int32_t requiredNumberOfTriangles(6);
        for (int32_t i = numIntersectionTriangles; i < requiredNumberOfTriangles; i++) {
            Vector3D zeros(0.0, 0.0, 0.0);
            trianglesVertices.push_back(zeros);
            trianglesVertices.push_back(zeros);
            trianglesVertices.push_back(zeros);
        }
        CaretAssert( (trianglesVertices.size() / 3) == requiredNumberOfTriangles);
    }
    
    return trianglesVertices;
}

/**
 * Copy constructor.
 * @param obj
 *    Object that is copied.
 */
VolumeMprVirtualSliceView::VolumeMprVirtualSliceView(const VolumeMprVirtualSliceView& obj)
: CaretObject(obj)
{
    this->copyHelperVolumeMprVirtualSliceView(obj);
}

/**
 * Assignment operator.
 * @param obj
 *    Data copied from obj to this.
 * @return 
 *    Reference to this object.
 */
VolumeMprVirtualSliceView&
VolumeMprVirtualSliceView::operator=(const VolumeMprVirtualSliceView& obj)
{
    if (this != &obj) {
        CaretObject::operator=(obj); 
        this->copyHelperVolumeMprVirtualSliceView(obj);
    }
    return *this;    
}

/**
 * Helps with copying an object of this type.
 * @param obj
 *    Object that is copied.
 */
void 
VolumeMprVirtualSliceView::copyHelperVolumeMprVirtualSliceView(const VolumeMprVirtualSliceView& obj)
{
    m_viewType = obj.m_viewType;
    
    m_volumeCenterXYZ = obj.m_volumeCenterXYZ;
    
    m_selectedSlicesXYZ = obj.m_selectedSlicesXYZ;
    
    m_sliceWidthHeight = obj.m_sliceWidthHeight;
    
    m_sliceViewPlane = obj.m_sliceViewPlane;
    
    m_mprOrientationMode = obj.m_mprOrientationMode;
    
    m_rotationMatrix = obj.m_rotationMatrix;
    
    m_transformationMatrix = obj.m_transformationMatrix;
    
    m_cameraXYZ = obj.m_cameraXYZ;
    
    m_cameraLookAtXYZ = obj.m_cameraLookAtXYZ;
    
    m_cameraUpVector = obj.m_cameraUpVector;
    
    m_planeRightVector = obj.m_planeRightVector;
    
    m_planeUpVector = obj.m_planeUpVector;
    
    m_preLookAtTranslation = obj.m_preLookAtTranslation;
    
    m_postLookAtTranslation = obj.m_postLookAtTranslation;
    
    m_virtualSlicePlane = obj.m_virtualSlicePlane;
    
    m_montageVirutalSliceIncreasingDirectionPlane = obj.m_montageVirutalSliceIncreasingDirectionPlane;
    
    m_neurologicalOrientationFlag = obj.m_neurologicalOrientationFlag;
    
    m_radiologicalOrientationFlag = obj.m_radiologicalOrientationFlag;
}

/**
 * @return The coordinate of the camera
 */
Vector3D
VolumeMprVirtualSliceView::getCameraXYZ() const
{
    return m_cameraXYZ;
}

/**
 * @return The coordinate of what the camera is looking at
 */
Vector3D
VolumeMprVirtualSliceView::getCameraLookAtXYZ() const
{
    return m_cameraLookAtXYZ;
}

/**
 * @return The up vector of the camera
 */
Vector3D
VolumeMprVirtualSliceView::getCameraUpVector() const
{
    return m_cameraUpVector;
}

/**
 * @return Plane for the virtual slice
 */
Plane
VolumeMprVirtualSliceView::getPlane() const
{
    return m_virtualSlicePlane;
}

/**
 * @return The plane's right vector
 */
Vector3D
VolumeMprVirtualSliceView::getPlaneRightVector() const
{
    return m_planeRightVector;
}

/**
 * @return The plane's up vector
 */
Vector3D
VolumeMprVirtualSliceView::getPlaneUpVector() const
{
    return m_planeUpVector;
}

/**
 * @return Plane for using when increasing slices coordinates
 * in montage view
 */
Plane
VolumeMprVirtualSliceView::getMontageIncreasingDirectionPlane() const
{
    return m_montageVirutalSliceIncreasingDirectionPlane;
}

/**
 * @return Normal vector of virtual slice
 */
Vector3D
VolumeMprVirtualSliceView::getNormalVector() const
{
    Vector3D n;
    m_virtualSlicePlane.getNormalVector(n);
    return n;
}

/**
 * @return Center of volume
 */
Vector3D
VolumeMprVirtualSliceView::getVolumeCenterXYZ() const
{
    return m_volumeCenterXYZ;;
}

/**
 * @return Translation after look at view is set
 */
Vector3D
VolumeMprVirtualSliceView::getPostLookAtTranslation() const
{
    return m_postLookAtTranslation;
}

/**
 * @return The transformation matrix
 */
Matrix4x4
VolumeMprVirtualSliceView::getTransformationMatrix() const
{
    return m_transformationMatrix;
}

/**
 * @return Translation before look at view is set
 */
Vector3D
VolumeMprVirtualSliceView::getPreLookAtTranslation() const
{
    return m_preLookAtTranslation;
}


/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
VolumeMprVirtualSliceView::toString() const
{
    AString txt;
    
    txt.appendWithNewLine("m_sliceViewPlane=" + VolumeSliceViewPlaneEnum::toName(m_sliceViewPlane));
    txt.appendWithNewLine("m_cameraXYZ=" + m_cameraXYZ.toString());
    txt.appendWithNewLine("m_cameraLookAtXYZ=" + m_cameraLookAtXYZ.toString());
    txt.appendWithNewLine("m_cameraUpVector=" + m_cameraUpVector.toString());

    return txt;
}

/**
 * Save information specific to this type of model to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param instanceName
 *    Name of instance in the scene.
 */
SceneClass*
VolumeMprVirtualSliceView::saveToScene(const SceneAttributes* sceneAttributes,
                                 const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "VolumeMprVirtualSliceView",
                                            1);
    m_sceneAssistant->saveMembers(sceneAttributes,
                                  sceneClass);
    
    return sceneClass;
}

/**
 * Restore information specific to the type of model from the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass from which model specific information is obtained.
 */
void
VolumeMprVirtualSliceView::restoreFromScene(const SceneAttributes* sceneAttributes,
                                      const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);
}

