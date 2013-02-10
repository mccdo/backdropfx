// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <osg/Node>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgwTools/Version.h>
#include <osgwTools/Shapes.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <string>
#include <sstream>


const int winW( 800 ), winH( 600 );


// Borrowed from ShaderLibrary.cpp
osg::Program*
myCreateEffectProgram( const std::string& baseName )
{
    std::string fileName;
    const std::string namePrefix( "shaders/effects/" );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".vs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > vertShader = new osg::Shader( osg::Shader::VERTEX );
    vertShader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( vertShader.get() );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".fs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > fragShader = new osg::Shader( osg::Shader::FRAGMENT );
    fragShader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( fragShader.get() );

    osg::ref_ptr< osg::Program > program = new osg::Program();
    program->addShader( vertShader.get() );
    program->addShader( fragShader.get() );
    return( program.release() );
}

osg::Program*
createPerfProgram( const std::string& baseName )
{
    std::string fileName;
    const std::string namePrefix( "shaders/perftest/" );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".vs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > vertShader = new osg::Shader( osg::Shader::VERTEX );
    vertShader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".fs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > fragShader = new osg::Shader( osg::Shader::FRAGMENT );
    fragShader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );

    osg::ref_ptr< osg::Program > program = new osg::Program();
    program->addShader( vertShader.get() );
    program->addShader( fragShader.get() );
    return( program.release() );
}


osg::Node*
postRender( osgViewer::Viewer& viewer )
{
    osg::Camera* rootCamera( viewer.getCamera() );

    // Create the texture; we'll use this as our color buffer.
    // Note it has no image data; not required.
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setTextureSize( winW, winH );
    tex->dirtyTextureObject();
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
    postRenderCamera->setClearColor( osg::Vec4( 0., 1., 0., 1. ) ); // should never see this.
    postRenderCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER, osg::Camera::FRAME_BUFFER );

    postRenderCamera->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
    postRenderCamera->setRenderOrder( osg::Camera::POST_RENDER );
    postRenderCamera->setViewMatrix( osg::Matrixd::identity() );
    postRenderCamera->setProjectionMatrix( osg::Matrixd::identity() );

    osg::Geode* geode( new osg::Geode );
    geode->addDrawable( osgwTools::makePlane(
        osg::Vec3( -1,-1,0 ), osg::Vec3( 2,0,0 ), osg::Vec3( 0,2,0 ) ) );
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, tex, osg::StateAttribute::ON );
    geode->getOrCreateStateSet()->setAttribute(
        myCreateEffectProgram( "none" ) );
    geode->getOrCreateStateSet()->addUniform( new osg::Uniform(
        "inputTextures[0]", 0 ) );

    postRenderCamera->addChild( geode );

    return( postRenderCamera.release() );
}

int
main( int argc, char** argv )
{
    osg::ArgumentParser arguments( &argc, argv );

    bool plainMode( false );
    osg::notify( osg::NOTICE ) << "  -p\tPlain mode. Runs with FFP and no RTT. Disabled by default (emulate BDFX with shaders and RTT)." << std::endl;
    if( arguments.read( "-p" ) )
    {
        plainMode = true;
        osg::notify( osg::INFO ) << "-p (plain mode)" << std::endl;
    }

    osg::ref_ptr< osg::Group > root( new osg::Group );
    root->addChild( osgDB::readNodeFiles( arguments ) );
    if( root->getNumChildren() == 0 )
        return( 1 );

    osgViewer::Viewer viewer;
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.setUpViewInWindow( 30, 30, winW, winH );
    viewer.setSceneData( root.get() );
    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.realize();

    if( !plainMode )
    {
        root->addChild( postRender( viewer ) );
        root->getOrCreateStateSet()->setAttribute( createPerfProgram( "dumb" ) );
    }

    return( viewer.run() );
}
