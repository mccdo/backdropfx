
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
#include <backdropFX/SurfaceUtils.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/ReadFile.h>
#include <osgwTools/GeometryModifier.h>
#include <osgwTools/TangentSpaceOp.h>



osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, .01, 100000. ) );
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
    osg::ref_ptr< osg::Fog > fog = new osg::Fog();
    fog->setMode( osg::Fog::EXP );
	fog->setColor( osg::Vec4( 0., 0.1, 0.2, 1.0 ) );
    fog->setDensity( 0.001f );
    backdropFX::Manager::instance()->setFogState( fog.get() );

    // TBD Until bdfx support multiple shadows/lights, set light 0.
    osg::ref_ptr< osg::Light > light = new osg::Light;
    light->setLightNum( 0 );
    light->setPosition( osg::Vec4( 1.1, -30.0, 30.0, 1.0 ) );
    backdropFX::Manager::instance()->setLight( light.get() );

    //backdropFX::Manager::instance()->setDebugMode( backdropFX::BackdropCommon::debugShaders );

    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild(
        backdropFX::Manager::shadowMap );

    // With a reasonably large FOV in the window, the sun and moon typically
    // appear too small. Scale them up to make them more visible.
    backdropFX::SkyDome& sd = backdropFX::Manager::instance()->getSkyDome();
    sd.setSunScale( 2. );
    sd.setMoonScale( 2. );

    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void
viewerSetUp( osgViewer::Viewer& viewer, unsigned int width, unsigned int height, osg::Node* node )
{
    double aspect = (double)width / (double)height;

    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
    viewer.addEventHandler( new ResizeHandler( viewer, width, height ) );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

void prepSurface( osg::Node* node )
{
    osgwTools::GeometryModifier gm( new osgwTools::TangentSpaceOp( 0 ) );
    node->accept( gm );
}

int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "simple example root" );

    osg::ArgumentParser arguments( &argc, argv );
    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "dumptruck.osg.(0,0,7).trans" );
    osg::Group* modelParent = new osg::Group;
    modelParent->addChild( loadedModels );
    root->addChild( modelParent );

    osg::ref_ptr< osg::Node > ground = osgDB::readNodeFile( "lzground.osg" );
    prepSurface( ground.get() );
    backdropFX::createDirt( ground.get() );
    modelParent->addChild( ground.get() );

    // Convert loaded data to use shader composition.
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        backdropFX::convertFFPToShaderModules( root.get(), &smv );
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
