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
#include <backdropFX/DepthPartition.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/ReadFile.h>



osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, .05, 1000. ) );
}

/** \cond */
class ResizeHandler : public osgGA::GUIEventHandler
{
public:
    ResizeHandler( osgViewer::Viewer& viewer, unsigned int width, unsigned int height )
      : _viewer( viewer ),
        _maxWidth( width ),
        _maxHeight( height )
    {}
    ~ResizeHandler() {}

    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        bool handled = false;

        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::RESIZE:
        {
            unsigned int width = ( unsigned int )ea.getWindowWidth();
            unsigned int height = ( unsigned int )ea.getWindowHeight();

            const double aspect = (double) width / (double) height;
            _viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );
            _viewer.getCamera()->getViewport()->setViewport( 0, 0, width, height );

            if( ( width > _maxWidth ) || ( height > _maxHeight ) )
            {
                _maxWidth = osg::maximum< unsigned int >( width, _maxWidth );
                _maxHeight = osg::maximum< unsigned int >( height, _maxHeight );
                backdropFX::Manager::instance()->setTextureWidthHeight( _maxWidth, _maxHeight );
            }
        }
        }
        return( handled );
    }

protected:
    osgViewer::Viewer& _viewer;

    unsigned int _maxWidth, _maxHeight;
};
/** \endcond */


void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height )
{
    // Set global state.
    // For 'alloff', this is required. When SkyDome is off, bdfx doesn't
    // provide a default Sun light. Because lighting is enabled, the app
    // *must* turn on at least one light source.
    osg::ref_ptr< osg::Light > light = new osg::Light;
    light->setLightNum( 0 );
    light->setPosition( osg::Vec4( 1.1, -30.0, 30.0, 1.0 ) );
    backdropFX::Manager::instance()->setLight( light.get() );

    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild( 0 ); // sky dome / depth peel off

    // Disable depth partitioning.
    backdropFX::Manager::instance()->getDepthPartition().setNumPartitions( 1 );


    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void
viewerSetUp( osgViewer::Viewer& viewer, unsigned int width, unsigned int height, osg::Node* node )
{
    double aspect = (double)width / (double)height;

    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    //viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    //viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

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
    viewer.addEventHandler( new ResizeHandler( viewer, width, height ) );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "alloff example root" );

    osg::ArgumentParser arguments( &argc, argv );
    bool useSMV = ( arguments.find( "-nosmv" ) < 1 );

    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(10,0,0).trans cow.osg" );
    osg::Group* modelParent = new osg::Group;
    modelParent->addChild( loadedModels );
    root->addChild( modelParent );

    // DO NOT disable GL_LIGHT0. We have the sky dome turned off, which means there
    // is no Sun light source, so GL_LIGHT0 is the only light source by default.
    //modelParent->getOrCreateStateSet()->setMode( GL_LIGHT0, osg::StateAttribute::OFF );

    // Convert loaded data to use shader composition.
    if( useSMV )
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform

        osg::NotifySeverity ns = osg::getNotifyLevel();
        osg::setNotifyLevel( osg::INFO );

        backdropFX::convertFFPToShaderModules( root.get(), &smv );

        osg::setNotifyLevel( ns );
    }

    unsigned int width( 800 ), height( 600 );

    backdropFXSetUp( root.get(), width, height );


    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 20, 30, width, height );
    viewer.realize();

    viewerSetUp( viewer, width, height, root.get() );


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
    <td><b>-nosmv</b></td>
    <td>Force app to not use ShaderModuleVisitor. By default, the app uses the ShaderModuleVisitor to replace FFP with Uniforms and Programs.</td>
  </tr>
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
