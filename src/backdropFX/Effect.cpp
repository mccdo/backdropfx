// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/RenderingEffects.h>
#include <backdropFX/RenderingEffectsStage.h>
#include <backdropFX/Effect.h>
#include <backdropFX/Utils.h>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osgwTools/Shapes.h>
#include <osgwTools/Version.h>
#include <backdropFX/Utils.h>
#include <osgwTools/FBOUtils.h>

#include <sstream>


namespace backdropFX
{


Effect::Effect()
{
    // TBD need to share the uniform and VBO across all BackdropCommon classes.

    _fstp = osgwTools::makePlane(
        osg::Vec3( -1,-1,1 ), osg::Vec3( 2,0,0 ), osg::Vec3( 0,2,0 ) );
    _fstp->setColorBinding( osg::Geometry::BIND_OFF );
    _fstp->setNormalBinding( osg::Geometry::BIND_OFF );
    _fstp->setTexCoordArray( 0, NULL );
    _fstp->setUseDisplayList( false );
    _fstp->setUseVertexBufferObjects( true );

    _textureUniform = new osg::Uniform( osg::Uniform::SAMPLER_2D_ARRAY, "inputTextures", 8 );
    int values[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    osg::IntArray* intArray = new osg::IntArray( 8, values );
    _textureUniform->setArray( intArray );

    _depth = new osg::Depth( osg::Depth::ALWAYS, 0., 1., true );
}
Effect::Effect( const Effect& rhs, const osg::CopyOp& copyop )
  : osg::Object( rhs ),
    _program( rhs._program ),
    _inputs( rhs._inputs ),
    _output( rhs._output ),
    // TBD need to share this with all Effect classes (a static, perhaps?)
    _fstp( rhs._fstp ),
    _depth( rhs._depth ),
    _textureUniform( rhs._textureUniform )
{
}
Effect::~Effect()
{
}


void
Effect::addInput( const unsigned int unit, osg::Texture* texture )
{
    _inputs[ unit ] = texture;
}
osg::Texture*
Effect::getInput( const unsigned int unit ) const
{
    IntTextureMap::const_iterator itr = _inputs.find( unit );
    if( itr != _inputs.end() )
        return( itr->second.get() );
    else
        return( NULL );
}
int
Effect::getInputUnit( const osg::Texture* texture ) const
{
    IntTextureMap::const_iterator itr;
    for( itr = _inputs.begin(); itr != _inputs.end(); itr++ )
    {
        if( itr->second == texture )
            return( itr->first );
    }
    return( -1 );
}
bool
Effect::removeInput( const unsigned int unit )
{
    IntTextureMap::iterator itr = _inputs.find( unit );
    if( itr != _inputs.end() )
    {
        _inputs.erase( itr );
        return( true );
    }
    else
        return( false );
}
bool
Effect::removeInput( osg::Texture* texture )
{
    int unit = getInputUnit( texture );
    if( unit < 0 )
        return( false );
    else
        return( removeInput( (unsigned int)unit ) );
}

void
Effect::setOutput( osg::FrameBufferObject* fbo )
{
    _output = fbo;
}
osg::FrameBufferObject*
Effect::getOutput() const
{
    return( _output.get() );
}


void
Effect::setProgram( osg::Program* program )
{
    _program = program;
}

void
Effect::draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo )
{
    osg::State& state = *( renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    RenderingEffects* renderingEffects = rfxs->getRenderingEffects();

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
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
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

    const osg::Viewport* vp = dynamic_cast< const osg::Viewport* >(
        state.getLastAppliedAttribute( osg::StateAttribute::VIEWPORT ) );
    vp->apply( state );


    // Use the specified program.
    if( _program.valid() )
    {
        osg::notify( osg::INFO ) << "Effect: applying program." << std::endl;

        state.applyAttribute( _program.get() );
        if( !_inputs.empty() )
        {
            osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );
            _textureUniform->apply( gl2Ext, state.getUniformLocation( _textureUniform->getName().c_str() ) );
        }
    }

    // Bind the input textures.
    IntTextureMap::const_iterator inItr;
    for( inItr = _inputs.begin(); inItr != _inputs.end(); inItr++ )
    {
        osg::notify( osg::INFO ) << "Effect: applying input texture." << std::endl;

        state.setActiveTextureUnit( inItr->first );
        osg::Texture* texture = inItr->second.get();
        state.applyTextureAttribute( inItr->first, texture );
    }

    UTIL_GL_ERROR_CHECK( "Effect::draw()." ) \

    // Draw.
    internalDraw( renderInfo );

    if( ( renderingEffects->getDebugMode() & backdropFX::BackdropCommon::debugImages ) != 0 )
        dumpImage( rfxs->getViewport(), renderingEffects->debugImageBaseFileName( contextID ) );
}

void
Effect::setTextureWidthHeight( unsigned int texW, unsigned int texH )
{
    _width = texW;
    _height = texH;
}

void
Effect::internalDraw( osg::RenderInfo& renderInfo )
{
    osg::notify( osg::INFO ) << "Effect: internalDraw rendering TRIANGLE_PAIR." << std::endl;

    // Disable depth test for fullscreen tri pair.
    osg::State& state = *( renderInfo.getState() );
    state.applyAttribute( _depth.get() );
    state.applyMode( GL_DEPTH_TEST, true );
    // Force disable blending, otherwise we get this psychadelic blur effect.
    state.applyMode( GL_BLEND, false );

    _fstp->draw( renderInfo );

    UTIL_GL_ERROR_CHECK( "Effect::internamDraw()." ) \
}

void
Effect::dumpImage( const osg::Viewport* vp, const std::string baseFileName )
{
    const GLint x( vp->x() );
    const GLint y( vp->y() );
    const GLsizei w( vp->width() );
    const GLsizei h( vp->height() );

    std::string fileName;
    {
        std::ostringstream ostr;
        ostr << baseFileName;
        ostr << "effect-" << getName() << ".png";
        fileName = std::string( ostr.str() );
    }

    char* pixels = new char[ w * h * 4 ];
    glReadPixels( x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels );
    backdropFX::debugDumpImage( fileName, pixels, w, h );
    delete[] pixels;
}


// namespace backdropFX
}
