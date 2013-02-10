// Copyright (c) 2011 Skew Matrix Software LLC. All rights reserved.

#include <osg/Image>
#include <osg/Texture2D>
#include <osg/BlendFunc>
#include <osg/Program>
#include <osg/Geometry>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgViewer/Viewer>

#include <iostream>


//#define FULLSCREEN


int
main( int argc,
      char ** argv )
{
    osg::ref_ptr< osg::Image > noiseImage = osgDB::readImageFile( "noise.png" );
    if( noiseImage == NULL )
        std::cerr << "Can't fine noise.png." << std::endl;
    osg::ref_ptr< osg::Image > testImage = osgDB::readImageFile( "testdesert.png" );
    if( testImage == NULL )
        std::cerr << "Can't fine testdesert.png." << std::endl;
    if( ( noiseImage == NULL ) || ( testImage == NULL ) )
        return( 1 );

#ifdef FULLSCREEN
    osg::Geometry* geom = osg::createTexturedQuadGeometry( osg::Vec3( -1., -1., 0. ),
        osg::Vec3( 2., 0., 0. ), osg::Vec3( 0., 2., 0. ) );
#else
    osg::Geometry* geom = osg::createTexturedQuadGeometry( osg::Vec3( -1., 0., -1. ),
        osg::Vec3( 2., 0., 0. ), osg::Vec3( 0., 0., 2. ) );
#endif
    osg::ref_ptr< osg::Geode > geode = new osg::Geode;
    geode->addDrawable( geom );


    osg::StateSet* stateSet = geode->getOrCreateStateSet();

    osg::ref_ptr< osg::Texture2D > noiseTex = new osg::Texture2D( noiseImage.get() );
    noiseTex->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
    noiseTex->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );
    stateSet->setTextureAttributeAndModes( 0, noiseTex.get() );
    osg::Uniform* noiseUniform = new osg::Uniform( osg::Uniform::SAMPLER_2D, "noise" );
    noiseUniform->set( 0 );
    stateSet->addUniform( noiseUniform );

    osg::ref_ptr< osg::Texture2D > testTex = new osg::Texture2D( testImage.get() );
    testTex->setResizeNonPowerOfTwoHint( false );
    testTex->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
    testTex->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );
    stateSet->setTextureAttributeAndModes( 1, testTex.get() );
    osg::Uniform* testUniform = new osg::Uniform( osg::Uniform::SAMPLER_2D, "testTex" );
    testUniform->set( 1 );
    stateSet->addUniform( testUniform );

    osg::ref_ptr< osg::Shader > vShader = new osg::Shader( osg::Shader::VERTEX );
    vShader->setName( "proto-heat.vs" );
    vShader->loadShaderSourceFromFile( osgDB::findDataFile( vShader->getName() ) );
    osg::ref_ptr< osg::Shader > fShader = new osg::Shader( osg::Shader::FRAGMENT );
    fShader->setName( "proto-heat.fs" );
    fShader->loadShaderSourceFromFile( osgDB::findDataFile( fShader->getName() ) );
    osg::ref_ptr< osg::Program > program = new osg::Program;
    program->addShader( vShader.get() );
    program->addShader( fShader.get() );
    stateSet->setAttributeAndModes( program.get() );

    stateSet->setAttributeAndModes( new osg::BlendFunc() );


    osgViewer::Viewer viewer;
    viewer.setSceneData( geode.get() );
    viewer.getCamera()->setClearColor( osg::Vec4( .6, .6, .6, 1. ) );

#ifdef FULLSCREEN
    while( !viewer.done() )
        viewer.frame();

    return( 0 );
#else
    return( viewer.run() );
#endif
}

