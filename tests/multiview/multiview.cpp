// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osg/Viewport>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>

#include <osg/DisplaySettings>
#include <osg/GraphicsContext>
#include <osgUtil/SceneView>
#include <osgUtil/UpdateVisitor>
#include <osgUtil/GLObjectsVisitor>

#include <backdropFX/Manager.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/RTTViewport.h>
#include <backdropFX/EffectLibraryUtils.h>

#include <osgwTools/ReadFile.h>
#include <osgwTools/Version.h>

#include <vector>



class XformCB : public osg::NodeCallback
{
public:
    XformCB()
      : _rot( 0.0 )
    {}
    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        osg::MatrixTransform* mt = static_cast< osg::MatrixTransform* >( node );
        mt->setMatrix( osg::Matrix::rotate( _rot, 0., 0., 1. ) );
        _rot += .035;
        traverse( node, nv );
    }
    double _rot;
};


osg::ref_ptr< osg::GraphicsContext > gc;
osg::ref_ptr< osgViewer::GraphicsWindow > gw;
osg::ref_ptr< osgUtil::SceneView > sv;

struct ViewInfo
{
    ViewInfo( osg::Viewport* vp, osg::Matrix& view, osg::Matrix& proj )
      : _vp( vp ),
        _view( view ),
        _proj( proj )
    {
    }
    osg::ref_ptr< osg::Viewport > _vp;
    osg::Matrix _view;
    osg::Matrix _proj;
};
typedef std::vector< ViewInfo > ViewInfoList;
ViewInfoList vil;


void setUpViewInWindow( int x, int y, int width, int height )
{
#if( OSGWORKS_OSG_VERSION >= 20908 )
    // April 30 2010, instance() now returns a ref_ptr.
    // June 18 2010, 2.9.8 released.
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
#else
    osg::DisplaySettings* ds = osg::DisplaySettings::instance();
#endif
    osg::ref_ptr< osg::GraphicsContext::Traits > traits = new osg::GraphicsContext::Traits;

    traits->readDISPLAY();
    if (traits->displayNum<0) traits->displayNum = 0;
    
    traits->screenNum = 0;
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();
    if (ds->getStereo())
    {
        switch(ds->getStereoMode())
        {
            case(osg::DisplaySettings::QUAD_BUFFER): traits->quadBufferStereo = true; break;
            case(osg::DisplaySettings::VERTICAL_INTERLACE):
            case(osg::DisplaySettings::CHECKERBOARD):
            case(osg::DisplaySettings::HORIZONTAL_INTERLACE): traits->stencil = 8; break;
            default: break;
        }
    }

    osg::Camera* camera = sv->getCamera();

    gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    camera->setGraphicsContext( gc.get() );

    gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
    if (gw)
    {
        osg::notify(osg::INFO)<<"View::setUpViewOnSingleScreen - GraphicsWindow has been created successfully."<<std::endl;
        gw->getEventQueue()->getCurrentEventState()->setWindowRectangle(x, y, width, height );
    }
    else
    {
        osg::notify(osg::NOTICE)<<"  GraphicsWindow has not been created successfully."<<std::endl;
    }

    double fovy, aspectRatio, zNear, zFar;
    camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

    double newAspectRatio = double(traits->width) / double(traits->height);
    double aspectRatioChange = newAspectRatio / aspectRatio;
    if (aspectRatioChange != 1.0)
    {
        camera->getProjectionMatrix() *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0);
    }

    camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));

    GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;

    camera->setDrawBuffer(buffer);
    camera->setReadBuffer(buffer);
}

void setUpViewAcrossAllScreens( unsigned int& widthOut, unsigned int& heightOut )
{
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi) 
    {
        osg::notify(osg::NOTICE)<<"View::setUpViewAcrossAllScreens() : Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
        return;
    }

#if( OSGWORKS_OSG_VERSION >= 20908 )
    // April 30 2010, instance() now returns a ref_ptr.
    // June 18 2010, 2.9.8 released.
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
#else
    osg::DisplaySettings* ds = osg::DisplaySettings::instance();
#endif
    osg::Camera* camera = sv->getCamera();

    double fovy, aspectRatio, zNear, zFar;        
    camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

    osg::GraphicsContext::ScreenIdentifier si;
    si.readDISPLAY();
    
    // displayNum has not been set so reset it to 0.
    if (si.displayNum<0) si.displayNum = 0;

    unsigned int numScreens = wsi->getNumScreens(si);
    osg::notify( osg::NOTICE ) << "multiview: Number of screens: " << numScreens << std::endl;

    unsigned int width( 0 ), height( 0 );
    for( unsigned int i=0; i<numScreens; i++ )
    {
        unsigned int w, h;
        wsi->getScreenResolution(si, w, h);
        width += w;
        height = osg::maximum< unsigned int >( h, height );
    }
    osg::notify( osg::NOTICE ) << "multiview: Got width: " << width << " height: " << height << std::endl;
    if( ( width > 0 ) && ( height > 0 ) )
    {
        if (si.screenNum<0) si.screenNum = 0;
    
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->hostName = si.hostName;
        traits->displayNum = si.displayNum;
        traits->screenNum = si.screenNum;
        traits->x = 0;
        traits->y = 0;
        traits->width = width;
        traits->height = height;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->windowDecoration = false;
        traits->doubleBuffer = true;
        traits->sharedContext = 0;
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
        
        gc = osg::GraphicsContext::createGraphicsContext(traits.get());
        camera->setGraphicsContext(gc.get());
        
        gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
        if (gw)
        {
            osg::notify(osg::INFO)<<"  GraphicsWindow has been created successfully."<<std::endl;
            gw->getEventQueue()->getCurrentEventState()->setWindowRectangle(0, 0, width, height );
        }
        else
        {
            osg::notify(osg::NOTICE)<<"  GraphicsWindow has not been created successfully."<<std::endl;
        }

        double newAspectRatio = double(traits->width) / double(traits->height);
        double aspectRatioChange = newAspectRatio / aspectRatio;
        if (aspectRatioChange != 1.0)
        {
            camera->getProjectionMatrix() *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0);
        }

        camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));

        GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;

        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);

        widthOut = width;
        heightOut = height;
    }
}

void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height )
{
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild(
        backdropFX::Manager::skyDome | backdropFX::Manager::depthPeel
        );

    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );

    backdropFX::RenderingEffects& rfx = backdropFX::Manager::instance()->getRenderingEffects();
    rfx.setEffectSet( backdropFX::RenderingEffects::effectGlow );
}

osg::Node* glowing( const std::string& fileName )
{
    osg::Node* node = osgDB::readNodeFile( fileName );

    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *node );

    osg::Shader* shader = new osg::Shader( osg::Shader::FRAGMENT );
    std::string shaderName = "shaders/gl2/bdfx-finalize.fs";
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( shaderName ), shader );

    osg::StateSet* ss = node->getOrCreateStateSet();
    osg::Vec4f glowColor( 0., .4, .6, 1. );
    osg::Uniform* glow = new osg::Uniform( "bdfx_glowColor", glowColor );
    ss->addUniform( glow );

    return( node );
}


int main( int argc, char ** argv )
{
    osg::ArgumentParser arguments( &argc, argv );

    unsigned int width( 0 ), height( 0 );
    bool renderToWindow( false );
    osg::notify( osg::NOTICE ) << "  -w <w> <h>\tOpen in a window. Default renders across all available displays." << std::endl;
    if( arguments.read( "-w", width, height ) )
    {
        renderToWindow = true;
        osg::notify( osg::INFO ) << "Found: -w (window mode) width: " << width << ", height: " << height << std::endl;
    }

    unsigned int numViews( 2 );
    osg::notify( osg::NOTICE ) << "  -n <#views>\tNumber of views, either 2 or 3. Default uses 2 views." << std::endl;
    if( arguments.read( "-n", numViews ) )
    {
        osg::notify( osg::INFO ) << "Found: -n (number of views): " << numViews << std::endl;
    }
    unsigned int clampedViews = osg::clampBetween< unsigned int >( numViews, 2, 3 );
    if( numViews != clampedViews )
    {
        numViews = clampedViews;
        osg::notify( osg::NOTICE ) << " Clamping number of views. Using: " << numViews << std::endl;
    }

    bool bdfx( false );
    osg::notify( osg::NOTICE ) << "  -b\tUse backdropFX. Default does not use backdropFX." << std::endl;
    if( bdfx = arguments.read( "-b") )
    {
        osg::notify( osg::INFO ) << "Found: -b (using backdropFX)" << std::endl;
    }


    // Set up scene graph
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "Multiview test case 0" );

    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(6,0,0).trans cow.osg.(-4,0,0).trans" );
    osg::ref_ptr< osg::MatrixTransform > mt = new osg::MatrixTransform;
    mt->setUpdateCallback( new XformCB() );
    mt->addChild( loadedModels );
    if( bdfx )
        mt->addChild( glowing( "teapot.osg.-2,0,6.trans" ) );
    root->addChild( mt.get() );


    if( bdfx )
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        backdropFX::convertFFPToShaderModules( root.get(), &smv );
    }


    // Set up viewer / SceneView
#if 0
    // For testing / dev. Use regular osgViewer w/ 1 view.

    osgViewer::Viewer viewer;
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    if( renderToWindow )
        viewer.setUpViewInWindow( 20, 30, width, height );
    else
    {
        viewer.setUpViewAcrossAllScreens();
        // hack
        width = 1280;
        height = 1024;
    }

    if( bdfx )
    {
        backdropFXSetUp( root.get(), width, height );
        viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );


        osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator( tbm );
        tbm->setNode( root.get() );
        tbm->home( 0 );
    }
    else
    {
        viewer.setSceneData( root.get() );
    }

    viewer.run();

#else

    sv = new osgUtil::SceneView;
    osg::ref_ptr< osg::FrameStamp > fs = new osg::FrameStamp();
    fs->setFrameNumber( 0 );
    sv->setFrameStamp( fs.get() );

    if( renderToWindow )
        setUpViewInWindow( 20, 30, width, height );
    else
        setUpViewAcrossAllScreens( width, height );

    unsigned int maxW( 0 ), maxH( 0 );
    unsigned int lastx( 10 );
    for( unsigned int idx=0; idx<numViews; idx++ )
    {
        float border( 10. );
        float vpw( ( (float)width/(float)numViews ) - border - border );
        float vph( (float)height * .7 );
        float vpy( border + ( ((idx&0x1)==1) ?
            (float)height * .2 : 0. ) );

        osg::Viewport* vp = bdfx ?
            new backdropFX::RTTViewport( lastx, vpy, vpw, vph ) :
            new osg::Viewport( lastx, vpy, vpw, vph );
        osg::Matrix view( osg::Matrix::lookAt(
                osg::Vec3( 0., -30., -7. + (7.*idx) ),
                osg::Vec3( 0., 0., 0. ),
                osg::Vec3( 0., 0., 1. ) ) );
        osg::Matrix proj( osg::Matrix::perspective( 50. - (7.5*idx), vpw/vph, 1., 100. ) );
        vil.push_back( ViewInfo( vp, view, proj ) );
        osg::notify( osg::NOTICE ) << "multiview: Added view, width: " << vpw << " height: " << vph << std::endl;
        lastx += (vpw + border + border);

        maxW = osg::maximum< unsigned int >( maxW, vpw );
        maxH = osg::maximum< unsigned int >( maxH, vph );
    }
    osg::notify( osg::NOTICE ) << "multiview: Maximums: width: " << maxW << " height: " << maxH << std::endl;

    if( bdfx )
    {
        backdropFXSetUp( root.get(), maxW, maxH );
        sv->setSceneData( backdropFX::Manager::instance()->getManagedRoot() );
    }
    else
    {
        sv->setSceneData( root.get() );
    }

    gc->realize();
    gc->makeCurrent();
    osg::Camera* cam = sv->getCamera();
    osg::StateSet* stateSet = cam->getOrCreateStateSet();
    stateSet->setGlobalDefaults();
    sv->setGlobalStateSet( stateSet );
    sv->setDefaults( osgUtil::SceneView::HEADLIGHT );
    gw->grabFocusIfPointerInWindow();
    sv->setState( gc->getState() );
#if( OSGWORKS_OSG_VERSION >= 20908 )
    // April 30 2010, instance() now returns a ref_ptr.
    // June 18 2010, 2.9.8 released.
    sv->setDisplaySettings( osg::DisplaySettings::instance().get() );
#else
    sv->setDisplaySettings( osg::DisplaySettings::instance() );
#endif
    sv->inheritCullSettings( *( sv->getCamera() ) );
    {
        osgUtil::GLObjectsVisitor glov;
        glov.setState( sv->getState() );
        sv->getSceneData()->accept(glov);
    }
    if( bdfx )
    {
        // The SceneView camera should not be allowed to do a clear.
        cam->setClearMask( 0 );
    }

    sv->getUpdateVisitor()->setFrameStamp( fs.get() );
    for( int fc=0; fc<200; fc++ )
    {
        fs->setFrameNumber( fs->getFrameNumber() + 1 );

        // Clear the entire window.
        glViewport( 0, 0, width, height );
        glClearColor( .4, 0., 0., 1. );
        glDisable( GL_SCISSOR_TEST ); // Clear is restricted to the scissor region.
        glClear( GL_COLOR_BUFFER_BIT );

        // Update
        sv->getSceneData()->accept( *( sv->getUpdateVisitor() ) );

        for( unsigned int idx = 0; idx < numViews; idx++ )
        {
            // Set viewport, view matrix, and projection matrix
            cam->setViewport( vil[ idx ]._vp.get() );
            cam->setViewMatrix( vil[ idx ]._view );
            cam->setProjectionMatrix( vil[ idx ]._proj );

            // Get bounds, cull, and draw
            root->getBound();
            sv->cull();
            sv->draw();
        }

        // swap
        gc->swapBuffers();
    }

#endif

    return( 0 );
}


#if 0
// This is the handy function that writes out the amtat-test.osg file
// This file is already checked into the data directory, so you can
// test it with:
//   multiview amtat-test.osg

#include <osg/AutoTransform>
#include <osgwTools/AbsoluteModelTransform.h>
#include <osgwTools/Shapes.h>
#include <osgDB/WriteFile>
void makeamtattest()
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->getOrCreateStateSet()->setMode( GL_LIGHTING, false );

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable( osgwTools::makeWirePlane( osg::Vec3( -5., -5., 0. ),
        osg::Vec3( 10., 0., 0. ), osg::Vec3( 0., 10., 0. ), osg::Vec2s( 4, 4 ) ) );
    root->addChild( geode );

    osg::MatrixTransform* mt = new osg::MatrixTransform(
        osg::Matrix::translate( -20., -50., 3000. ) );
    root->addChild( mt );
    osgwTools::AbsoluteModelTransform* amt = new osgwTools::AbsoluteModelTransform(
        osg::Matrix::translate( 5., -5., 0. ) );
    mt->addChild( amt );
    geode = new osg::Geode;
    geode->addDrawable( osgwTools::makePlane( osg::Vec3( 0., 0., 0. ),
        osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 0., 1. ) ) );
    amt->addChild( geode );

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( -5., 5., 0. ) );
    root->addChild( mt );
    osg::AutoTransform* at = new osg::AutoTransform();
    at->setAutoRotateMode( osg::AutoTransform::ROTATE_TO_CAMERA );
    mt->addChild( at );
    geode = new osg::Geode;
    geode->addDrawable( osgwTools::makePlane( osg::Vec3( 0., 0., 0. ),
        osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 1., 0. ) ) );
    at->addChild( geode );

    osgDB::writeNodeFile( *root, "amtat-test.osg" );
}

#endif



namespace backdropFX {


/** \page multiviewtest Test: multiview -- Support for multiple views using SceneView.

The purpose of this test is to verify correct rendering of two views per frame
from a single SceneView class.

On the command line, you can specify the models to display. Otherwise, the test
loads the default scene, a cow and a teapot. We added a test model in the data
directory, amtat-test.osg, specifically to test osgwTools::AbsoluteModelTransform
and osg::AutoTransform in multiple view usage. When -b is present, the code
always adds a glowing teapot to the scene.

The test attaches the scene (either that the user specifies or the default creates)
to a MatrixTransform with a rotation callback. The code spins the
scene a few times, then exits automatically. It allows no interaction. (This
test doesn't interface with osgGA for event handling.)

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b>-n <numViews></b></td>
    <td>Number of views, either 2 or 3. Default uses 2 views.</td>
  </tr>
  <tr>
    <td><b>-w <w> <h></b></td>
    <td>Open in a window. Default renders across all available displays.</td>
  </tr>
  <tr>
    <td><b>-b</b></td>
    <td>Use backdropFX. Default uses vanilla OSG (no backdropFX).</td>
  </tr>
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If you don't specify a model, this test displays the cow and teapot.</td>
  </tr>
</table>

\section multiviewsupport Support for Multiple Views.

OSG lets an application use SceneView to render to two viewports in the
same window using a single context. This is useful, for example, when two displays
are attached to the same graphics card with a single desktop spanning both displays.
To create a single view, accounting for monitor bezels, your app creates a
single window across the entire desktop, and performs two cull/draw traversals per frame,
one for the left monitor view and one for the right monitor view. Such usage
poses interesting problems for backdropFX, which uses RTT internally to support a
number of rendering effects.

For example, consider the case where your two-monitor system has a total desktop
resolution of 2560x1024. You would set the left monitor viewport to 0,0.1280,1024
(x, y, width, and height), perform a cull/draw pass, then set the right monitor 
viewport to 1280,0,1280,1024, and perform a cull/draw pass. Unfortunately, this means 
backdropFX needs an internal texture size equal to that of your total desktop: 
2560x1024, even though it uses only half of that texture at any one time. For backdropFX 
to make efficient use of GPU memory, we need to find a way to use an internal viewport with
xy=(0,0), and only use non-zero xy when displaying the final textured triangle pair.

While developing support for this usage, we found a number of issues and limitations in OSG
that we had to develop workarounds for in backdropFX.

We initially tried to set our own viewport with a direct call to glViewport(),
always specifying xy=(0,0). We made this call internally in the DepthPeelBin
class just before rendering the child render bins. However, we discovered that
the OSG lazy state setting prohibits this. When the first render leaf applies its
state, the top level Camera applies its own viewport, with non-zero xy. OSG doesn't
appear to support a mechanism to allow application code to set OpenGL state that
overrides state the scene graph during cull collects.

We attempted to create our own custom viewport class that stores the original xy
values, but always specified xy=(0,0) in the apply() call. We eventually made this
work, but encountered the following issues along the way.

The OSG Camera class allows an application to render to texture by attaching
a texture for use as a color buffer, but does not allow an application to attach
a render buffer for use as a depth buffer. Instead, the first time the Camera's
RenderStage executes during draw, the RenderStage implicitly creates a depth render
buffer. Unfortunately, OSG uses viewport x and y values when computing
what it believes is the required size of the render buffer. For example, a viewport of
1280,0,1280,1024 results in a render buffer with width 2560. Unfortunately, OSG prevents 
us from creating our own correctly sized render buffer and attaching it to the Camera, 
so our code creates a depth texture and attach it to the Camera, even though we have no 
need for a depth texture. (For the effects camera, we don't use a depth texture for normalized
DOF and heat distortion distances, we use a second color buffer for that.)

We also encountered problems due to OSG's use of viewport values for other operations
besides applying the viewport. Internal OSG code uses the viewport values to
specify a scissor rectangle, but doesn't provide any application control over this.
Our custom viewport class returns the original xy values when any code queries
for them, and as a result OSG specifies a scissor rectangle that doesn't correspond
to our (0,0)-based viewport.

We modified our custom viewport class to accomodate this. The class zeros the
queriable xy values, but stores the original xy values as \c _xFull and \c _yFull.
The resulting custom viewport class is available as backdropFX::RTTViewport.

In summary, OSG makes it very difficult to render to texture internally with
efficient memory usage and display that texture arbitrarily without the use of
two Cameras, one for RTT and one for window display. However, we believe we now have
a solution that handles this scenario.

Here are some other interesting things we learned about how OSG handles viewports:

Camera and RenderStage have a 1-to-1 mapping in vanilla OSG. The RenderStage applies
the Camera viewport during draw just before it calls glClear(). (OpenGL restricts
the clear operation to within the scissor region.) However, Camera also stores its
viewport in the Camera StateSet. This seemes redundant, given the viewport is set
by the RenderStage, and having it in the StateSet causes the OSG lazy state setting
machanism to overwrite our viewport. However, storing viewport in the StateSet
supports child Cameras in the scene graph by ensuring that it modifies and restores 
the viewport around child Camera rendering.

If you have a child Camera in your scene graph, and you set its viewport to NULL, it
inherits a viewport from the parent Camera. backdropFX does this to the effects Camera,
so it uses the same viewport (in this case, an RTTViewport) as the parent
SceneView Camera.

*/


// backdropFX
}
