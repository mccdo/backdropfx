// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>
#include <osgUtil/RenderBin>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <backdropFX/DepthPeelUtils.h>
#include <osgwTools/Shapes.h>
#include <osgwTools/ReadFile.h>

#include <backdropFX/Manager.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/Utils.h>


unsigned int winW( 800 ), winH( 600 );



osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, 5., 11. ) );
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
            _viewer.getCamera()->setViewport( new osg::Viewport( 0, 0, width, height ) );

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



osg::Node*
postRender( osgViewer::Viewer& viewer )
{
    osg::Camera* rootCamera( viewer.getCamera() );

    // Create the texture; we'll use this as our color buffer.
    // Note it has no image data; not required.
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureWidth( winW );
    tex->setTextureHeight( winH );
    tex->setInternalFormat( GL_RGBA );
    tex->setBorderWidth( 0 );
    tex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    // Attach the texture to the camera. Tell it to use multisampling.
    // Internally, OSG allocates a multisampled renderbuffer, renders to it,
    // and at the end of the frame performs a BlitFramebuffer into our texture.
    rootCamera->attach( osg::Camera::COLOR_BUFFER0, tex, 0, 0, false );
    rootCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER );
#if( OSGWORKS_OSG_VERSION >= 20906 )
    rootCamera->setImplicitBufferAttachmentMask(
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT );
#endif


    // Configure postRenderCamera to draw fullscreen textured quad
    osg::ref_ptr< osg::Camera > postRenderCamera( new osg::Camera );
    postRenderCamera->setClearColor( osg::Vec4( 0.25, 0., 0., 1. ) ); // border color
    postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );

    postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    postRenderCamera->setViewMatrix( osg::Matrixd::identity() );
    postRenderCamera->setProjectionMatrix( osg::Matrixd::identity() );

    osg::Geode* geode( new osg::Geode );
    geode->addDrawable( osg::createTexturedQuadGeometry(
        osg::Vec3( -0.9,-0.9,0 ), osg::Vec3( 1.8,0,0 ), osg::Vec3( 0,1.8,0 ) ) );
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, tex, osg::StateAttribute::ON );
    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->getOrCreateStateSet()->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    // Use FFP:
    geode->getOrCreateStateSet()->setAttribute( new osg::Program(), osg::StateAttribute::OFF );

    postRenderCamera->addChild( geode );

    return( postRenderCamera.release() );
}

void
viewerSetUp( osgViewer::Viewer& viewer, unsigned int width, unsigned int height, osg::Node* node )
{
    double aspect = (double)width / (double)height;

    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.setSceneData( node );

    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
    viewer.addEventHandler( new ResizeHandler( viewer, width, height ) );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

osg::Node* makeSphere( const osg::BoundingSphere& bs )
{
    osg::ref_ptr< osg::Geode > geode = new osg::Geode;
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    stateSet->setAttributeAndModes( new osg::BlendFunc() );

    osg::Geometry* geom = osgwTools::makeGeodesicSphere( bs._radius, 2 );
    osg::Vec4Array* c = new osg::Vec4Array;
    c->push_back( osg::Vec4( 1., .5, .1, 0.25 ) );
    geom->setColorArray( c );
    geom->setColorBinding( osg::Geometry::BIND_OVERALL );
    geode->addDrawable( geom );

    geom = osgwTools::makePlane( bs._center,
        osg::Vec3( bs._radius * 1.2, 0., 0. ), osg::Vec3( 0., 0., bs._radius * 1.2 ) );
    c = new osg::Vec4Array;
    c->push_back( osg::Vec4( .0, .1, .4, 0.25 ) );
    geom->setColorArray( c );
    geom->setColorBinding( osg::Geometry::BIND_OVERALL );
    geode->addDrawable( geom );

    return( geode.release() );
}


int
main( int argc, char ** argv )
{
    //backdropFX::Manager::instance()->setDebugMode( backdropFX::BackdropCommon::debugImages );

    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "newdepthpeel root" );

    osg::ref_ptr< osg::Group > depthpeel = new osg::Group;
    depthpeel->setName( "DepthPeel" );
    backdropFX::configureAsDepthPeel( depthpeel.get() );
    root->addChild( depthpeel.get() );

    osg::ArgumentParser arguments( &argc, argv );
    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(10,0,0).trans cow.osg" );
    depthpeel->addChild( loadedModels );

    depthpeel->addChild( makeSphere( loadedModels->getBound() ) );

    // Convert loaded data to use shader composition.
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( true );
        smv.setAttachTransform( true );
        smv.setSupportSunLighting( false );
        root->accept( smv );

        backdropFX::RebuildShaderModules rsm;
        root->accept( rsm );
    }

    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 20, 30, winW, winH );
    viewer.realize();

    viewerSetUp( viewer, winW, winH, root.get() );
    osgDB::writeNodeFile( *root, "out.osg" );
    viewer.getCamera()->setClearColor( osg::Vec4( .1, .2, .5, 1. ) );

    root->addChild( postRender( viewer ) );


    return( viewer.run() );
}
