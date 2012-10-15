
/*LICENSE_START*/
/*
 * Copyright 2012 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/

#include <cmath>

#include <QCheckBox>
#include <QMouseEvent>
#include <QSizePolicy>

#define __FIBER_SAMPLES_OPEN_G_L_WIDGET_DECLARE__
#include "FiberSamplesOpenGLWidget.h"
#undef __FIBER_SAMPLES_OPEN_G_L_WIDGET_DECLARE__

using namespace caret;

#include "Brain.h"
#include "BrainOpenGL.h"
#include "BrainOpenGLShapeRing.h"
#include "BrainOpenGLShapeSphere.h"
#include "DisplayPropertiesFiberOrientation.h"
#include "Fiber.h"
#include "FiberOrientation.h"
#include "GuiManager.h"
#include "WuQMessageBox.h"

/**
 * \class caret::FiberSamplesOpenGLWidget 
 * \brief OpenGL Widget for drawing fiber samples on a sphere
 */

/**
 * Constructor.
 */
FiberSamplesOpenGLWidget::FiberSamplesOpenGLWidget(QCheckBox* enabledCheckBox,
                                                   QWidget* parent)
: QGLWidget(parent)
{
    m_enabledCheckBox = enabledCheckBox;
    m_ring = NULL;
    m_sphereBig = NULL;
    m_sphereSmall = NULL;
    
    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);
}

/**
 * Destructor.
 */
FiberSamplesOpenGLWidget::~FiberSamplesOpenGLWidget()
{
    makeCurrent();
    
    if (m_ring != NULL) {
        delete m_ring;
    }
    if (m_sphereBig != NULL) {
        delete m_sphereBig;
    }
    if (m_sphereSmall != NULL) {
        delete m_sphereSmall;
    }
}

/**
 * Called once prior to resizeGL() and paintGL() to
 * make any necessary initializations.
 */
void
FiberSamplesOpenGLWidget::initializeGL()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0);
    glFrontFace(GL_CCW);
    glEnable(GL_NORMALIZE);

    //
    // Avoid drawing backfacing polygons
    //
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    glShadeModel(GL_SMOOTH);
        
    createShapes();
}

/**
 * Setup lighting parameters.
 */
void
FiberSamplesOpenGLWidget::setupLighting()
{
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    float lightColor[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor);
    
    float materialColor[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialColor);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    
    float ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    
    
    float lightPosition[] = { 0.0f, 0.0f, 100.0f, 0.0f };
    glPushMatrix();
    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);
    
    //
    // Light 1 position is opposite of light 0
    //
    lightPosition[0] = -lightPosition[0];
    lightPosition[1] = -lightPosition[1];
    lightPosition[2] = -lightPosition[2];
    glLightfv(GL_LIGHT1, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT1);
    glPopMatrix();
    
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);
}

/**
 * Gets called whenever the widget is resized.
 * @param width
 *    New width of widget.
 * @param height
 *    New height of widget.
 */
void
FiberSamplesOpenGLWidget::resizeGL(int width,
                                   int height)
{
    setOrthographicProjection(width, height);
}

/**
 * Set the orthographic projection.
 * @param width 
 *    New width.
 * @param height
 *    New height.
 */
void
FiberSamplesOpenGLWidget::setOrthographicProjection(const int width,
                                                    const int height)
{
    m_widgetWidth = width;
    m_widgetHeight = height;
    
    glViewport(0,
               0,
               width,
               height);
    
    const double orthoSize = 125.0;
    const double aspectRatio = (static_cast<double>(width)
                                / static_cast<double>(height));
    double orthoWidth  = orthoSize;
    double orthoHeight = orthoSize;
    if (aspectRatio > 1.0) {
        orthoWidth *= aspectRatio;
    }
    else {
        const float inverseAspectRatio = 1.0 / aspectRatio;
        orthoHeight *= inverseAspectRatio;
    }
    
    const double orthoNearFar = 5000.0;  // allows zooming
    
    double orthographicRight  =  orthoWidth;
    double orthographicLeft   = -orthoWidth;
    double orthographicTop    =  orthoHeight;
    double orthographicBottom = -orthoHeight;
    double orthographicNear   = -orthoNearFar;
    double orthographicFar    =  orthoNearFar;
    
    glMatrixMode(GL_PROJECTION);
    
    glLoadIdentity();
    glOrtho(orthographicLeft,
            orthographicRight,
            orthographicBottom,
            orthographicTop,
            orthographicNear,
            orthographicFar);
    
    glMatrixMode(GL_MODELVIEW);
}

/**
 * Called when the widget needs to be repainted.
 */
void
FiberSamplesOpenGLWidget::paintGL()
{
    setOrthographicProjection(width(),
                              height());
    glClearColor(0.0,
                 0.0,
                 0.0,
                 1.0);
    glClear(GL_COLOR_BUFFER_BIT
            | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_MODELVIEW);
    
    setupLighting();
    
    glPushMatrix();
    
    glLoadIdentity();

    double rotationMatrixElements[16];
    m_rotationMatrix.getMatrixForOpenGL(rotationMatrixElements);
    glMultMatrixd(rotationMatrixElements);

    
    const GLfloat lineStart = -s_sphereBigRadius * 5.0;
    const GLfloat lineEnd   =  s_sphereBigRadius * 5.0;
    
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    
    /*
     * Axis lines
     */
    glLineWidth(3.0);
    
    glBegin(GL_LINES);

    glColor3f(0.5, 0.0, 0.0);
    glVertex3f(lineStart, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    
    glColor3f(1.0,0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(lineEnd, 0.0, 0.0);
    
    glColor3f(0.0, 0.5, 0.0);
    glVertex3f(0.0, lineStart, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, lineEnd, 0.0);
    
    glColor3f(0.0, 0.0, 0.5);
    glVertex3f(0.0, 0.0, lineStart);
    glVertex3f(0.0, 0.0, 0.0);
    
    glColor3f(0.0,0.0, 1.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, lineEnd);
    
    glEnd();
    
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    
    /*
     * Sphere
     */
    glColor3f(0.7,
              0.7,
              0.7);

    //m_sphereBig->draw();
    
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    
    if (m_enabledCheckBox->isChecked()) {
        drawOrientations();
    }
    
    glPopMatrix();
    
}
/**
 * Convert the given vector to an OpenGL rotation matrix.
 * "This" matrix is set to the identity matrix before createing
 * the rotation matrix.  Use Matrix4x4::getMatrixForOpenGL()
 * to get the matrix after calling this method and then pass
 * array to glMultMatrixd().
 *
 * @param vector
 *    The vector.  Does not need to be a unit vector.
 */
void
getMatrixToOpenGLRotationFromVector(const float vector[3],
                                    double mOut[16])
{
    float vx = vector[0];
    float vy = vector[1];
    float vz = vector[2];
    
    float z = (float)std::sqrt( vx*vx + vy*vy + vz*vz );
    double ax = 0.0f;
    
    double zero = 1.0e-3;
    
    if (std::abs(vz) < zero) {
        ax = 57.2957795*std::acos( vx/z ); // rotation angle in x-y plane
        if ( vx <= 0.0f ) ax = -ax;
    }
    else {
        ax = 57.2957795*std::acos( vz/z ); // rotation angle
        if ( vz <= 0.0f ) ax = -ax;
    }
    
    float rx = -vy*vz;
    float ry = vx*vz;
    
    if ((std::abs(vx) < zero) && (std::fabs(vz) < zero)) {
        if (vy > 0) {
            ax = 90;
        }
    }
    
    glPushMatrix();
    
    glLoadIdentity();
    if (std::abs(vz) < zero)  {
        glRotatef(90.0, 0.0, 1.0, 0.0);
        glRotatef(-ax, 1.0, 0.0, 0.0);
    }
    else {
        glRotatef(ax, rx, ry, 0.0);
    }
    glGetDoublev(GL_MODELVIEW_MATRIX, mOut);
    
    glPopMatrix();
}

/**
 * Draw the fibers on the sphere
 */
void
FiberSamplesOpenGLWidget::drawOrientations()
{
    glDisable(GL_CULL_FACE);
    
    const float startLength = s_sphereBigRadius - 1;
    const float endLength   = s_sphereBigRadius + 2;
    
    DisplayPropertiesFiberOrientation* dpfo = GuiManager::get()->getBrain()->getDisplayPropertiesFiberOrientation();
    
    std::vector<DisplayPropertiesFiberOrientation::OrientationVector> xVectors;
    std::vector<DisplayPropertiesFiberOrientation::OrientationVector> yVectors;
    std::vector<DisplayPropertiesFiberOrientation::OrientationVector> zVectors;
    FiberOrientation* fiberOrientation;
    AString errorMessage;
    if (dpfo->getSphericalOrientationVectors(xVectors,
                                             yVectors,
                                             zVectors,
                                             fiberOrientation,
                                             errorMessage)) {

//        /*
//         * First orientations
//         */
//        glColor3f(1.0, 0.0, 0.0);
//        const int32_t numVectorsX = static_cast<int32_t>(xVectors.size());
//        glLineWidth(1.0);
//        glBegin(GL_LINES);
//        for (int32_t i = 0; i < numVectorsX; i++) {
//            const float* v = xVectors[i].vector;
//            glVertex3f(v[0] * startLength,
//                       v[1] * startLength,
//                       v[2] * startLength);
//            glVertex3f(v[0] * endLength,
//                       v[1] * endLength,
//                       v[2] * endLength);
//        }
//        glEnd();

        bool drawSamples = false;
        if (drawSamples) {
            /*
             * First orientations
             */
            glColor3f(1.0, 0.0, 0.0);
            const int32_t numVectorsX = static_cast<int32_t>(xVectors.size());
            for (int32_t i = 0; i < numVectorsX; i++) {
                float xyz[3] = {
                    xVectors[i].vector[0] * s_sphereBigRadius,
                    xVectors[i].vector[1] * s_sphereBigRadius,
                    xVectors[i].vector[2] * s_sphereBigRadius
                };
                glPushMatrix();
                glTranslatef(xyz[0], xyz[1], xyz[2]);
                m_sphereSmall->draw();
                glPopMatrix();
                
                glPushMatrix();
                glTranslatef(-xyz[0], -xyz[1], -xyz[2]);
                m_sphereSmall->draw();
                glPopMatrix();
            }
            
            /*
             * Second orientations
             */
            glColor3f(0.0, 1.0, 0.0);
            const int32_t numVectorsY = static_cast<int32_t>(yVectors.size());
            for (int32_t i = 0; i < numVectorsY; i++) {
                float xyz[3] = {
                    yVectors[i].vector[0] * s_sphereBigRadius,
                    yVectors[i].vector[1] * s_sphereBigRadius,
                    yVectors[i].vector[2] * s_sphereBigRadius
                };
                glPushMatrix();
                glTranslatef(xyz[0], xyz[1], xyz[2]);
                m_sphereSmall->draw();
                glPopMatrix();
                
                glPushMatrix();
                glTranslatef(-xyz[0], -xyz[1], -xyz[2]);
                m_sphereSmall->draw();
                glPopMatrix();
            }
            
            /*
             * Third orientations
             */
            glColor3f(0.0, 0.0, 1.0);
            const int32_t numVectorsZ = static_cast<int32_t>(zVectors.size());
            for (int32_t i = 0; i < numVectorsZ; i++) {
                float xyz[3] = {
                    zVectors[i].vector[0] * s_sphereBigRadius,
                    zVectors[i].vector[1] * s_sphereBigRadius,
                    zVectors[i].vector[2] * s_sphereBigRadius
                };
                glPushMatrix();
                glTranslatef(xyz[0], xyz[1], xyz[2]);
                m_sphereSmall->draw();
                glPopMatrix();
                
                glPushMatrix();
                glTranslatef(-xyz[0], -xyz[1], -xyz[2]);
                m_sphereSmall->draw();
                glPopMatrix();
            }
        }
        
        /*
         * Orientation Ellipse
         */
        const float radiansToDegrees = 180.0 / M_PI;
        if (fiberOrientation != NULL) {
            const int32_t numFibers = fiberOrientation->m_numberOfFibers;
            for (int32_t j = 0; j < numFibers; j++) {
                const Fiber* fiber = fiberOrientation->m_fibers[j];
                if (fiber->m_valid) {
                    const int32_t colorIndex = j % 3;
                    switch (colorIndex) {
                        case 0:
                            glColor3f(1.0, 0.0, 0.0);
                            break;
                        case 1:
                            glColor3f(0.0, 1.0, 0.0);
                            break;
                        case 2:
                            glColor3f(0.0, 0.0, 1.0);
                            break;
                    }
                    {
                        
                        const float maxAngle = M_PI_2 * 0.95;
                        float baseMajorAngle = fiber->m_fanningMajorAxisAngle;
                        if (baseMajorAngle > maxAngle) {
                            baseMajorAngle = maxAngle;
                        }
                        float baseMinorAngle = fiber->m_fanningMinorAxisAngle;
                        if (baseMinorAngle > maxAngle) {
                            baseMinorAngle = maxAngle;
                        }
                        
                        const float z = s_sphereBigRadius * 1.05;
                        const float baseRadiusScaling = 1.0;
                        const float maxWidth = z;
                        const float majorAxis = std::min(z * std::tan(baseMajorAngle) * baseRadiusScaling,
                                                         maxWidth);
                        const float minorAxis = std::min(z * std::tan(baseMinorAngle) * baseRadiusScaling,
                                                         maxWidth);
                        
                        
                        /*
                         * Orientations are uni-directional so draw on both
                         * sides of the sphere.
                         */
                        for (int32_t iTwoTimes = 0; iTwoTimes < 2; iTwoTimes++) {
                            const bool secondTimeFlag = (iTwoTimes == 1);
                            
                            Matrix4x4 matrix;
                            matrix.setMatrixToOpenGLRotationFromVector(fiber->m_directionUnitVector);
                            if (secondTimeFlag) {
                                matrix.identity();
                                const float vector[3] = {
                                    -fiber->m_directionUnitVector[0],
                                    -fiber->m_directionUnitVector[1],
                                    -fiber->m_directionUnitVector[2]
                                };
                                matrix.setMatrixToOpenGLRotationFromVector(vector);
                            }
                            
                            double m[16];
                            matrix.getMatrixForOpenGL(m);
                            
                            float transVector[3] = {
                                fiber->m_directionUnitVector[0] * s_sphereBigRadius * 1.05,
                                fiber->m_directionUnitVector[1] * s_sphereBigRadius * 1.05,
                                fiber->m_directionUnitVector[2] * s_sphereBigRadius * 1.05
                            };
                            if (secondTimeFlag) {
                                transVector[0] *= -1.0;
                                transVector[1] *= -1.0;
                                transVector[2] *= -1.0;
                            }
                            
                            getMatrixToOpenGLRotationFromVector(transVector, m);
                            
                            glPushMatrix();
                            glTranslatef(transVector[0],
                                         transVector[1],
                                         transVector[2]);
                            glMultMatrixd(m);
//                            glTranslatef(0.0,
//                                         0.0,
//                                         z);
                            
                            float psi = fiber->m_psi * radiansToDegrees;
                            if (secondTimeFlag) {
                                //glRotatef(-90.0, 0.0, 0.0, 1.0);
                                glRotatef(180.0, 0.0, 1.0, 0.0);
                                psi = -psi;
                            }
                            
                            glRotatef(psi,
                                      0.0,
                                      0.0,
                                      1.0);
                            /* TEST */
                            glBegin(GL_LINES);
                            if (secondTimeFlag) {
                                glColor3f(1.0, 1.0, 0.0);
                            }
                            else {
                                glColor3f(1.0, 1.0, 1.0);
                            }
                            glVertex3f(-10.0, 0.0, 0.0);
                            glVertex3f( 10.0, 0.0, 0.0);
                            glColor3f(0.0, 1.0, 1.0);
                            glVertex3f( 0.0, -10.0, 0.0);
                            glVertex3f( 0.0,  10.0, 0.0);
                            glEnd();
                            
                            switch (colorIndex) {
                                case 0:
                                    glColor3f(1.0, 0.0, 0.0);
                                    break;
                                case 1:
                                    glColor3f(0.0, 1.0, 0.0);
                                    break;
                                case 2:
                                    glColor3f(0.0, 0.0, 1.0);
                                    break;
                            }
                            std::cout << j << ": psi=" << psi << std::endl;
                            /* TEST */
                            
                            
                            
                            glScalef(majorAxis * 2.0,
                                     minorAxis * 2.0,
                                     1.0);                            
                            m_ring->draw();
                            
                            glPopMatrix();
                        }
                    }
                    
                    /*
                     * Sphere at end of directional vector
                     */
//                    glPushMatrix();
//                    glTranslatef(fiber->m_directionUnitVector[0] * s_sphereBigRadius * 1.01,
//                                 fiber->m_directionUnitVector[1] * s_sphereBigRadius * 1.01,
//                                 fiber->m_directionUnitVector[2] * s_sphereBigRadius * 1.01);
//                    glScalef(3.0, 3.0, 3.0);
//                    m_sphereSmall->draw();
//                    glPopMatrix();
                }
            }
        }
    }
    else {
        if (errorMessage.isEmpty() == false) {
            WuQMessageBox::errorOk(this,
                                   errorMessage);
        }
    }
    
    glEnable(GL_CULL_FACE);
}

/**
 * Create the shapes on which fibers are viewed.
 */
void
FiberSamplesOpenGLWidget::createShapes()
{
    if (m_sphereBig == NULL) {
        m_sphereBig = new BrainOpenGLShapeSphere(25,
                                          s_sphereBigRadius);
    }
    if (m_sphereSmall == NULL) {
        m_sphereSmall = new BrainOpenGLShapeSphere(5,
                                              s_sphereSmallRadius);
    }
    
    if (m_ring == NULL) {
        m_ring = new BrainOpenGLShapeRing(10,
                                     0.85,
                                     1.0);
    }
}

/**
 * Receive mouse press events from Qt.
 * @param me
 *    The mouse event.
 */
void
FiberSamplesOpenGLWidget::mousePressEvent(QMouseEvent* me)
{
    const Qt::MouseButton button = me->button();
//    const Qt::KeyboardModifiers keyModifiers = me->modifiers();
    
    if (button == Qt::LeftButton) {
        const int mouseX = me->x();
        const int mouseY = m_widgetHeight - me->y();
        
        m_mousePressX = mouseX;
        m_mousePressY = mouseY;
        
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
    }
    else {
        m_mousePressX = -10000;
        m_mousePressY = -10000;
    }
    
    me->accept();
}

/**
 * Receive mouse button release events from Qt.
 * @param me
 *    The mouse event.
 */
void
FiberSamplesOpenGLWidget::mouseReleaseEvent(QMouseEvent* me)
{
    m_mousePressX = -10000;
    m_mousePressY = -10000;
    
    me->accept();
}

/**
 * Receive mouse move events from Qt.
 * @param me
 *    The mouse event.
 */
void
FiberSamplesOpenGLWidget::mouseMoveEvent(QMouseEvent* me)
{
    const Qt::MouseButton button = me->button();
//    Qt::KeyboardModifiers keyModifiers = me->modifiers();
    
    if (button == Qt::NoButton) {
        if (me->buttons() == Qt::LeftButton) {
            const int mouseX = me->x();
            const int mouseY = m_widgetHeight - me->y();
            
            const int dx = mouseX - m_lastMouseX;
            const int dy = mouseY - m_lastMouseY;
            const int absDX = (dx >= 0) ? dx : -dx;
            const int absDY = (dy >= 0) ? dy : -dy;
            
            if ((absDX > 0)
                || (absDY > 0)) {
                m_rotationMatrix.rotateX(-dy);
                m_rotationMatrix.rotateY(dx);
            }
            
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        }
    }
    
    me->accept();
    
    updateGL();
}

