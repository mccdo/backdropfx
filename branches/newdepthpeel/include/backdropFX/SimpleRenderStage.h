// Copyright (c) 2008 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_SIMPLE_RENDER_STAGE_H__
#define __BACKDROPFX_SIMPLE_RENDER_STAGE_H__ 1


#include <backdropFX/Export.h>
#include <osgUtil/RenderStage>
#include <osg/FrameBufferObject>
#include <osg/Version>

#include <string>



namespace backdropFX
{


/** \cond */

class BACKDROPFX_EXPORT SimpleRenderStage : public osgUtil::RenderStage
{
public:
    SimpleRenderStage();
    SimpleRenderStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    SimpleRenderStage( const SimpleRenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new SimpleRenderStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new SimpleRenderStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const SimpleRenderStage*>(obj)!=0L; }
    virtual const char* className() const { return "SimpleRenderStage"; }

    virtual void draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    void setFBO( osg::FrameBufferObject* fbo );
    const osg::FrameBufferObject& getFBO() const;


protected:
    ~SimpleRenderStage();

    void internalInit();

    osg::ref_ptr< osg::FrameBufferObject > _fbo;
};

/** \endcond */


// namespace backdropFX
}

// __BACKDROPFX_SIMPLE_RENDER_STAGE_H__
#endif
