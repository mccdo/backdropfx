// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>
#include <osgUtil/Optimizer>

#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <backdropFX/Manager.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/DepthPeelUtils.h>
#include <backdropFX/RenderingEffects.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <backdropFX/LocationData.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/CameraConfigObject.h>

#include <backdropFX/Version.h>
#include "DefaultScene.h"


osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, .01, 100000. ) );
}

class KbdEventHandler : public osgGA::GUIEventHandler
{
public:
    KbdEventHandler( osg::Group* root, osgViewer::Viewer& viewer )
      : _root( root ),
        _viewer( viewer ),
        _frameCount( -1 ),
        _featureFlags( backdropFX::Manager::defaultFeatures ),
        _glow( false )
    {}

    void usage()
    {
        osg::notify( osg::NOTICE ) << "  D\tDump images for a single frame." << std::endl;
        osg::notify( osg::NOTICE ) << "  P\tToggle DepthPartition." << std::endl;
        osg::notify( osg::NOTICE ) << "  G\tToggle glow effect." << std::endl;
        osg::notify( osg::NOTICE ) << "  K\tToggle SkyDome." << std::endl;
        osg::notify( osg::NOTICE ) << "  T\tToggle depth peel." << std::endl;
        osg::notify( osg::NOTICE ) << "  H\tToggle shadows." << std::endl;
        osg::notify( osg::NOTICE ) << "  adxw\tMove light." << std::endl;
    }
    
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& )
    {
        backdropFX::Manager* mgr( backdropFX::Manager::instance() );
        bool handled( false );
        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::RESIZE:
        {
            int width = ea.getWindowWidth();
            int height = ea.getWindowHeight();

            const double aspect = (double) width / (double) height;
            _viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );
            _viewer.getCamera()->getViewport()->setViewport( 0, 0, width, height );

            backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
        }
        case osgGA::GUIEventAdapter::FRAME:
        {
            if( _frameCount == 0 )
            {
                if( ( mgr->getDebugMode() & backdropFX::BackdropCommon::debugImages ) != 0 )
                    mgr->setDebugMode( mgr->getDebugMode() & ~backdropFX::BackdropCommon::debugImages );
                _frameCount = -1;
            }
            else
                _frameCount--;
        }
        case osgGA::GUIEventAdapter::KEYDOWN:
        {
            switch( ea.getKey() )
            {
            case 'D':
                mgr->setDebugMode( mgr->getDebugMode() | backdropFX::BackdropCommon::debugImages );
                _frameCount = 1;
                handled = true;
                break;
            case 'K':
                flipBit( _featureFlags, backdropFX::Manager::skyDome );
                mgr->rebuild( _featureFlags );
                handled = true;
                break;
            case 'P':
            {
                // You can't remove DepthPartition with the rebuild feature flags.
                // Depth partition is always present. You can, however, "disable" it
                // be setting the number of passes to 1.
                //   Num passes == 0 : auto compute the best number of passes
                //   Num passes == 1 : Disable, just do one partition pass.
                backdropFX::DepthPartition& dPart = mgr->getDepthPartition();
                unsigned int currentPasses = dPart.getNumPartitions();
                if( currentPasses == 0 )
                    // "disable"
                    dPart.setNumPartitions( 1 );
                else
                    // "enable"
                    dPart.setNumPartitions( 0 );
                handled = true;
                break;
            }
            case 'G':
            {
                _glow = !_glow;
                backdropFX::RenderingEffects& rfx = backdropFX::Manager::instance()->getRenderingEffects();
                const unsigned int bit = backdropFX::RenderingEffects::effectGlow;
                unsigned int fxset = rfx.getEffectSet();
                rfx.setEffectSet( _glow ? ( fxset | bit ) : ( fxset & ~bit ) );
                handled = true;
                break;
            }
            case 'T':
                flipBit( _featureFlags, backdropFX::Manager::depthPeel );
                mgr->rebuild( _featureFlags );
                handled = true;
                break;
            case 'H':
                flipBit( _featureFlags, backdropFX::Manager::shadowMap );
                mgr->rebuild( _featureFlags );
                handled = true;
                break;
            case 'a':
            {
                osg::Vec4 pos = backdropFX::Manager::instance()->getLightPosition( 0 );
                pos[ 0 ] -= 1.;
                backdropFX::Manager::instance()->setLightPosition( 0, pos );
                handled = true;
                break;
            }
            case 'd':
            {
                osg::Vec4 pos = backdropFX::Manager::instance()->getLightPosition( 0 );
                pos[ 0 ] += 1.;
                backdropFX::Manager::instance()->setLightPosition( 0, pos );
                handled = true;
                break;
            }
            case 'x':
            {
                osg::Vec4 pos = backdropFX::Manager::instance()->getLightPosition( 0 );
                pos[ 1 ] -= 1.;
                backdropFX::Manager::instance()->setLightPosition( 0, pos );
                handled = true;
                break;
            }
            case 'w':
            {
                osg::Vec4 pos = backdropFX::Manager::instance()->getLightPosition( 0 );
                pos[ 1 ] += 1.;
                backdropFX::Manager::instance()->setLightPosition( 0, pos );
                handled = true;
                break;
            }
            case 'F':
            {
                backdropFX::Manager::instance()->setFogState(
                    backdropFX::Manager::instance()->getFog(),
                    !( backdropFX::Manager::instance()->getFogEnable() ) );
                // TBD auto-run RSM? See Manager::setFogState()
                backdropFX::RebuildShaderModules rsm;
                backdropFX::Manager::instance()->getManagedRoot()->accept( rsm );
                handled = true;
                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        return( handled );
    }

protected:
    osg::ref_ptr< osg::Group > _root;
    osgViewer::Viewer& _viewer;
    int _frameCount;
    unsigned int _featureFlags;
    bool _glow;

    void flipBit( unsigned int& flags, unsigned int bit )
    {
        const bool on = ( ( flags & bit ) != 0 );
        if( on )
            flags &= ~bit;
        else
            flags |= bit;
    }
};



osg::FrameBufferObject*
createDestinationFBO()
{
    osg::ref_ptr< osg::Texture2D > tex( new osg::Texture2D );
    tex->setTextureSize( 1920, 1200 );
    tex->dirtyTextureObject();
    tex->setInternalFormat( GL_RGBA );
    tex->setBorderWidth( 0 );
    tex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    osg::ref_ptr< osg::FrameBufferObject > fbo( new osg::FrameBufferObject );
    fbo->setAttachment( osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( tex.get() ) );
    // backdropFX doesn't output depth buffer, so no need to specify it.

    return( fbo.release() );
}



void backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height, bool yup, bool renderToWindow )
{
    // Set scene globals for overall appearance
    // NOTE: rebuild() implicitly calls RSM. If we set up scene
    // globals *before* rebuild(), we don't need to run RSM (again).
    // if we set global state *after* rebuild(), we need an
    // additional explicit RSM run.
    // TBD auto-run RSM? See Manager::setFogState
    osg::ref_ptr< osg::Fog > fog = new osg::Fog();
    fog->setMode( osg::Fog::EXP );
	fog->setColor( osg::Vec4( 0.31, 0.39, 0.5, 1.0 ) );
    fog->setDensity( 0.05f );
	fog->setStart( 0.f );
	fog->setEnd( 500.f );
    backdropFX::Manager::instance()->setFogState( fog.get() );

    osg::ref_ptr< osg::Light > light = new osg::Light;
    light->setLightNum( 0 );
    light->setPosition( osg::Vec4( 1.1, -30.0, 30.0, 1.0 ) );
    backdropFX::Manager::instance()->setLight( light.get() );


    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild();

    // With a reasonably large FOV in the window, the sun and moon typically
    // appear too small. Scale them up to make them more visible.
    backdropFX::SkyDome& sd = backdropFX::Manager::instance()->getSkyDome();
    sd.setSunScale( 2. );
    sd.setMoonScale( 2. );

    // Support +y up, if requested.
    if( yup )
    {
        backdropFX::LocationData::s_instance()->setEastUp(
            osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 1., 0. ) );
    }

    // Depth partitioning
    backdropFX::DepthPartition& dPart = backdropFX::Manager::instance()->getDepthPartition();
    dPart.setNumPartitions( 0 );
    dPart.setRatio( 1. / 500. );

    // If we're not rendering to window, then we're rendering fullscreen and (probably)
    // using slave cameras to handle multiple displays. In this case, we want the
    // RenderingEffectsStage added as a post-render stage so that the slave camera
    // clear doesn't erase our rendering.
    if( !renderToWindow )
        backdropFX::Manager::instance()->getRenderingEffects().setRenderOrder(
            backdropFX::RenderingEffects::POST_RENDER );


    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void
viewerSetUp( osgViewer::Viewer& viewer, double aspect, osg::Node* root )
{
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );

    osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator();
    viewer.setCameraManipulator( tb );
    tb->setNode( root );
    tb->home( 0 );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
}

int
main( int argc, char ** argv )
{
    backdropFX::Manager::instance()->setDebugMode( backdropFX::BackdropCommon::debugShaders );

    osg::notify( osg::NOTICE ) << backdropFX::getVersionString() << std::endl;

    osg::notify( osg::NOTICE ) << "Usage: verticalslice [files] [options]" << std::endl;
    osg::notify( osg::NOTICE ) << "If [files] is absent, the default scene is rendered." << std::endl;
    osg::notify( osg::NOTICE ) << "Options:" << std::endl;

    osg::ArgumentParser arguments( &argc, argv );

    unsigned int width, height;
    bool renderToWindow( false );
    osg::notify( osg::NOTICE ) << "  -w <w> <h>\tOpen in a window (otherwise, fullscreen)" << std::endl;
    if( arguments.read( "-w", width, height ) )
    {
        renderToWindow = true;
        osg::notify( osg::INFO ) << "-w (window mode) width: " << width << ", height: " << height << std::endl;
    }
    bool renderToAllScreens( false );
    osg::notify( osg::NOTICE ) << "  --all\tIf '-w' is not present, render to all screens. Default: Render to screen 0." << std::endl;
    if( !renderToWindow && arguments.read( "--all" ) )
    {
        renderToAllScreens = true;
        osg::notify( osg::INFO ) << "--all: Rendering to all screens." << std::endl;
    }

    bool yup( false );
    osg::notify( osg::NOTICE ) << "  -yup\tUse +y as up vector. Otherwise, use +z." << std::endl;
    if( arguments.read( "-yup" ) )
    {
        yup = true;
        osg::notify( osg::INFO ) << "-yup: Using +y for up vector." << std::endl;
    }

    float globalTransparency( 1.f );
    osg::notify( osg::NOTICE ) << "  -t <trans>\tGlobal transparency between 0.0 and 1.0. Default: 1.0." << std::endl;
    if( arguments.read( "-t", globalTransparency ) )
    {
        osg::notify( osg::INFO ) << "-t (global transparency): " << globalTransparency << std::endl;
    }

    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "verticalslice root" );

    // Debugging: Dump out preprocessed shaders to current working directory.
    // (To get all shaders dumped, must do this now, before subsequent calls into
    // Manager trigger shader loads.)
    backdropFX::Manager::instance()->setDebugMode(
    /*    backdropFX::BackdropCommon::debugShaders | */
        backdropFX::BackdropCommon::debugConsole );

    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    DefaultScene ds;
    osg::Node* scene = ds( loadedModels );
    root->addChild( scene );

    {
        osgUtil::Optimizer opt;
        opt.optimize( root.get(),
            osgUtil::Optimizer::REMOVE_LOADED_PROXY_NODES |
            osgUtil::Optimizer::SHARE_DUPLICATE_STATE
            );
    }

    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        backdropFX::convertFFPToShaderModules( root.get(), &smv );
    }

    if( globalTransparency != 1.f )
    {
        backdropFX::transparentEnable( root.get(), globalTransparency );
    }


    osgViewer::Viewer viewer;
    if( renderToWindow )
    {
        viewer.setUpViewInWindow( 20, 30, width, height );
        viewer.realize();
    }
    else
    {
        if( renderToAllScreens )
            viewer.setUpViewAcrossAllScreens();
        else
            viewer.setUpViewOnSingleScreen( 0 );
        viewer.realize();
        if( ( viewer.getCamera() ) && ( viewer.getCamera()->getViewport() ) )
        {
            const osg::Viewport* vp = viewer.getCamera()->getViewport();
            width = (unsigned int) vp->width();
            height = (unsigned int) vp->height();
        }
        else
        {
            osg::notify( osg::ALWAYS ) << "No camera" << std::endl;
            width = 1920;
            height = 1200;
        }
        osg::notify( osg::ALWAYS ) << "  Using width " << width << " and height " << height << std::endl;
    }
    osg::notify( osg::NOTICE ) << "  -st\tForce SingleThreaded mode. Default is CullDrawThreadPerContext." << std::endl;
    if( arguments.read( "-st" ) )
        viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    else
        viewer.setThreadingModel( osgViewer::ViewerBase::CullDrawThreadPerContext );

    backdropFXSetUp( root.get(), width, height, yup, renderToWindow );
    viewerSetUp( viewer, (double)width/(double)height, root.get() );


    KbdEventHandler* kbh = new KbdEventHandler( root.get(), viewer );
    osg::notify( osg::NOTICE ) << "Key commands:" << std::endl;
    kbh->usage();
    viewer.addEventHandler( kbh );


    viewer.run();


    // Cleanup and exit.
    backdropFX::Manager::instance( true );
    return( 0 );
}


namespace backdropFX {


/** \page verticalslicetest Test: verticalslice

Tests several aspects of backdropFX rendering.

Specify model(s) on the command line, or use the default scene, consisting of
streamlines, and teapots that demonstrate various rendering effects.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b>--all</b></td>
    <td>If '-w' is not present, render to all screens. Default: Render to screen 0 when 'w' is not present.</td>
  </tr>
  <tr>
    <td><b>-st</b></td>
    <td>Force SingleThreaded mode. Default is CullDrawThreadPerContext.</td>
  </tr>
  <tr>
    <td><b>-t <trans></b></td>
    <td>Global transparency between 0.0 and 1.0. Default: 1.0.</td>
  </tr>
  <tr>
    <td><b>-w <w> <h></b></td>
    <td>Open in a window (otherwise, fullscreen)</td>
  </tr>
  <tr>
    <td><b>-yup</b></td>
    <td>Use +y as up vector. Otherwise, use +z.</td>
  </tr>
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If no models are specified, this test displays a default scene.</td>
  </tr>
</table>

\section kbd Keyboard Commands

In the following table, please note use of caps.

<table border="0">
  <tr>
    <td><b>D</b></td>
    <td>Dump images for a single frame.</td>
  </tr>
  <tr>
    <td><b>P</b></td>
    <td>Toggle DepthPartition.</td>
  </tr>
  <tr>
    <td><b>G</b></td>
    <td>Toggle glow effect.</td>
  </tr>
  <tr>
    <td><b>K</b></td>
    <td>Toggle SkyDome.</td>
  </tr>
  <tr>
    <td><b>T</b></td>
    <td>Toggle depth peel.</td>
  </tr>
  <tr>
    <td><b>H</b></td>
    <td>Toggle shadows.</td>
  </tr>
  <tr>
    <td><b>adxw</b></td>
    <td>Move shadow light source position.</td>
  </tr>
  <tr>
    <td><b>F</b></td>
    <td>Toggle fog.</td>
  </tr>
</table>

\section handlers Supported OSG Event Handlers
    \li osgViewer::StatsHandler
    \li osgViewer::ThreadingHandler

\section ss Screen Shots

\image html fig00-verticalslice.jpg

Command: \code verticalslice -w 800 -h 600 \endcode This shows the default scene
consisting of four teapots (two transparent), with red and blue animated streamlines.

\image html fig01-verticalslice.jpg

Same command, but view rotated downward and the user has hit the 'T' key to disable transparency.
Note all four teapots are now opaque, and the streamline animation has stopped and now
renders incorrectly.

\image html fig02-verticalslice.jpg

Same command, but the user had hit the 'G' key to enable glowing on two of the
teapots.

\image html fig03-verticalslice.jpg

Command: \code verticalslice -w 800 -h 600 -t 0.28 dumptruck.osg \endcode This 
renders the dumptruck model with a glocal transparency of 0.28 alpha.

*/


// backdropFX
}
