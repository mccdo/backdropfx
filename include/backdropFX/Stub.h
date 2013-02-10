// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_STUB_H__
#define __BACKDROPFX_STUB_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/BackdropCommon.h>
#include <osg/Group>
#include <osg/FrameBufferObject>

#include <backdropFX/StubStage.h>


namespace backdropFX {


/** \cond */

class BACKDROPFX_EXPORT Stub : public osg::Group, public backdropFX::BackdropCommon
{
public:
    Stub();
    Stub( const Stub& stub, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    META_Node( backdropFX, Stub );

    void traverse( osg::NodeVisitor& nv );


    //
    // For internal use
    //

    void setRenderingCache( osg::Object* rc ) { _renderingCache = rc; }
    osg::Object* getRenderingCache() { return _renderingCache.get(); }
    const osg::Object* getRenderingCache() const { return _renderingCache.get(); }

    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;

protected:
    ~Stub();
    void internalInit();

    osg::ref_ptr< osg::Object > _renderingCache;
};

/** \endcond */


// namespace backdropFX
}

// __BACKDROPFX_STUB_H__
#endif
