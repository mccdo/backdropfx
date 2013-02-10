// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <backdropFX/SurfaceUtils.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/Utils.h>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/Image>
#include <osg/Texture2D>


namespace backdropFX
{



#define TEXUNIT_PERM        4
#define TEXUNIT_BUMP        5
#define TEXUNIT_DARK        6

// vertex attribute subscripts, not related to above
#define TANGENT_ATR_UNIT	6
#define BINORMAL_ATR_UNIT	7


void createConcrete( osg::Node* node )
{
    if( node == NULL )
        return;

    osg::StateSet* stateSet = node->getOrCreateStateSet();

    osg::Image* concreteDarkenImage = osgDB::readImageFile( "ConcreteDarken.png" );
    osg::Texture2D* darkenTexture = new osg::Texture2D( concreteDarkenImage );
    darkenTexture->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT );
    darkenTexture->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT );
    stateSet->setTextureAttribute( TEXUNIT_DARK, darkenTexture );

    osg::Image* concreteBumpImage   = osgDB::readImageFile( "ConcreteBump.png" );
    osg::Texture2D* bumpTexture = new osg::Texture2D( concreteBumpImage );
    bumpTexture->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT );
    bumpTexture->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT );
    stateSet->setTextureAttribute( TEXUNIT_BUMP, bumpTexture );

    osg::Image* permImage = osgDB::readImageFile( "permTexture.png" );
    osg::Texture2D* permTexture = new osg::Texture2D( permImage );
    permTexture->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT );
    permTexture->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT );
    stateSet->setTextureAttribute( TEXUNIT_PERM, permTexture );

    stateSet->addUniform( new osg::Uniform("baseMap", TEXUNIT_DARK) );
    stateSet->addUniform( new osg::Uniform("bumpMap", TEXUNIT_BUMP) );
    stateSet->addUniform( new osg::Uniform("permTexture", TEXUNIT_PERM) );

    stateSet->addUniform( new osg::Uniform("fSpecularPower", 25.0f) );
    stateSet->addUniform( new osg::Uniform("fScale", 16.0f) );

    stateSet->addUniform( new osg::Uniform( "fvLightPosition", osg::Vec3( -100.0f, 0.0f, 1000.0f ) ) );
    stateSet->addUniform( new osg::Uniform( "fvEyePosition", osg::Vec3( 0.0f, 0.0f, 1000.0f ) ) );

    stateSet->addUniform( new osg::Uniform("fvAmbient", osg::Vec4(0.3686f, 0.3686f, 0.3686f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvDiffuse", osg::Vec4(0.8863f, 0.8850f, 0.8850f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvSpecular", osg::Vec4(0.0157f, 0.0157f, 0.0157f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvBaseColorA", osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvBaseColorB", osg::Vec4(0.9882f, 0.9579f, 0.9158f, 1.0f)) );


    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *node );
    UTIL_MEMORY_CHECK( smccb, "SurfaceUtils createConcrete SMCCB", );

    osg::ref_ptr< osg::Shader > shader;
    std::string fileName( "shaders/gl2/surface-lighting.vs" );
    __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createConcrete surface-lighting.vs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/surface-surface-concrete.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createConcrete surface-surface-concrete.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/noise-noise.vs.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createConcrete noise-noise.vs.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/perpixel-lighting-on.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createConcrete perpixel-lighting-on.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/perpixel-normal-surface.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createConcrete perpixel-normal-surface.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );
}

void createDirt( osg::Node* node )
{
    if( node == NULL )
        return;

    osg::StateSet* stateSet = node->getOrCreateStateSet();

    osg::Image* permImage = osgDB::readImageFile( "permTexture.png" );
    osg::Texture2D* permTexture = new osg::Texture2D( permImage );
    permTexture->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT );
    permTexture->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT );
    stateSet->setTextureAttribute( TEXUNIT_PERM, permTexture );

    stateSet->addUniform( new osg::Uniform( "permTexture", TEXUNIT_PERM ) );

    stateSet->addUniform( new osg::Uniform("fSpecularPower", 25.0f) );
    stateSet->addUniform( new osg::Uniform("fScale", 10.f) );
    stateSet->addUniform( new osg::Uniform("fGravelScale", 8.0f) );
    stateSet->addUniform( new osg::Uniform("fDirtScale", 0.15f) );
    stateSet->addUniform( new osg::Uniform("fDirtGravelDistScale", 0.06f) );

    stateSet->addUniform( new osg::Uniform( "fvLightPosition", osg::Vec3( -100.0f, 0.0f, 1000.0f ) ) );
    stateSet->addUniform( new osg::Uniform( "fvEyePosition", osg::Vec3( 0.0f, 0.0f, 1000.0f ) ) );

    stateSet->addUniform( new osg::Uniform("fvAmbient", osg::Vec4(0.3686f, 0.3686f, 0.3686f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvDiffuse", osg::Vec4(0.8863f, 0.8850f, 0.8850f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvSpecular", osg::Vec4(0.0157f, 0.0157f, 0.0157f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvBaseColorA", osg::Vec4(0.7255f, 0.6471f, 0.3451f, 1.0f)) );
    stateSet->addUniform( new osg::Uniform("fvBaseColorB", osg::Vec4(0.6392f, 0.5412f, 0.2118f, 1.0f)) );


    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *node );
    UTIL_MEMORY_CHECK( smccb, "SurfaceUtils createDirt SMCCB", );

    osg::ref_ptr< osg::Shader > shader;
    std::string fileName( "shaders/gl2/surface-lighting.vs" );
    __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createDirt surface-lighting.vs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/surface-surface-dirt.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createDirt surface-surface-dirt.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/noise-noise.vs.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createDirt noise-noise.vs.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/perpixel-lighting-on.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createDirt perpixel-lighting-on.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/perpixel-normal-surface.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createDirt perpixel-normal-surface.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );
}

void createGrass( osg::Node* node )
{
    if( node == NULL )
        return;

    osg::StateSet* stateSet = node->getOrCreateStateSet();

    osg::Image* permImage = osgDB::readImageFile( "permTexture.png" );
    osg::Texture2D* permTexture = new osg::Texture2D( permImage );
    permTexture->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT );
    permTexture->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT );
    stateSet->setTextureAttribute( TEXUNIT_PERM, permTexture );

    stateSet->addUniform( new osg::Uniform( "permTexture", TEXUNIT_PERM) );

    stateSet->addUniform( new osg::Uniform( "fSpecularPower", 25.0f) );
    stateSet->addUniform( new osg::Uniform( "fScale", 10.f) );
    stateSet->addUniform( new osg::Uniform( "fMacroToMicroScale", -0.56f) );

    stateSet->addUniform( new osg::Uniform( "fvLightPosition", osg::Vec3( -100.0f, 0.0f, 1000.0f)) );
    stateSet->addUniform( new osg::Uniform( "fvEyePosition", osg::Vec3( 0.0f, 0.0f, 1000.0f)) );

    stateSet->addUniform( new osg::Uniform( "fvAmbient", osg::Vec4( 0.3686f, 0.3686f, 0.3686f, 1.0f ) ) );
    stateSet->addUniform( new osg::Uniform( "fvDiffuse", osg::Vec4( 0.8863f, 0.8850f, 0.8850f, 1.0f ) ) );
    stateSet->addUniform( new osg::Uniform( "fvSpecular", osg::Vec4( 0.0157f, 0.0157f, 0.0157f, 1.0f ) ) );
    stateSet->addUniform( new osg::Uniform( "fvBaseColorA", osg::Vec4( 0.1098f, 0.6692f, 0.1628f, 1.0f ) ) );
    stateSet->addUniform( new osg::Uniform( "fvBaseColorB", osg::Vec4( 0.0863f, 0.5564f, 0.1156f, 1.0f ) ) );


    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *node );
    UTIL_MEMORY_CHECK( smccb, "SurfaceUtils createGrass SMCCB", );

    osg::ref_ptr< osg::Shader > shader;
    std::string fileName( "shaders/gl2/surface-lighting.vs" );
    __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createGrass surface-lighting.vs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/surface-surface-grass.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createGrass surface-surface-grass.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/noise-noise.vs.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createGrass noise-noise.vs.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/perpixel-lighting-on.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createGrass perpixel-lighting-on.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/perpixel-normal-surface.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "SurfaceUtils createGrass perpixel-normal-surface.fs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );
}


// backdropFX
}
