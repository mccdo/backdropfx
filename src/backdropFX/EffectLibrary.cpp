// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/Manager.h>
#include <backdropFX/EffectLibrary.h>
#include <backdropFX/Effect.h>
#include <backdropFX/RenderingEffects.h>
#include <backdropFX/RTTViewport.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osg/Program>
#include <osg/Texture2D>
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
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
#endif
    {
        osg::Uniform* uniform = rfxs->getTexturePercentUniform();
#if OSG_SUPPORTS_UNIFORM_ID
        GLint location = state.getUniformLocation( uniform->getNameID() );
#else
        GLint location = state.getUniformLocation( uniform->getName() );
#endif
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
    UTIL_GL_FBO_ERROR_CHECK( (getName() + std::string(" Effect::draw") ), fboExt );
 
    // If the 'last' flag is set, apply the full viewport.
    backdropFX::RTTViewport* rttvp = last ? dynamic_cast< backdropFX::RTTViewport* >( rfxs->getViewport() ) : NULL;
    if( rttvp != NULL )
        rttvp->applyFullViewport( state );
    else
        state.applyAttribute( rfxs->getViewport() );


    // OK, do the blur now...
    state.applyAttribute( _vertical.get() );
    uniform = _textureUniform[ 0 ].get();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
#endif
    {
        osg::Uniform* uniform = rfxs->getTexturePercentUniform();
#if OSG_SUPPORTS_UNIFORM_ID
        GLint location = state.getUniformLocation( uniform->getNameID() );
#else
        GLint location = state.getUniformLocation( uniform->getName() );
#endif
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


GaussConvolution::GaussConvolution()
    :
    Effect(),
    m_viewportSize(
        new osg::Uniform( "viewportSize", osg::Vec2( 800.0, 600.0 ) ) ),
    m_viewport( new osg::Viewport( 0, 0, 0, 0 ) ),
    m_1Dx( createEffectProgram( "gaussConvolution1Dx" ) ),
    m_1Dy( createEffectProgram( "gaussConvolution1Dy" ) ),
    m_tex2D( new osg::Texture2D() ),
    m_fbo( new osg::FrameBufferObject() )
{
    UTIL_MEMORY_CHECK(
        m_viewportSize.get(), "GaussConvolution Viewport Size Uniform", );

    UTIL_MEMORY_CHECK( m_viewport.get(), "GaussConvolution m_viewport", )

    UTIL_MEMORY_CHECK( m_1Dx.get(), "GaussConvolution m_1Dx", )
    UTIL_MEMORY_CHECK( m_1Dy.get(), "GaussConvolution m_1Dy", )

    UTIL_MEMORY_CHECK( m_tex2D.get(), "GaussConvolution m_tex2D", )
    //TBD can't hard code this size
    m_tex2D->setTextureSize( 800, 600 );
    m_tex2D->dirtyTextureObject();
    m_tex2D->setInternalFormat( GL_RGBA );
    m_tex2D->setBorderWidth( 0 );
    m_tex2D->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    m_tex2D->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    m_tex2D->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    m_tex2D->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    UTIL_MEMORY_CHECK( m_fbo.get(), "GaussConvolution m_fbo", )
    m_fbo->setAttachment(
        osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( m_tex2D.get() ) );
}

GaussConvolution::GaussConvolution(
    GaussConvolution const& rhs,
    osg::CopyOp const& copyop )
    :
    Effect( rhs, copyop ),
    m_viewportSize( rhs.m_viewportSize ),
    m_viewport( rhs.m_viewport ),
    m_1Dx( rhs.m_1Dx ),
    m_1Dy( rhs.m_1Dy ),
    m_tex2D( rhs.m_tex2D ),
    m_fbo( rhs.m_fbo )
{
    osg::notify( osg::NOTICE )
        << "BDFX: GaussConvolution: Unexcepted copy operator invocation."
        << std::endl;
}

GaussConvolution::~GaussConvolution()
{
    ;
}

void GaussConvolution::draw(
    RenderingEffectsStage* rfxs,
    osg::RenderInfo& renderInfo,
    bool last )
{
    osg::State& state = *( renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    RenderingEffects* renderingEffects = rfxs->getRenderingEffects();

    //Bind the output FBO
    osg::FBOExtensions* fboExt(
        osg::FBOExtensions::instance( contextID, true ) );
    m_fbo->apply( state );
    UTIL_GL_FBO_ERROR_CHECK( "GaussConvolution::draw", fboExt );

    state.applyAttribute( m_viewport.get() );

    //Use the specified program
    state.applyAttribute( m_1Dx.get() );

    //Bind the input textures and set their sampler uniforms
    IntTextureMap::const_iterator inItr = _inputs.find( 0 );
    state.setActiveTextureUnit( 0 );
    osg::Texture* texture = inItr->second.get();
    state.applyTextureAttribute( 0, texture );

    osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );
    osg::Uniform* uniform = _textureUniform[ 0 ].get();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
#endif
    uniform = rfxs->getTexturePercentUniform();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
    m_viewportSize->apply( gl2Ext, state.getUniformLocation( m_viewportSize->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
    m_viewportSize->apply( gl2Ext, state.getUniformLocation( m_viewportSize->getName() ) );
#endif

    UTIL_GL_ERROR_CHECK( "GaussConvolution::draw()." ) \

    //Draw
    internalDraw( renderInfo );

    //Bind the output FBO
    osg::FrameBufferObject* fbo;
    if( _output.valid() )
    {
        osg::notify( osg::INFO )
            << "Effect: applying local Effect FBO." << std::endl;

        //We have an FBO assigned, use it
        fbo = _output.get();
    }
    else
    {
        osg::notify( osg::INFO )
            << "Effect: applying RenderingEffects FBO." << std::endl;

        //Use RenderingEffects output
        fbo = renderingEffects->getFBO();
    }
    if( fbo != NULL )
    {
        fbo->apply( state );
    }
    else
    {
        //Bind the default framebuffer
        osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, 0 );
    }
    UTIL_GL_FBO_ERROR_CHECK( (getName() + std::string(" Effect::draw") ), fboExt );

    state.applyAttribute( m_viewport.get() );

    //Use the specified program
    state.applyAttribute( m_1Dy.get() );
    state.setActiveTextureUnit( 0 );
    state.applyTextureAttribute( 0, m_tex2D.get() );

    uniform = _textureUniform[ 0 ].get();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
#endif
    uniform = rfxs->getTexturePercentUniform();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
    m_viewportSize->apply( gl2Ext, state.getUniformLocation( m_viewportSize->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
    m_viewportSize->apply( gl2Ext, state.getUniformLocation( m_viewportSize->getName() ) );
#endif

    internalDraw( renderInfo );

    if( ( renderingEffects->getDebugMode() &
          backdropFX::BackdropCommon::debugImages ) != 0 )
    {
        dumpImage(
            m_viewport.get(),
            renderingEffects->debugImageBaseFileName( contextID ) );
    }
}

void GaussConvolution::setTextureWidthHeight(
    unsigned int texW, unsigned int texH )
{
    Effect::setTextureWidthHeight( texW, texH );

    m_viewportSize->set( osg::Vec2(
        static_cast< osg::Viewport::value_type >( texW ),
        static_cast< osg::Viewport::value_type >( texH ) ) );
    m_viewport->width() =
        static_cast< osg::Viewport::value_type >( texW );
    m_viewport->height() =
        static_cast< osg::Viewport::value_type >( texH );

    m_tex2D->setTextureSize( texW, texH );
    m_tex2D->dirtyTextureObject(); 

    //Force rebuild of FBOs
    m_fbo = new osg::FrameBufferObject();
    UTIL_MEMORY_CHECK( m_fbo.get(), "GaussConvolution m_fbo", )
    m_fbo->setAttachment(
        osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( m_tex2D.get() ) );
}


Resample::Resample()
    :
    Effect(),
    m_factor( 1.0, 1.0 ),
    m_viewport( new osg::Viewport( 0, 0, 0, 0 ) ),
    m_tex2D( new osg::Texture2D() )
{
    UTIL_MEMORY_CHECK( m_viewport.get(), "Resample m_viewport", )

    _program = backdropFX::createEffectProgram( "none" );
    UTIL_MEMORY_CHECK( _program.get(), "Resample _program", )

    UTIL_MEMORY_CHECK( m_tex2D.get(), "Resample m_tex2D", )
    //TBD can't hard code this size
    m_tex2D->setTextureSize( 800, 600 );
    m_tex2D->dirtyTextureObject();
    m_tex2D->setInternalFormat( GL_RGBA );
    m_tex2D->setBorderWidth( 0 );
    m_tex2D->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    m_tex2D->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );
    m_tex2D->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    m_tex2D->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );

    _output = new osg::FrameBufferObject();
    UTIL_MEMORY_CHECK( _output.get(), "GaussConvolution _output", )
    _output->setAttachment(
        osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( m_tex2D.get() ) );
}

Resample::Resample(
    Resample const& rhs,
    osg::CopyOp const& copyop )
    :
    Effect( rhs, copyop ),
    m_factor( rhs.m_factor ),
    m_viewport( rhs.m_viewport ),
    m_tex2D( rhs.m_tex2D )
{
    osg::notify( osg::NOTICE )
        << "BDFX: Resample: Unexcepted copy operator invocation."
        << std::endl;
}

Resample::~Resample()
{
    ;
}

void Resample::draw(
    RenderingEffectsStage* rfxs,
    osg::RenderInfo& renderInfo,
    bool last )
{
    osg::State& state = *( renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    RenderingEffects* renderingEffects = rfxs->getRenderingEffects();

    //Bind the output FBO
    osg::FrameBufferObject* fbo;
    if( _output.valid() )
    {
        osg::notify( osg::INFO )
            << "Resample: applying local Effect FBO." << std::endl;

        //We have an FBO assigned, use it
        fbo = _output.get();
    }

    osg::FBOExtensions* fboExt(
        osg::FBOExtensions::instance( contextID, true ) );
    if( fbo != NULL )
    {
        fbo->apply( state );
    }

    UTIL_GL_FBO_ERROR_CHECK( "Resample::draw", fboExt );

    state.applyAttribute( m_viewport.get() );

    //Use the specified program
    if( _program.valid() )
    {
        osg::notify( osg::INFO ) << "Resample: applying program." << std::endl;
        state.applyAttribute( _program.get() );
    }

    //Bind the input textures and set their sampler uniforms
    IntTextureMap::const_iterator inItr = _inputs.find( 0 );
    state.setActiveTextureUnit( 0 );
    osg::Texture* texture = inItr->second.get();
    state.applyTextureAttribute( 0, texture );

    osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );
    osg::Uniform* uniform = _textureUniform[ 0 ].get();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
#endif
    uniform = rfxs->getTexturePercentUniform();
#if OSG_SUPPORTS_UNIFORM_ID
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getNameID() ) );
#else
    uniform->apply( gl2Ext, state.getUniformLocation( uniform->getName() ) );
#endif

    UTIL_GL_ERROR_CHECK( "Resample::draw()." ) \

    //Draw
    internalDraw( renderInfo );

    if( ( renderingEffects->getDebugMode() &
          backdropFX::BackdropCommon::debugImages ) != 0 )
    {
        dumpImage(
            m_viewport.get(),
            renderingEffects->debugImageBaseFileName( contextID ) );
    }
}

void Resample::setFactor( float xFactor, float yFactor )
{
    m_factor.set( xFactor, yFactor );
}

void Resample::setTextureWidthHeight(
    unsigned int texW, unsigned int texH )
{
    m_viewport->width() =
        static_cast< osg::Viewport::value_type >( texW * m_factor.x() );
    m_viewport->height() =
        static_cast< osg::Viewport::value_type >( texH * m_factor.y() );

    _width = static_cast< unsigned int >( m_viewport->width() );
    _height = static_cast< unsigned int >( m_viewport->height() );

    m_tex2D->setTextureSize( _width, _height );
    m_tex2D->dirtyTextureObject();

    _output = new osg::FrameBufferObject();
    _output->setAttachment(
        osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( m_tex2D.get() ) );
}

GlowCombine::GlowCombine()
    :
    Effect()
{
    osg::Program* program = backdropFX::createEffectProgram( "glowCombine" );
    setProgram( program );
}

GlowCombine::GlowCombine(
    GlowCombine const& rhs,
    osg::CopyOp const& copyop )
    :
    Effect( rhs, copyop )
{
    osg::notify( osg::NOTICE )
        << "BDFX: GlowCombine: Unexcepted copy operator invocation."
        << std::endl;
}

GlowCombine::~GlowCombine()
{
    ;
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




EffectBasicGlow::EffectBasicGlow()
  : _glowColor( osg::Vec4( 0., 0., 0., 0. ) ),
    _useGlowAlpha( false )
{
    //  EffectSeperableBlur
    //  EffectCombine

    Effect* effectBlur = new backdropFX::EffectSeperableBlur();
    UTIL_MEMORY_CHECK( effectBlur, "EffectLibraryUtil Glow EffectSeperableBlur", );
    effectBlur->setName( "EffectBasicGlow-Blur" );
    _subEffects.push_back( effectBlur );

    Effect* effectCombine = new backdropFX::EffectCombine();
    UTIL_MEMORY_CHECK( effectCombine, "EffectLibraryUtil Glow EffectCombine", );
    effectCombine->setName( "EffectBasicGlow-Combine" );
    _subEffects.push_back( effectCombine );

    effectBlur->attachOutputTo( effectCombine, 0 );

    _glowColorUniform = new osg::Uniform( "bdfx_glowColor", _glowColor );
    UTIL_MEMORY_CHECK( _glowColorUniform, "EffectBasicGlow constructur uniform", );
    _effectsPassUniforms.push_back( _glowColorUniform );

    _useGlowAlphaUniform = new osg::Uniform( "bdfx_useGlowAlpha", _useGlowAlpha?1:0 );
    UTIL_MEMORY_CHECK( _useGlowAlphaUniform, "EffectBasicGlow constructur uniform", );
    _effectsPassUniforms.push_back( _useGlowAlphaUniform );
}
EffectBasicGlow::EffectBasicGlow( const EffectBasicGlow& rhs, const osg::CopyOp& copyop )
  : CompositeEffect( rhs, copyop ),
    _glowColor( rhs._glowColor ),
    _useGlowAlpha( rhs._useGlowAlpha ),
    _glowColorUniform( rhs._glowColorUniform ),
    _useGlowAlphaUniform( rhs._useGlowAlphaUniform )
{
}
EffectBasicGlow::~EffectBasicGlow()
{
}

void EffectBasicGlow::addInput( const unsigned int unit, osg::Texture* texture )
{
    // Calling code will attach:
    //   ColorBufferA to input 0
    //   ColorBufferGlow to input 1
    if( unit == 0 )
    {
        // Attach ColorBufferA.
        // Input 1 of the combine effect.
        ( _subEffects[ 1 ] )->addInput( 1, texture );
    }
    else if( unit == 1 )
    {
        // Attach ColorBufferGlow.
        // Input 0 of the blur effect.
        ( _subEffects[ 0 ] )->addInput( 0, texture );
    }
    else
    {
        osg::notify( osg::WARN ) << "backdropFX: EffectBasicGlow: Request to attach unsupported input." << std::endl;
    }
}

void EffectBasicGlow::setDefaultGlowColor( const osg::Vec4f& glowColor )
{
    _glowColor = glowColor;
    _glowColorUniform->set( _glowColor );
}
void EffectBasicGlow::setUseGlowAlpha( bool useGlowAlpha )
{
    _useGlowAlpha = useGlowAlpha;
    _useGlowAlphaUniform->set( _useGlowAlpha?1:0 );
}



EffectImprovedGlow::EffectImprovedGlow()
  : _resampleFactor( .5, .5 ),
    _glowColor( osg::Vec4( 0., 0., 0., 0. ) ),
    _useGlowAlpha( false )
{
    //  Resample
    //  GaussConvolution
    //  GlowCombine

    Effect* effectResample = new backdropFX::Resample();
    UTIL_MEMORY_CHECK( effectResample, "EffectLibraryUtil Glow Resample", );
    effectResample->setName( "EffectImprovedGlow-Resample" );
    static_cast< Resample* >( effectResample )->setFactor(
        _resampleFactor.x(), _resampleFactor.y() );
    _subEffects.push_back( effectResample );

    Effect* effectBlur = new backdropFX::GaussConvolution();
    UTIL_MEMORY_CHECK( effectBlur, "EffectLibraryUtil Glow GaussConvolution", );
    effectBlur->setName( "EffectImprovedGlow-Blur" );
    _subEffects.push_back( effectBlur );

    effectResample->attachOutputTo( effectBlur, 0 );

    Effect* effectCombine = new backdropFX::GlowCombine();
    UTIL_MEMORY_CHECK( effectCombine, "EffectLibraryUtil Glow GlowCombine", );
    effectCombine->setName( "EffectImprovedGlow-GlowCombine" );
    _subEffects.push_back( effectCombine );

    effectBlur->attachOutputTo( effectCombine, 1 );

    _glowColorUniform = new osg::Uniform( "bdfx_glowColor", _glowColor );
    UTIL_MEMORY_CHECK( _glowColorUniform, "EffectBasicGlow constructur uniform", );
    _effectsPassUniforms.push_back( _glowColorUniform );

    _useGlowAlphaUniform = new osg::Uniform( "bdfx_useGlowAlpha", _useGlowAlpha?1:0 );
    UTIL_MEMORY_CHECK( _useGlowAlphaUniform, "EffectBasicGlow constructur uniform", );
    _effectsPassUniforms.push_back( _useGlowAlphaUniform );
}
EffectImprovedGlow::EffectImprovedGlow( const EffectImprovedGlow& rhs, const osg::CopyOp& copyop )
  : CompositeEffect( rhs, copyop ),
    _resampleFactor( rhs._resampleFactor ),
    _glowColor( rhs._glowColor ),
    _useGlowAlpha( rhs._useGlowAlpha ),
    _glowColorUniform( rhs._glowColorUniform ),
    _useGlowAlphaUniform( rhs._useGlowAlphaUniform )
{
}
EffectImprovedGlow::~EffectImprovedGlow()
{
}

void EffectImprovedGlow::setTextureWidthHeight( unsigned int texW, unsigned int texH )
{
    int idx;
    EffectVector::iterator it;
    for( it = _subEffects.begin(), idx=0; it != _subEffects.end(); it++, idx++ )
    {
        if( idx == 1 )
        {
            // Special handling for the GaussConvolution effect.
            (*it)->setTextureWidthHeight( texW * _resampleFactor.x(), texH * _resampleFactor.y() );
        }
        else
        {
            (*it)->setTextureWidthHeight( texW, texH );
        }
    }
}

void EffectImprovedGlow::addInput( const unsigned int unit, osg::Texture* texture )
{
    // Calling code will attach:
    //   ColorBufferA to input 0
    //   ColorBufferGlow to input 1
    if( unit == 0 )
    {
        // Attach ColorBufferA.
        // Input 0 of the 3rd (combine) effect.
        ( _subEffects[ 2 ] )->addInput( 0, texture );
    }
    else if( unit == 1 )
    {
        // Attach ColorBufferGlow.
        // Input 0 of the 1st (resample) effect.
        // Input 2 of the 3rd (combine) effect.
        ( _subEffects[ 0 ] )->addInput( 0, texture );
        ( _subEffects[ 2 ] )->addInput( 2, texture );
    }
    else
    {
        osg::notify( osg::WARN ) << "backdropFX: EffectImprovedGlow: Request to attach unsupported input." << std::endl;
    }
}

void EffectImprovedGlow::setDefaultGlowColor( const osg::Vec4f& glowColor )
{
    _glowColor = glowColor;
    _glowColorUniform->set( _glowColor );
}
void EffectImprovedGlow::setUseGlowAlpha( bool useGlowAlpha )
{
    _useGlowAlpha = useGlowAlpha;
    _useGlowAlphaUniform->set( _useGlowAlpha?1:0 );
}



EffectDOF::EffectDOF()
  : _distance( 2.f ),
    _range( 500.f )
{
    //  EffectSeperableBlur
    //  EffectAlphaBlend

    Effect* effectBlur = new backdropFX::EffectSeperableBlur();
    UTIL_MEMORY_CHECK( effectBlur, "EffectLibraryUtil DOF EffectSeperableBlur", );
    effectBlur->setName( "EffectDOF-Blur" );
    _subEffects.push_back( effectBlur );

    Effect* effectBlend = new backdropFX::EffectAlphaBlend();
    UTIL_MEMORY_CHECK( effectBlend, "EffectLibraryUtil DOF EffectAlphaBlend", );
    effectBlend->setName( "EffectDOF-Blend" );
    _subEffects.push_back( effectBlend );

    effectBlur->attachOutputTo( effectBlend, 0 );

    _focalDistance = new osg::Uniform( "bdfx_dofFocalDistance", _distance );
    UTIL_MEMORY_CHECK( _focalDistance, "EffectDOF constructor uniform", );
    _effectsPassUniforms.push_back( _focalDistance );

    _focalRange = new osg::Uniform( "bdfx_dofFocalRange", _range );
    UTIL_MEMORY_CHECK( _focalRange, "EffectDOF constructor uniform", );
    _effectsPassUniforms.push_back( _focalRange );
}
EffectDOF::EffectDOF( const EffectDOF& rhs, const osg::CopyOp& copyop )
  : CompositeEffect( rhs, copyop ),
    _distance( rhs._distance ),
    _range( rhs._range )
{
}
EffectDOF::~EffectDOF()
{
}

void EffectDOF::addInput( const unsigned int unit, osg::Texture* texture )
{
    // Calling code will attach:
    //   ColorBufferA (or upstream output) to input 0
    if( unit == 0 )
    {
        // Attach ColorBufferA.
        // Input 0 of the 1st (blur) effect.
        // Input 1 of the 2nd (combine) effect.
        ( _subEffects[ 0 ] )->addInput( 0, texture );
        ( _subEffects[ 1 ] )->addInput( 1, texture );
    }
    else
    {
        osg::notify( osg::WARN ) << "backdropFX: EffectImprovedGlow: Request to attach unsupported input." << std::endl;
    }

    // Putting this here so that the constructor doesn't invoke the Manager singleton instance.
    // This results in some redundancy, but no runtime performance penalty.
    backdropFX::Manager& mgr = *( backdropFX::Manager::instance() );
    ( _subEffects[ 1 ] )->addInput( 2, mgr.getDepthBuffer() );
}

void EffectDOF::setFocalDistance( float distance )
{
    _distance = distance;
    _focalDistance->set( _distance );
}
void EffectDOF::setFocalRange( float range )
{
    _range = range;
    _focalRange->set( _range );
}



EffectHeatDistortion::EffectHeatDistortion()
  : _range( osg::Vec2f( 20.f, 60.f ) ) // TBD default should be 0,1000 (or maybe more?)
{
    // TBD formalize the texture-based noise feature.
    osg::ref_ptr< osg::Image > noiseImage = osgDB::readImageFile( "noise.png" );
    UTIL_MEMORY_CHECK( noiseImage, "setEffectSet heatEffect noise image", );
    osg::ref_ptr< osg::Texture2D > noiseTex = new osg::Texture2D( noiseImage.get() );
    UTIL_MEMORY_CHECK( noiseTex, "setEffectSet heatEffect noise texture", );
    noiseTex->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
    noiseTex->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );
    noiseTex->setUseHardwareMipMapGeneration( true );
    noiseTex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR );
    noiseTex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    setProgram( createEffectProgram( "heat-texture" ) );
    addInput( 1, noiseTex.get() );
    // TBD shouldn't invoke Manager constructor here
    //   Could avoid this by overriding addInput.
    addInput( 2, Manager::instance()->getDepthBuffer() );

    _heatRange = new osg::Uniform( "bdfx_heatMinMax", _range );
    UTIL_MEMORY_CHECK( _heatRange, "EffectHeatRange constructur uniform", );
    _effectsPassUniforms.push_back( _heatRange );
}
EffectHeatDistortion::EffectHeatDistortion( const EffectHeatDistortion& rhs, const osg::CopyOp& copyop )
  : Effect( rhs, copyop ),
    _range( rhs._range ),
    _heatRange( rhs._heatRange )
{
}
EffectHeatDistortion::~EffectHeatDistortion()
{
}

void EffectHeatDistortion::setHeatRange( const osg::Vec2f& range )
{
    _range = range;
    _heatRange->set( _range );
}



// namespace backdropFX
}
