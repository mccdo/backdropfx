// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/EffectLibrary.h>
#include <backdropFX/Effect.h>
#include <backdropFX/RenderingEffects.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <osgDB/FileUtils>
#include <osg/Program>
#include <osgwTools/Version.h>
#include <osgwTools/FBOUtils.h>
#include <backdropFX/Utils.h>

#include <string>
#include <sstream>



namespace backdropFX
{


osg::Program*
createEffectProgram( const std::string& baseName )
{
    std::string fileName;
    const std::string namePrefix( "shaders/effects/" );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".vs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > vertShader;
    __LOAD_SHADER( vertShader, osg::Shader::VERTEX, fileName );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".fs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > fragShader;
    __LOAD_SHADER( fragShader, osg::Shader::FRAGMENT, fileName );

    osg::ref_ptr< osg::Program > program = new osg::Program();
    program->setName( "Effect " + baseName );
    program->addShader( vertShader.get() );
    program->addShader( fragShader.get() );
    return( program.release() );
}




EffectGlow::EffectGlow()
{
    _horizontal = createEffectProgram( "glow0a" );
    _vertical = createEffectProgram( "glow0b" );
    _combine = createEffectProgram( "glow1" );

    _hBlur = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _hBlur.get(), "EffectGlow _hBlur", )
    // TBD can't hard code this size.
    _hBlur->setTextureSize( 800, 600 );
    _hBlur->dirtyTextureObject();
    _hBlur->setInternalFormat( GL_RGBA );
    _hBlur->setBorderWidth( 0 );
    _hBlur->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _hBlur->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    _vBlur = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _vBlur.get(), "EffectGlow _vBlur", )
    // TBD can't hard code this size.
    _vBlur->setTextureSize( 800, 600 );
    _vBlur->dirtyTextureObject();
    _vBlur->setInternalFormat( GL_RGBA );
    _vBlur->setBorderWidth( 0 );
    _vBlur->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _vBlur->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    _hBlurFBO = _vBlurFBO = NULL;
}
EffectGlow::EffectGlow( const EffectGlow& rhs, const osg::CopyOp& copyop )
  : Effect( rhs )
{
}
EffectGlow::~EffectGlow()
{
}

void
EffectGlow::draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo )
{
    osg::State& state = *( renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );
    RenderingEffects* renderingEffects = rfxs->getRenderingEffects();

    const osg::Viewport* vp = dynamic_cast< const osg::Viewport* >(
        state.getLastAppliedAttribute( osg::StateAttribute::VIEWPORT ) );

    if( !_hBlurFBO.valid() )
    {
        _hBlurFBO = new osg::FrameBufferObject;
        UTIL_MEMORY_CHECK( _hBlurFBO.get(), "EffectGlow _hBlurFBO", )
        _hBlurFBO->setAttachment( osg::Camera::COLOR_BUFFER0,
            osg::FrameBufferAttachment( _hBlur.get() ) );
    }
    if( !_vBlurFBO.valid() )
    {
        _vBlurFBO = new osg::FrameBufferObject;
        UTIL_MEMORY_CHECK( _vBlurFBO.get(), "EffectGlow _vBlurFBO", )
        _vBlurFBO->setAttachment( osg::Camera::COLOR_BUFFER0,
            osg::FrameBufferAttachment( _vBlur.get() ) );
    }

    // Step 1
    // Render input1 glow map into hBlur.
    _hBlurFBO->apply( state );
    UTIL_GL_FBO_ERROR_CHECK( "EffectGlow _hBlurFBO", fboExt );
    vp->apply( state );

    state.applyAttribute( _horizontal.get() );
    _textureUniform->apply( gl2Ext, state.getUniformLocation( _textureUniform->getName().c_str() ) );

    IntTextureMap::const_iterator inItr = _inputs.find( 1 );
    state.setActiveTextureUnit( 0 );
    osg::Texture* texture = inItr->second.get();
    state.applyTextureAttribute( 0, texture );

    internalDraw( renderInfo );


    // Step 2
    // Render hBlur into vBlur.
    _vBlurFBO->apply( state );
    UTIL_GL_FBO_ERROR_CHECK( "EffectGlow _vBlurFBO", fboExt );
    vp->apply( state );

    state.applyAttribute( _vertical.get() );
    _textureUniform->apply( gl2Ext, state.getUniformLocation( _textureUniform->getName().c_str() ) );

    state.setActiveTextureUnit( 0 );
    state.applyTextureAttribute( 0, _hBlur.get() );

    internalDraw( renderInfo );

    if( ( renderingEffects->getDebugMode() & backdropFX::BackdropCommon::debugImages ) != 0 )
        dumpImage( rfxs->getViewport(), renderingEffects->debugImageBaseFileName( contextID ) );


    // Step 3
    // Render combine stage, with vBlur and input0, into _output.

    // Bind the output FBO.
    osg::FrameBufferObject* fbo;
    if( _output.valid() )
    {
        osg::notify( osg::INFO ) << "Effect: applying local Effect FBO." << std::endl;

        // We have an FBO assigned, use it.
        fbo = _output.get();
    }
    else
    {
        osg::notify( osg::INFO ) << "Effect: applying RenderingEffects FBO." << std::endl;

        // Use RenderingEffects output
        fbo = renderingEffects->getFBO();
    }
    if( fbo != NULL )
    {
        fbo->apply( state );
    }
    else
    {
        // Bind the default framebuffer.
        osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, 0 );
    }
    UTIL_GL_FBO_ERROR_CHECK( "Effect::draw", fboExt )
 
    vp->apply( state );
 

    // Bind the combine program (glow1).
    state.applyAttribute( _combine.get() );
    if( !_inputs.empty() )
    {
        _textureUniform->apply( gl2Ext, state.getUniformLocation( _textureUniform->getName().c_str() ) );
    }

    // Bind the input textures.
    inItr = _inputs.find( 0 );
    state.setActiveTextureUnit( 0 );
    texture = inItr->second.get();
    state.applyTextureAttribute ( 0, texture );

    state.setActiveTextureUnit( 1 );
    state.applyTextureAttribute( 1, _vBlur );

    UTIL_GL_ERROR_CHECK( "Effect::draw()." ) \

    // Draw.
    internalDraw( renderInfo );
}

void
EffectGlow::setTextureWidthHeight( unsigned int texW, unsigned int texH )
{
    Effect::setTextureWidthHeight( texW, texH );

    _hBlur->setTextureSize( texW, texH );
    _hBlur->dirtyTextureObject();

    _vBlur->setTextureSize( texW, texH );
    _vBlur->dirtyTextureObject();

    // Force rebuild of FBOs.
    _hBlurFBO = _vBlurFBO = NULL;
}


// namespace backdropFX
}
