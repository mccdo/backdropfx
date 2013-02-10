// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include "DefaultScene.h"
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/Node>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/Uniform>
#include <osg/PointSprite>
#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/Depth>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/Shapes.h>
#include <osgwTools/TransparencyUtils.h>
#include <osgwTools/Version.h>



DefaultScene::DefaultScene()
{
}
DefaultScene::~DefaultScene()
{
}

osg::StateSet*
DefaultScene::glowStateSet( osg::Group* grp )
{
    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *grp );

    osg::Shader* shader = new osg::Shader( osg::Shader::FRAGMENT );
    std::string fileName = "shaders/gl2/bdfx-finalize.fs";
    shader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader );

    osg::StateSet* ss = grp->getOrCreateStateSet();
    // TBD Not sure how we'll set the glow color at this time.
    osg::Vec4f glowColor( 0., .4, .6, 1. );
    osg::Uniform* glow = new osg::Uniform( "bdfx_glowColor", glowColor );
    ss->addUniform( glow );

    return( ss );
}
osg::StateSet*
DefaultScene::glowTransparentStateSet( osg::Group* grp )
{
    osgwTools::transparentEnable( grp, .2f );

    return( glowStateSet( grp ) );
}

osg::Geode*
DefaultScene::makePoints()
{
    osg::Vec3Array* vertices = new osg::Vec3Array;
    vertices->push_back( osg::Vec3( -0.2, 0.2, 0. ) );
    vertices->push_back( osg::Vec3( -0.2, -0.2, -0.05 ) );
    vertices->push_back( osg::Vec3( 0.2, 0., 0. ) );

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back( osg::Vec4( 1., 0., 0., 1. ) );
    colors->push_back( osg::Vec4( 1., 1., 1., 1. ) );
    colors->push_back( osg::Vec4( .1, .9, .2, 1. ) );

    osg::Geometry* geom = new osg::Geometry;
    geom->setVertexArray( vertices );
    geom->setColorArray( colors );
    geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );

    geom->addPrimitiveSet( new osg::DrawArrays( GL_POINTS, 0, 3 ) );

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable( geom );
    return( geode );
}
void
DefaultScene::pointShader( osg::Group* grp )
{
    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *grp );

    osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
    std::string fileName = "shaders/gl2/point-finalize.vs";
    shader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader );
}



osg::Geometry*
DefaultScene::makeStreamline( const float radius, const osg::BoundingBox& bound, int nInstances, const osg::Vec4& color )
{
    osg::ref_ptr< osg::Geometry > geom = new osg::Geometry;
    geom->setUseDisplayList( false );
    geom->setUseVertexBufferObjects( true );

    geom->setInitialBound( bound );

    // Configure a Geometry to draw a single tri pair, but use the draw instanced PrimitiveSet
    // to draw the tri pair multiple times.
    osg::Vec3Array* v = new osg::Vec3Array;
    v->resize( 4 );
    geom->setVertexArray( v );
    (*v)[ 0 ] = osg::Vec3( -radius, -radius, 0. );
    (*v)[ 1 ] = osg::Vec3( radius, -radius, 0. );
    (*v)[ 2 ] = osg::Vec3( -radius, radius, 0. );
    (*v)[ 3 ] = osg::Vec3( radius, radius, 0. );

    osg::Vec2Array* tc = new osg::Vec2Array;
    tc->resize( 4 );
    geom->setTexCoordArray( 1, tc );
    (*tc)[ 0 ] = osg::Vec2( 0., 0. );
    (*tc)[ 1 ] = osg::Vec2( 1., 0. );
    (*tc)[ 2 ] = osg::Vec2( 0., 1. );
    (*tc)[ 3 ] = osg::Vec2( 1., 1. );

    {
        // TBD don't really need this, colors will come from
        // instanced color array.
        osg::Vec4Array* colors = new osg::Vec4Array;
        colors->push_back( color );
        colors->push_back( color );
        colors->push_back( color );
        colors->push_back( color );
        geom->setColorArray( colors );
        geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
    }

#if (OSGWORKS_OSG_VERSION >= 20800 )
    geom->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLE_STRIP, 0, 4, nInstances ) );
#endif

    return( geom.release() );
}
osg::StateSet*
DefaultScene::streamlineStateSet( osg::Texture2D* texPos, int m, int n )
{
    osg::StateSet* ss = new osg::StateSet;
    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

    ss->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );

    // Specify the splotch texture.
    osg::Texture2D *tex = new osg::Texture2D();
    tex->setImage( osgDB::readImageFile( "splotch.png" ) );
    ss->setTextureAttributeAndModes( 1, tex, osg::StateAttribute::ON );

    osg::ref_ptr< osg::Uniform > splotchUniform =
        new osg::Uniform( "textureSplotch", 1 );
    ss->addUniform( splotchUniform.get() );

    // We want depth writes off for streamlines to work with glow pass.
    // But we want depth writes on for depth peeling, so DepthPeel state
    // will override this.
    osg::Depth* depth = new osg::Depth( osg::Depth::LESS, 0., 1., false );
    //ss->setAttributeAndModes( depth );


    // Uniforms used by the streamline vertex shader are:
    // vec2 sizes;
    // sampler2D texPos;
    // int totalInstances;
    // int numTraces;
    // float traceInterval;
    // int traceLength;

    osg::ref_ptr< osg::Uniform > sizesUniform =
        new osg::Uniform( "streamline.sizes", osg::Vec2( (float)m, (float)n ) );
    ss->addUniform( sizesUniform.get() );

    ss->setTextureAttributeAndModes( 0, texPos, osg::StateAttribute::ON );

    osg::ref_ptr< osg::Uniform > texPosUniform =
        new osg::Uniform( "streamline.positions", 0 );
    ss->addUniform( texPosUniform.get() );

    osg::ref_ptr< osg::Uniform > totalInstancesUniform =
        new osg::Uniform( "streamline.totalInstances", m * n );
    ss->addUniform( totalInstancesUniform.get() );

    // Specify the number of traces in the streamline set.
    osg::ref_ptr< osg::Uniform > numTracesUniform =
        new osg::Uniform( "streamline.numTraces", 2 );
    ss->addUniform( numTracesUniform.get() );

    // Specify the trace interval in seconds. This is the time interval
    // from a single sample point being the head of trace N, to being
    // the head of trace N+1.
    osg::ref_ptr< osg::Uniform > traceIntervalUniform =
        new osg::Uniform( "streamline.traceInterval", 2.f );
    ss->addUniform( traceIntervalUniform.get() );

    // Specify the trace length in number of sample points.
    // Alpha of rendered point fades linearly over the trace length.
    osg::ref_ptr< osg::Uniform > traceLengthUniform =
        new osg::Uniform( "streamline.traceLength", 14 );
    ss->addUniform( traceLengthUniform.get() );

    return( ss );
}
void
DefaultScene::streamlineShader( osg::Group* grp )
{
    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *grp );

    osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
    std::string fileName = std::string( "shaders/gl2/streamline-transform.vs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader );

    shader = new osg::Shader( osg::Shader::FRAGMENT );
    fileName = std::string( "shaders/gl2/streamline-texture.fs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader );

    shader = new osg::Shader( osg::Shader::FRAGMENT );
    fileName = std::string( "shaders/gl2/streamline-main.fs" );
    shader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader );
}
osg::Texture2D*
DefaultScene::streamlinePositionData( const osg::Vec3& loc, int& m, int& n, osg::BoundingBox& bound )
{
    m = 64;
    n = 1;


    float* pos = new float[ m * n * 3 ];
    float* posI = pos;
    const float x( loc.x() ), y( loc.y() ), z( loc.z() );

    const float dX = .125;
    int iIdx;
    for( iIdx = 0; iIdx < m*n; iIdx++ )
    {
        osg::Vec3 v( x + iIdx*dX, y, z + ( .75 * sin( (float)iIdx / (float)(m*n) * osg::PI * 2. ) ) );
        *posI++ = v.x();
        *posI++ = v.y();
        *posI++ = v.z();

        bound.expandBy( v );
    }

    osg::Image* iPos = new osg::Image;
    iPos->setImage( m, n, 1, GL_RGB32F_ARB, GL_RGB, GL_FLOAT,
        (unsigned char*) pos, osg::Image::USE_NEW_DELETE );
    osg::ref_ptr< osg::Texture2D > texPos = new osg::Texture2D( iPos );
    texPos->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST );
    texPos->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST );

    // TBD delete[] pos ?? memory leak?

    return( texPos.release() );
}


//#define SUPER_SIMPLE 1

osg::Node*
DefaultScene::operator()( osg::Node* node )
{
    if( node==NULL )
        return( internalDefaultScene() );

    osg::ref_ptr< osg::Group > grp = new osg::Group;
    grp->setName( "DefaultScene root" );

    grp->addChild( node );

    // TBD After ShaderModuleVisitor, run a visitor to reduce
    // number of uniforms?
    // TBD Run osgUtil::Optimizer for framerate issues?

#if 0
    osg::Node* teapot = osgDB::readNodeFile( "teapot.osg.(-5,0,0).trans" );
    // TBD OSG doesn't draw the teapots when the eye gets
    // close, need to investigate why.
    teapot->setCullingActive( false );
    osg::Group* teapotParent = new osg::Group;
    teapotParent->addChild( teapot );
    grp->addChild( teapotParent );
    {
        osg::StateSet* ss = teapotParent->getOrCreateStateSet();
        osg::Uniform* transparency = new osg::Uniform( "bdfx_depthPeelTransparency", .2f );
        ss->addUniform( transparency );
    }
#endif

    osg::Geode* geode = new osg::Geode;
    osg::Vec3 size( 7., 7., 3.5 );
    geode->addDrawable( osgwTools::makePlane(
        osg::Vec3( -size.x(), -size.y(), -size.z() ),
        osg::Vec3( size.x() * 2., 0., 0. ), osg::Vec3( 0., size.x() * 2., 0. ),
        osg::Vec2s( 10, 10 ) ) );
    grp->addChild( geode );

    return( grp.release() );
}

osg::Node*
DefaultScene::internalDefaultScene()
{
    osg::ref_ptr< osg::Group > grp = new osg::Group;
    grp->setName( "DefaultScene root" );

    osg::Node* teapot = osgDB::readNodeFile( "teapot.osg" );

    // TBD OSG doesn't draw the teapots when the eye gets
    // close, need to investigate why.
    teapot->setCullingActive( false );

#ifdef SUPER_SIMPLE
    osg::MatrixTransform* mt;
    mt = new osg::MatrixTransform( osg::Matrix::translate( 0, 0, 1 ) );
    mt->addChild( teapot );
    grp->addChild( mt );

    mt = new osg::MatrixTransform( osg::Matrix::translate( 0, 0, -1 ) );
    mt->addChild( teapot );
    osgwTools::transparentEnable( mt, .5f );
    grp->addChild( mt );
#else
    osg::MatrixTransform* mt;
    mt = new osg::MatrixTransform( osg::Matrix::translate( -2, 0, 1 ) );
    mt->setName( "Plain teapot xform" );
    mt->addChild( teapot );
    grp->addChild( mt );

    mt = new osg::MatrixTransform( osg::Matrix::translate( -2, 0, -1 ) );
    mt->setName( "transparent teapot xform" );
    mt->addChild( teapot );
    grp->addChild( mt );
    osgwTools::transparentEnable( mt, .2f );

    mt = new osg::MatrixTransform( osg::Matrix::translate( 0, 0, 1 ) );
    mt->setName( "glow teapot xform" );
    mt->addChild( teapot );
    mt->setStateSet( glowStateSet( mt ) );
    grp->addChild( mt );

    mt = new osg::MatrixTransform( osg::Matrix::translate( 0, 0, -1 ) );
    mt->setName( "glow-transparent teapot xform" );
    mt->addChild( teapot );
    mt->setStateSet( glowTransparentStateSet( mt ) );
    grp->addChild( mt );

#if 0
    // Point sprites not compatible with depth peeling.

    mt = new osg::MatrixTransform( osg::Matrix::translate( 2, 0, 1 ) );
    mt->addChild( makePoints() );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
        ss->setMode( GL_VERTEX_PROGRAM_POINT_SIZE, osg::StateAttribute::ON );
        ss->setMode( GL_POINT_SPRITE, osg::StateAttribute::ON );

        osg::Texture2D *tex = new osg::Texture2D();
        tex->setImage( osgDB::readImageFile( "splotch.png" ) );
        ss->setTextureAttributeAndModes( 0, tex, osg::StateAttribute::ON );
    }
    pointShader( mt );
    grp->addChild( mt );
#endif

#if( OSGWORKS_OSG_VERSION >= 20800 )
    mt = new osg::MatrixTransform( osg::Matrix::translate( 2, 0, -1 ) );
    mt->setName( "streamline xform" );
    osg::Geode* geode = new osg::Geode;
    geode->setName( "streamline geode" );
    grp->addChild( mt );
    mt->addChild( geode );

    streamlineShader( mt );

    int m, n;
    osg::BoundingBox bound;
    osg::ref_ptr< osg::Texture2D > texPos;
    osg::ref_ptr< osg::Geometry > geom;
    const float radius = .25f;
    osg::Vec3 loc = osg::Vec3( 0., 0.5, 0. );
    osg::Vec4 color = osg::Vec4( 1., .5, .5, 1. );

    texPos = streamlinePositionData( loc, m, n, bound );
    geom = makeStreamline( radius, bound, m*n, color );
    geom->setStateSet( streamlineStateSet( texPos, m, n ) );
    geode->addDrawable( geom.get() );

    loc = osg::Vec3( 0.25, -0.5, 0.1 );
    color = osg::Vec4( 0., .5, 1., 1. );

    texPos = streamlinePositionData( loc, m, n, bound );
    geom = makeStreamline( radius, bound, m*n, color );
    geom->setStateSet( streamlineStateSet( texPos, m, n ) );
    geode->addDrawable( geom.get() );
#endif

// SUPER_SIMPLE
#endif

    return( grp.release() );
}
