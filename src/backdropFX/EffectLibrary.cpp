// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/EffectLibrary.h>
#include <backdropFX/Effect.h>
#include <backdropFX/RenderingEffects.h>
#include <backdropFX/RTTViewport.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <osgDB/FileUtils>
#include <osg/Program>
#include <osgwTools/Version.h>
#include <osgwTools/FBOUtils.h>
#include <backdropFX/Utils.h>

#include <string>
#include <sstream>



namespace backdropFX
{



EffectSeperableBlur::EffectSeperableBlur()
  : Effect()
{
    _horizontal = createEffectProgram( "blurHorizontal" );
    _vertical = createEffectProgram( "blurVertical" );

    _hBlur = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _hBlur.get(), "EffectSeperableBlur _hBlur", )
    // TBD can't hard code this size.
    _hBlur->setTextureSize( 800, 600 );
    _hBlur->dirtyTextureObject();
    _hBlur->setInternalFormat( GL_RGBA );
    _hBlur->setBorderWidth( 0 );
    _hBlur->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    _hBlur->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    _hBlur->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _hBlur->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    _hBlurFBO = NULL;
}
EffectSeperableBlur::EffectSeperableBlur( const EffectSeperableBlur& rhs, const osg::CopyOp& copyop )
  : Effect( rhs, copyop ),
    _horizontal( rhs._horizontal ),
    _vertical( rhs._vertical ),
    _hBlur( rhs._hBlur ),
    _hBlurFBO( rhs._hBlurFBO )
{
    osg::notify( osg::NOTICE ) << "BDFX: EffectSeperableBlur: Unexcepted copy operator invocation." << std::endl;
}
EffectSeperableBlur::~EffectSeperableBlur()
{
}

void
EffectSeperableBlur::draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo, bool last )
{
    osg::State& state = *( renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );
    RenderingEffects* renderingEffects = rfxs->getRenderingEffects();

    if( !_hBlurFBO.valid() )
    {
        _hBlurFBO = new osg::FrameBufferObject;
        UTIL_MEMORY_CHECK( _hBlurFBO.get(), "EffectSeperableBlur _hBlurFBO", )
        _hBlurFBO->setAttachment( osg::Camera::COLOR_BUFFER0,
            osg::FrameBufferAttachment( _hBlur.get() ) );
    }

    // Step 1
    // Render input texture into hBlur.
    _hBlurFBO->apply( state );
    UTIL_GL_FBO_ERROR_CHECK( "EffectSeperableBlur _hBlurFBO", fboExt );

    state.applyAttribute( _horizontal.get() );
    osg::Uniform* uniform = _textureUniform[ 0 ].get();
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName().c_str() ) );
    {
        osg::Uniform* uniform = rfxs->getTexturePercentUniform();
        GLint location = state.getUniformLocation( uniform->getName() );
        if( location >= 0 )
            uniform->apply( gl2Ext, location );
    }

    IntTextureMap::const_iterator inItr = _inputs.find( 0 );
    state.setActiveTextureUnit( 0 );
    osg::Texture* texture = inItr->second.get();
    state.applyTextureAttribute( 0, texture );

    internalDraw( renderInfo );


    // Step 2
    // Render hBlur into vBlur.

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
    UTIL_GL_FBO_ERROR_CHECK( "Effect::draw", fboExt );
 
    // If the 'last' flag is set, apply the full viewport.
    backdropFX::RTTViewport* rttvp = last ? dynamic_cast< backdropFX::RTTViewport* >( rfxs->getViewport() ) : NULL;
    if( rttvp != NULL )
        rttvp->applyFullViewport( state );
    else
        state.applyAttribute( rfxs->getViewport() );


    // OK, do the blur now...
    state.applyAttribute( _vertical.get() );
    uniform = _textureUniform[ 0 ].get();
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName().c_str() ) );
    {
        osg::Uniform* uniform = rfxs->getTexturePercentUniform();
        GLint location = state.getUniformLocation( uniform->getName() );
        if( location >= 0 )
            uniform->apply( gl2Ext, location );
    }

    state.setActiveTextureUnit( 0 );
    state.applyTextureAttribute( 0, _hBlur.get() );

    internalDraw( renderInfo );

    if( ( renderingEffects->getDebugMode() & backdropFX::BackdropCommon::debugImages ) != 0 )
    {
        const osg::Viewport* vp = rfxs->getViewport();
        dumpImage( vp, renderingEffects->debugImageBaseFileName( contextID ) );
    }


    if( rttvp != NULL )
        // State doesn't know we changed the viewport. Reset it the way it was.
        rttvp->apply( state );
}

void
EffectSeperableBlur::setTextureWidthHeight( unsigned int texW, unsigned int texH )
{
    Effect::setTextureWidthHeight( texW, texH );

    _hBlur->setTextureSize( texW, texH );
    _hBlur->dirtyTextureObject();

    // Force rebuild of FBOs.
    _hBlurFBO = NULL;
}



EffectCombine::EffectCombine()
  : Effect()
{
    osg::Program* program = backdropFX::createEffectProgram( "combine" );
    setProgram( program );
}
EffectCombine::EffectCombine( const EffectCombine& rhs, const osg::CopyOp& copyop )
  : Effect( rhs, copyop )
{
    osg::notify( osg::NOTICE ) << "BDFX: EffectCombine: Unexcepted copy operator invocation." << std::endl;
}
EffectCombine::~EffectCombine()
{
}



EffectAlphaBlend::EffectAlphaBlend()
  : Effect()
{
    osg::Program* program = backdropFX::createEffectProgram( "alphaBlend" );
    setProgram( program );
}
EffectAlphaBlend::EffectAlphaBlend( const EffectAlphaBlend& rhs, const osg::CopyOp& copyop )
  : Effect( rhs, copyop )
{
    osg::notify( osg::NOTICE ) << "BDFX: EffectAlphaBlend: Unexcepted copy operator invocation." << std::endl;
}
EffectAlphaBlend::~EffectAlphaBlend()
{
}



EffectScaleBias::EffectScaleBias()
  : _scaleBias( osg::Vec2f( 1.f, 0.f ) )
{
    osg::Uniform* scaleBiasUniform = new osg::Uniform( osg::Uniform::FLOAT_VEC2, "scaleBias" );
    scaleBiasUniform->set( _scaleBias );
    _uniforms.push_back( scaleBiasUniform );
}
EffectScaleBias::EffectScaleBias( const EffectScaleBias& rhs, const osg::CopyOp& copyop )
  : Effect( rhs, copyop ),
    _scaleBias( rhs._scaleBias )
{
}
EffectScaleBias::~EffectScaleBias()
{}

void EffectScaleBias::setScaleBias( const osg::Vec2f& scaleBias )
{
    _scaleBias = scaleBias;

    UniformVector::iterator itr;
    for( itr = _uniforms.begin(); itr != _uniforms.end(); itr++ )
    {
        if( (*itr)->getName() == std::string( "scaleBias" ) )
            break;
    }
    if( itr == _uniforms.end() )
    {
        osg::notify( osg::WARN ) << "BDFX: EffectScaleBias can't find uniform." << std::endl;
        return;
    }
    (*itr)->set( _scaleBias );
}




// namespace backdropFX
}
