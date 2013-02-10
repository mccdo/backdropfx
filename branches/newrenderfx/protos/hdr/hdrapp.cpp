// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/GUIEventHandler>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/FrameBufferObject>

#include <backdropFX/SkyDome.h>
#include <backdropFX/LocationData.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/Version.h>
#include <osgDB/FileUtils>


const float winW( 1920.f ), winH( 1200.f );
const float texW( 1920.f ), texH( 1200.f );


osg::ref_ptr< osg::Uniform > brightnessUniform;
osg::ref_ptr< osg::Uniform > contrastUniform;
float brightness, contrast;


class HDREventHandler : public osgGA::GUIEventHandler
{
public:

    HDREventHandler()
    {}
    
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& )
    {
        bool handled( false );
        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        {
            switch( ea.getKey() )
            {
            // 'b'/'B': decrease / increase brightness
            // 'c'/'C': decrease / increase contrast
            case 'b':
                brightness -= .05;
                handled = true;
                break;
            case 'B':
                brightness += .05;
                handled = true;
                break;

            case 'c':
                contrast -= .05;
                handled = true;
                break;
            case 'C':
                contrast += .05;
                handled = true;
                break;
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        if( handled )
        {
            osg::clampBetween<float>( brightness, -1.0, 1.0 );
            osg::clampBetween<float>( contrast, -1.0, 1.0 );
            brightnessUniform->set( brightness );
            contrastUniform->set( contrast );

            osg::notify( osg::ALWAYS ) << "Brightness: " << brightness <<
                "   Contrast: " << contrast << std::endl;
        }

        return( handled );
    }
};



osg::Geode*
createFullScreenQuad( osg::Texture2D* tex,
    const std::string& vertexShader, const std::string& fragmentShader )
{
    osg::Geometry* geom = new osg::Geometry;
    osg::Geode* geode = new osg::Geode;
    geode->setCullingActive( false );
    geode->addDrawable( geom );

    osg::Vec4Array* v = new osg::Vec4Array;
    v->push_back( osg::Vec4( -1., -1., 1., 1. ) );
    v->push_back( osg::Vec4( 1., -1., 1., 1. ) );
    v->push_back( osg::Vec4( -1., 1., 1., 1. ) );
    v->push_back( osg::Vec4( 1., 1., 1., 1. ) );
    geom->setVertexArray( v );

    osg::Vec2Array* tc = new osg::Vec2Array;
    tc->push_back( osg::Vec2( 0., 0. ) );
    tc->push_back( osg::Vec2( 1., 0. ) );
    tc->push_back( osg::Vec2( 0., 1. ) );
    tc->push_back( osg::Vec2( 1., 1. ) );
    geom->setTexCoordArray( 0, tc );

    osg::DrawElementsUShort* deus = new osg::DrawElementsUShort( GL_TRIANGLE_STRIP );
    deus->push_back( 0 );
    deus->push_back( 1 );
    deus->push_back( 2 );
    deus->push_back( 3 );

    geom->addPrimitiveSet( deus );

    {
        osg::StateSet* ss = geom->getOrCreateStateSet();

        // Turn on depth writes so that the fullscreen quad clears the depth buffer.
        osg::ref_ptr< osg::Depth > depth( new osg::Depth( osg::Depth::ALWAYS, 0.0, 1.0 ) );
        ss->setAttributeAndModes( depth.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        osg::ref_ptr< osg::CullFace > cf = new osg::CullFace;
        ss->setAttributeAndModes( cf.get(), osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED );

        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( vertexShader ) ) );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( fragmentShader ) ) );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        // Add texture to display
        ss->setTextureAttributeAndModes( 0, tex, osg::StateAttribute::ON );

        // Uniform for texture
        osg::ref_ptr< osg::Uniform > texUniform( new osg::Uniform( "tex", 0 ) );
        ss->addUniform( texUniform.get() );
    }

    return( geode );
}


#if !defined( OSG_GL3_AVAILABLE ) || !defined( GL_R11F_G11F_B10F )
#define GL_R11F_G11F_B10F 0x8C3A
#endif
#if !defined( OSG_GL3_AVAILABLE ) || !defined( GL_RGBA16F )
#define GL_RGBA16F 0x881A
#endif

#ifdef __APPLE__
#  define USE_FORMAT GL_RGBA
#else
#  define USE_FORMAT GL_R11F_G11F_B10F
#endif

osg::Node*
postRenderHDR( osgViewer::Viewer& viewer )
{
    osg::Camera* rootCamera( viewer.getCamera() );

    // Create the texture; we'll use this as our color buffer.
    // Note it has no image data; not required.
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureSize( texW, texH );
    tex->dirtyTextureObject();
    tex->setSourceFormat( GL_RGBA );
    tex->setInternalFormat( USE_FORMAT );
    tex->setBorderWidth( 0 );
    tex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    // Attach the texture to the camera.
    rootCamera->attach( osg::Camera::COLOR_BUFFER0, tex, 0, 0, false );
    rootCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER );
    rootCamera->setViewport( new osg::Viewport( 0, 0, texW, texH ) );


    // Configure postRenderCamera to draw fullscreen textured quad
    osg::ref_ptr< osg::Camera > postRenderCamera( new osg::Camera );
    postRenderCamera->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );
    postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    postRenderCamera->setViewport( new osg::Viewport( 0, 0, winW, winH ) );

    osg::Geode* geode( createFullScreenQuad( tex, "protohdr.vs", "protohdr.fs" ) );
    postRenderCamera->addChild( geode );
    {
        osg::StateSet* ss = geode->getOrCreateStateSet();
        ss->setRenderBinDetails( 5, "RenderBin", osg::StateSet::USE_RENDERBIN_DETAILS );

        // Set brightness and contrast uniforms.
        brightnessUniform = new osg::Uniform( "brightness", brightness = 0.0f );
        ss->addUniform( brightnessUniform.get() );

        contrastUniform = new osg::Uniform( "contrast", contrast = 0.0f );
        ss->addUniform( contrastUniform.get() );
    }

    return( postRenderCamera.release() );
}


osg::FrameBufferObject*
createFBO( osg::Texture2D* tex )
{
    osg::FrameBufferObject* fbo = new osg::FrameBufferObject;

    fbo->setAttachment( osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( tex ) );

    // Don't need depth buffer for sky dome

    return( fbo );
}


int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;

    osg::ref_ptr< backdropFX::SkyDome > skydome( new backdropFX::SkyDome );
    skydome->getOrCreateStateSet()->setRenderBinDetails( -1, "RenderBin", osg::StateSet::USE_RENDERBIN_DETAILS );
    //skydome->useTexture( "sky1.jpg" );
    skydome->setSunScale( 3. );
    skydome->setMoonScale( 3. );
    skydome->setDebugMode( backdropFX::BackdropCommon::debugImages );
    root->addChild( skydome.get() );

    if( true )
    {
        osg::ref_ptr< osg::Texture2D > tex = new osg::Texture2D;
        tex->setTextureSize( 1920, 1200 );
        tex->dirtyTextureObject();
        tex->setSourceFormat( GL_RGBA );
        tex->setInternalFormat( USE_FORMAT );
        tex->setBorderWidth( 0 );
        tex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
        tex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

        skydome->setFBO( createFBO( tex.get() ) );
        osg::Geode* geode( createFullScreenQuad( tex.get(), "protoskydomeapp.vs", "protoskydomeapp.fs" ) );
        geode->getOrCreateStateSet()->setRenderBinDetails( 0, "RenderBin", osg::StateSet::USE_RENDERBIN_DETAILS );
        root->addChild( geode );
    }

    osg::ref_ptr< osg::Node > lz( osgDB::readNodeFile( "lz.osg" ) );
    lz->getOrCreateStateSet()->setRenderBinDetails( 1, "RenderBin", osg::StateSet::USE_RENDERBIN_DETAILS );
    root->addChild( lz.get() );

    osgViewer::Viewer viewer;
    osgwTools::configureViewer( viewer );
//    viewer.setUpViewInWindow( 30, 30, winW, winH );
    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new HDREventHandler );
    // TBD SkyDome should probably set this directly in SkyDomeStage.
    viewer.getCamera()->setClearColor( osg::Vec4( 0., 0., 0., 1. ) );
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrixAsPerspective( 35., texW/texH, 1., 2000. );
    viewer.setSceneData( root.get() );

    root->addChild( postRenderHDR( viewer ) );

    // Probably not a hack, as we just draw a full screen quad
    // into the window at maximum z.
    //viewer.getCamera()->setClearMask( 0 );

    osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator();
    viewer.setCameraManipulator( tb );

    int hour( 12 ), min( 0 );
    osgEphemeris::DateTime dateTime( 2010, 1, 21, hour, min );
    backdropFX::LocationData::s_instance()->setDateTime( dateTime );
    backdropFX::LocationData::s_instance()->setLatitudeLongitude( 39.86, -104.68 );
    backdropFX::LocationData::s_instance()->setEastUp(
        osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 0., 1. ) );

    bool first( true );
    while( !viewer.done() )
    {
        if( ++min >= 300 )
        {
            min -= 300;
            dateTime.setHour( ++hour );
        }
        dateTime.setMinute( min/5.f );
        //backdropFX::LocationData::s_instance()->setDateTime( dateTime );

        viewer.frame();

        if( first )
        {
            tb->setNode( lz.get() );
            tb->home( 0 );
            first = false;
        }
    }
    return( 0 );
}
