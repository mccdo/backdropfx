// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>

#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <backdropFX/Manager.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/LocationData.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/ReadFile.h>



osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, .01, 100000. ) );
}

/** \cond */
class ResizeHandler : public osgGA::GUIEventHandler
{
public:
    ResizeHandler( osgViewer::Viewer& viewer ) : _viewer( viewer ) {}
    ~ResizeHandler() {}

    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        bool handled = false;

        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::RESIZE:
        {
            int width = ea.getWindowWidth();
            int height = ea.getWindowHeight();

            const double aspect = (double) width / (double) height;
            _viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );
            _viewer.getCamera()->setViewport( new osg::Viewport( 0, 0, width, height ) );

            backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
        }
        }
        return( handled );
    }

protected:
    osgViewer::Viewer& _viewer;
};
/** \endcond */


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

void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height )
{
    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild(
        backdropFX::Manager::skyDome | backdropFX::Manager::depthPeel
        );

    // TBD don't RTT yet. Get this working in a window first.
    // backdropFX::Manager::instance()->setFBO( createDestinationFBO() );
    // TBD When we do RTT, it is app responsibility to splat an fstp to display the output image.
    //   TBD Maybe backdropFX should contain a convenience mechanism for this.

    // With a reasonably large FOV in the window, the sun and moon typically
    // appear too small. Scale them up to make them more visible.
    backdropFX::SkyDome& sd = backdropFX::Manager::instance()->getSkyDome();
    sd.setSunScale( 2. );
    sd.setMoonScale( 2. );

    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );

#if 0
    // No longer needed, as LocationData now has a default.

    int day( 21 ), hour( 5 ), min( 0 ), sec( 0 );
    osgEphemeris::DateTime dateTime( 2010, 1, day, hour, min, sec );
    backdropFX::LocationData::s_instance()->setDateTime( dateTime );
    backdropFX::LocationData::s_instance()->setLatitudeLongitude( 39.86, -104.68 );
    backdropFX::LocationData::s_instance()->setEastUp(
        osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 0, 1. ) );
#endif
}

void
viewerSetUp( osgViewer::Viewer& viewer, double aspect, osg::Node* node )
{
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );

    // No longer needed, apparently. Seems like Viewer used to focus the
    // camera manipulator on the entire SkyDome, putting it too far away from
    // the scene. This code compensated for that.
    //osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator();
    //viewer.setCameraManipulator( tb );
    //tb->setNode( root.get() );
    //tb->home( 0 );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
    viewer.addEventHandler( new ResizeHandler( viewer ) );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "simple example root" );

    osg::ArgumentParser arguments( &argc, argv );
    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(10,0,0).trans cow.osg" );
    osg::Group* modelParent = new osg::Group;
    modelParent->addChild( loadedModels );
    root->addChild( modelParent );

    // Disable GL_LIGHT0, leaving only the Sun enabled. This is kind of a hack,
    // as we currently can't disable this on the root node, have to disable it on
    // a child node.
    modelParent->getOrCreateStateSet()->setMode( GL_LIGHT0, osg::StateAttribute::OFF );

    // Convert loaded data to use shader composition.
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        smv.setSupportSunLighting( true ); // Use shaders that support Sun lighting.
        root->accept( smv );
    }

    unsigned int width( 800 ), height( 600 );

    backdropFXSetUp( root.get(), width, height );


    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 20, 30, width, height );
    viewer.realize();

    viewerSetUp( viewer, (double)width / (double)height, root.get() );


    viewer.run();


    return( 0 );
}


namespace backdropFX {


/** \page simpleexample Example: simple

The purpose of this example is to show the least amount of code required to
render in backdropFX.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If no models are specified, this exmaple displays the cow and teapot.</td>
  </tr>
</table>

\section handlers Supported OSG Event Handlers
    \li osgViewer::StatsHandler
    \li osgViewer::ThreadingHandler

\c simple also supports a resize handler. However, you must switch
osgViewer threading models to \c SingleThreaded before attempting to resize
the windows.

*/


// backdropFX
}
