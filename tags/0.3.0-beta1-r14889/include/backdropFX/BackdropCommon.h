// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_BACKDROP_COMMON_H__
#define __BACKDROPFX_BACKDROP_COMMON_H__ 1

#include <backdropFX/Export.h>
#include <osg/GL>
#include <osg/Geometry>
#include <osg/FrameBufferObject>
#include <osg/Texture2D>
#include <osg/Depth>


namespace osgUtil {
    class CullVisitor;
}

namespace backdropFX
{


class BACKDROPFX_EXPORT BackdropCommon
{
public:
    BackdropCommon();
    BackdropCommon( const BackdropCommon& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    
    /** Controls for clearing */
    typedef enum {
        CLEAR,
        CLEAR_IMAGE,
        CLEAR_OFF
    } ClearMode;

    void setClearMode( ClearMode cm ) { _clearMode = cm; }
    ClearMode getClearMode() const { return( _clearMode ); }

    void setClearColor( osg::Vec4 color );
    void setClearMask( GLint mask );

    void setClearImage( osg::Texture2D* tex );

    void performClear( osg::RenderInfo& renderInfo );


    
    /** Specify the FBO to render to. NULL sets the FBO binding to 0. */
    virtual void setFBO( osg::FrameBufferObject* fbo );
    osg::FrameBufferObject* getFBO() const;


    
    /** Controls for debugging and profiling. */
    static unsigned int debugConsole;
    static unsigned int debugImages;
    static unsigned int debugProfile;
    static unsigned int debugShaders;
    static unsigned int debugVisual;

    virtual void setDebugMode( unsigned int debugMode );
    unsigned int getDebugMode() const { return( _debugMode); }

    std::string debugImageBaseFileName( unsigned int contextID );


    
    /** Use frame number in debug mode for image file names. */
    void setFrameNumber( int frameNumber );
    int getFrameNumber() const { return( _frameNumber ); }


    //
    // For internal use
    //
#if 0
    void setRenderingCache( osg::Object* rc ) { _renderingCache = rc; }
    osg::Object* getRenderingCache() { return _renderingCache.get(); }
    const osg::Object* getRenderingCache() const { return _renderingCache.get(); }

    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;
#endif
protected:
    ~BackdropCommon();
    void internalInit();

    void processCull( osgUtil::CullVisitor* cv );

    ClearMode _clearMode;
    osg::Vec4 _clearColor;
    GLbitfield _clearMask;
    osg::ref_ptr< osg::Texture2D > _clearImage;

    osg::ref_ptr< osg::FrameBufferObject > _fbo;

    unsigned int _debugMode;
    int _frameNumber;

    //osg::ref_ptr< osg::Object > _renderingCache;

    // TBD need to share these
    osg::ref_ptr< osg::Geometry > _fsq;
    osg::ref_ptr< osg::Depth > _depth;
    osg::ref_ptr< osg::Program > _program;
    osg::ref_ptr< osg::Uniform > _textureUniform;
};

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

// __BACKDROPFX_BACKDROP_COMMON_H__
#endif
