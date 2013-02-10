// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_STUB_STAGE_H__
#define __BACKDROPFX_STUB_STAGE_H__ 1


#include <osgUtil/RenderStage>
#include <osg/FrameBufferObject>
#include <osg/Version>

#include <string>



namespace backdropFX
{


/** \cond */

class BackdropCommon;

class StubStage : public osgUtil::RenderStage
{
public:
    StubStage();
    StubStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    StubStage( const StubStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new StubStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new StubStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const StubStage*>(obj)!=0L; }
    virtual const char* className() const { return "StubStage"; }

    virtual void draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    void setBackdropCommon( BackdropCommon* backdropCommon );

protected:
    ~StubStage();
    void internalInit();

    BackdropCommon* _backdropCommon;
};


/** \endcond */


// namespace backdropFX
}

// __BACKDROPFX_STUB_STAGE_H__
#endif
