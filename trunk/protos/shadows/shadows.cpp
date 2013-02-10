// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventHandler>

#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/PolygonOffset>
#include <osgwTools/Shapes.h>


osg::ref_ptr< osg::Texture2D > _depthTex( NULL );
osg::ref_ptr< osg::Camera > _depthCam( NULL );
osg::ref_ptr< osg::Uniform > _shadowViewProj;
osg::ref_ptr< osg::Uniform > _displayTC;
osg::Vec4 lightPos( 10., -10., 20., 1. );


class MyEventHandler : public osgGA::GUIEventHandler
{
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        if( ea.getEventType() == osgGA::GUIEventAdapter::FRAME )
        {
            osg::Vec3 pos = osg::Vec3( lightPos[0], lightPos[1], lightPos[2] );
            osg::Matrixf view = osg::Matrixf::lookAt( pos, osg::Vec3( 0., 0., 0. ), osg::Vec3( 0., 0., 1. ) );
            osg::Matrixf proj = osg::Matrixf::perspective( 50., 1., 10., 80. );
            _shadowViewProj->set( view * proj );
            return( true );
        }

        else if( ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN )
        {
            switch( ea.getKey() )
            {
            case 'd':
            {
                bool v;
                _displayTC->get( v );
                _displayTC->set( !v );
                return( true );
            }
            case 'l':
            {
                lightPos[ 0 ] -= 1.;
                return( true );
            }
            case 'L':
            {
                lightPos[ 0 ] += 1.;
                return( true );
            }
            case 'k':
            {
                lightPos[ 1 ] -= 1.;
                return( true );
            }
            case 'K':
            {
                lightPos[ 1 ] += 1.;
                return( true );
            }
            }
        }

        return( false );
    }
};


osg::StateSet* createDepthMapState()
{
    osg::ref_ptr< osg::StateSet > stateSet = new osg::StateSet;

    osg::PolygonOffset* po = new osg::PolygonOffset( 2., 8. );
    stateSet->setAttributeAndModes( po );

    osg::Program* program = new osg::Program;
    osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
    shader->setName( "protoCreateDepthMap.vs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shader->getName() ) );
    program->addShader( shader );
    shader = new osg::Shader( osg::Shader::FRAGMENT );
    shader->setName( "protoCreateDepthMap.fs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shader->getName() ) );
    program->addShader( shader );
    stateSet->setAttribute( program );

    stateSet->addUniform( _shadowViewProj.get() );

    return( stateSet.release() );
}
osg::StateSet* createFinalSceneState()
{
    osg::ref_ptr< osg::StateSet > stateSet = new osg::StateSet;

    osg::Program* program = new osg::Program;
    osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
    shader->setName( "protoDepthMapShadow.vs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shader->getName() ) );
    program->addShader( shader );
    shader = new osg::Shader( osg::Shader::FRAGMENT );
    shader->setName( "protoDepthMapShadow.fs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shader->getName() ) );
    program->addShader( shader );
    stateSet->setAttribute( program );

    stateSet->setTextureAttributeAndModes( 7, _depthTex.get() );
    osg::Uniform* depthTexUniform = new osg::Uniform( osg::Uniform::SAMPLER_2D_SHADOW, "depthTex" );
    depthTexUniform->set( 7 );
    stateSet->addUniform( depthTexUniform );

    stateSet->addUniform( _shadowViewProj.get() );
    stateSet->addUniform( _displayTC.get() );

    return( stateSet.release() );
}

void makeDepthCamera()
{
    unsigned int dim( 1024 );

    if( !_depthTex.valid() )
        _depthTex = new osg::Texture2D;
    _depthTex->setName( "Shadow Depth Map" );
    _depthTex->setInternalFormat( GL_DEPTH_COMPONENT );
    _depthTex->setShadowComparison( true );
    _depthTex->setShadowCompareFunc( osg::Texture::LEQUAL ); // if in R <= texR, alpha set to 1.0
    _depthTex->setShadowTextureMode( osg::Texture::ALPHA );
    _depthTex->setBorderWidth( 0 );
    _depthTex->setTextureSize( dim, dim );
    _depthTex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    _depthTex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    osg::Texture2D* scratchTex = new osg::Texture2D;
    scratchTex->setName( "Scratch color buffer" );
    scratchTex->setInternalFormat( GL_RGBA );
    scratchTex->setBorderWidth( 0 );
    scratchTex->setTextureSize( dim, dim );
    scratchTex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    scratchTex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    if( !_depthCam.valid() )
        _depthCam = new osg::Camera;
    _depthCam->setName( "_depthCam" );
    _depthCam->attach( osg::Camera::COLOR_BUFFER0, scratchTex );
    _depthCam->attach( osg::Camera::DEPTH_BUFFER, _depthTex.get() );
    _depthCam->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER );
#if( OSGWORKS_OSG_VERSION >= 20906 )
    _depthCam->setImplicitBufferAttachmentMask(
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT );
#endif
    //_depthCam->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
    //_depthCam->setClearMask( GL_DEPTH_BUFFER_BIT );
    _depthCam->setRenderOrder( osg::Camera::PRE_RENDER );
    //_depthCam->setViewMatrix( osg::Matrixd::identity() );
    //_depthCam->setProjectionMatrix( osg::Matrixd::identity() );
    _depthCam->setViewport( 0, 0, dim, dim );

    if( !_shadowViewProj.valid() )
        _shadowViewProj = new osg::Uniform( osg::Uniform::FLOAT_MAT4, "shadowViewProj" );
    if( !_displayTC.valid() )
        _displayTC = new osg::Uniform( "displayTC", false );
}

osg::Node* makeGround()
{
    const float xVal( 20.f ), yVal( 20.f ), zVal( -5.f );
    osg::Geometry* geom = osgwTools::makePlane( osg::Vec3( -xVal, -yVal, zVal ),
        osg::Vec3( 2.f*xVal, 0., 0. ), osg::Vec3( 0., 2.f*yVal, 0. ) );

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable( geom );

    return( geode );
}

int main( int argc, char** argv )
{
    osg::notify( osg::ALWAYS ) << "l/L\tMove light in x." << std::endl;
    osg::notify( osg::ALWAYS ) << "k/K\tMove light in y." << std::endl;
    osg::notify( osg::ALWAYS ) << "d\tToggle tex coord visualization." << std::endl;

    osg::ArgumentParser arguments( &argc, argv );

    osg::ref_ptr< osg::Group > sceneRoot = new osg::Group;
    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgDB::readNodeFile( "cow.osg" );
    sceneRoot->addChild( loadedModels );
    sceneRoot->addChild( makeGround() );

    makeDepthCamera();

    osg::ref_ptr< osg::Group > root = new osg::Group;
    osg::ref_ptr< osg::Group > createDepthMap = new osg::Group;
    osg::ref_ptr< osg::Group > renderFinalScene = new osg::Group;
    createDepthMap->setStateSet( createDepthMapState() );
    renderFinalScene->setStateSet( createFinalSceneState() );
    root->addChild( createDepthMap.get() );
    root->addChild( renderFinalScene.get() );

    createDepthMap->addChild( _depthCam.get() );
    _depthCam->addChild( sceneRoot.get() );

    renderFinalScene->addChild( sceneRoot.get() );

    osgViewer::Viewer viewer;
    viewer.addEventHandler( new MyEventHandler );
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.setUpViewInWindow( 30, 30, 1024, 640 );
    viewer.setSceneData( root.get() );
    return( viewer.run() );
}
