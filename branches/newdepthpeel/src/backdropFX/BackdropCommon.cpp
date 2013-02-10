// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/BackdropCommon.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/EffectLibrary.h>
#include <osgDB/FileUtils>
#include <osgUtil/CullVisitor>
#include <osgwTools/Shapes.h>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/Texture2D>

#include <backdropFX/Utils.h>
#include <iostream>
#include <sstream>
#include <iomanip>


namespace backdropFX
{


unsigned int BackdropCommon::debugConsole  ( 1u <<  0 );
unsigned int BackdropCommon::debugImages   ( 1u <<  1 );
unsigned int BackdropCommon::debugProfile  ( 1u <<  2 );
unsigned int BackdropCommon::debugShaders  ( 1u <<  3 );
unsigned int BackdropCommon::debugVisual   ( 1u <<  4 );


BackdropCommon::BackdropCommon()
  : _clearMode( CLEAR ),
    _clearColor( osg::Vec4( 0., 0., 0., 0. ) ),
    _clearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ),
    _debugMode( 0 ),
    _frameNumber( 0 )
{
    internalInit();
}
BackdropCommon::BackdropCommon( const BackdropCommon& rhs, const osg::CopyOp& copyop )
  : _clearMode( rhs._clearMode ),
    _clearColor( rhs._clearColor ),
    _clearMask( rhs._clearMask ),
    _clearImage( rhs._clearImage ),
    _fsq( rhs._fsq ), // TBD
    _fbo( rhs._fbo ),
    _debugMode( rhs._debugMode ),
    _frameNumber( rhs._frameNumber )
{
    internalInit();
}
BackdropCommon::~BackdropCommon()
{
}

void
BackdropCommon::internalInit()
{
    // TBD need to share the state set and VBO across all BackdropCommon classes.

    _fsq = osgwTools::makePlane(
        osg::Vec3( -1,-1,1 ), osg::Vec3( 2,0,0 ), osg::Vec3( 0,2,0 ) );
    _fsq->setColorBinding( osg::Geometry::BIND_OFF );
    _fsq->setNormalBinding( osg::Geometry::BIND_OFF );
    _fsq->setTexCoordArray( 0, NULL );
    _fsq->setUseDisplayList( false );
    _fsq->setUseVertexBufferObjects( true );

    // TBD. BackdropCommon should just use effects/none for
    // doing the image clear.
    _program = createEffectProgram( "none" );

    _textureUniform = new osg::Uniform( osg::Uniform::SAMPLER_2D_ARRAY, "inputTextures", 8 );
    int values[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    osg::IntArray* intArray = new osg::IntArray( 8, values );
    _textureUniform->setArray( intArray );

    _depth = new osg::Depth( osg::Depth::ALWAYS, 0., 1., true );
}


void
BackdropCommon::setClearColor( osg::Vec4 color )
{
    _clearColor = color;
}

void
BackdropCommon::setClearMask( GLint mask )
{
    _clearMask = mask;
}


void
BackdropCommon::setClearImage( osg::Texture2D* tex )
{
    _clearImage = tex;
}


void
BackdropCommon::performClear( osg::RenderInfo& renderInfo )
{
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

    osg::State& state( *( renderInfo.getState() ) );
    state.applyAttribute( _depth.get() );

    if( _clearMode == CLEAR_IMAGE )
    {
        const unsigned int contextID( state.getContextID() );
        osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );

        state.applyMode( GL_DEPTH_TEST, true );
        state.applyAttribute( _program.get() );
        GLint location = state.getUniformLocation( "inputTextures[0]" );
        if( location >= 0 )
            _textureUniform->apply( gl2Ext, location );
        else
            osg::notify( osg::WARN ) << "BDFX: bdfxCommon performClear, bad location" << std::endl;
        state.setActiveTextureUnit( 0 );
        state.applyTextureAttribute( 0, _clearImage.get() );

        _fsq->draw( renderInfo );
    }
    else if( _clearMode == CLEAR )
    {
        glClearColor( _clearColor.r(), _clearColor.g(), _clearColor.b(), _clearColor.a() );
        glClear( _clearMask );
    }
    // else do nothing.
}


void
BackdropCommon::setFBO( osg::FrameBufferObject* fbo )
{
    _fbo = fbo;
}
osg::FrameBufferObject*
BackdropCommon::getFBO() const
{
    return( _fbo.get() );
}


void
BackdropCommon::setDebugMode( unsigned int debugMode )
{
    if( _debugMode != debugMode )
    {
        _debugMode = debugMode;
    }
}

std::string
BackdropCommon::debugImageBaseFileName( unsigned int contextID )
{
    std::ostringstream ostr;
    ostr << std::setfill( '0' );
    ostr << "f" << std::setw( 6 ) << getFrameNumber() <<
        "_c" << std::setw( 2 ) << contextID << "_";
    return( ostr.str() );
}

void
BackdropCommon::setFrameNumber( int frameNumber )
{
    _frameNumber = frameNumber;
}

void
BackdropCommon::processCull( osgUtil::CullVisitor* cv )
{
    setFrameNumber( cv->getFrameStamp()->getFrameNumber() );
}


#if 0

    //
    // For internal use
    //

    void setRenderingCache( osg::Object* rc ) { _renderingCache = rc; }
    osg::Object* getRenderingCache() { return _renderingCache.get(); }
    const osg::Object* getRenderingCache() const { return _renderingCache.get(); }

    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;



    osg::ref_ptr< osg::Object > _renderingCache;
};
#endif

#if 0
class BackdropCommonStage : public osgUtil::RenderStage
{
public:
    BackdropCommonStage();
    BackdropCommonStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    BackdropCommonStage( const BackdropCommonStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new BackdropCommonStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new BackdropCommonStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const BackdropCommonStage*>(obj)!=0L; }
    virtual const char* className() const { return "BackdropCommonStage"; }

    void setBackdropCommon( BackdropCommon* bc );

protected:
    ~BackdropCommonStage();

    BackdropCommon* _bc;
};
#endif


// namespace backdropFX
}
