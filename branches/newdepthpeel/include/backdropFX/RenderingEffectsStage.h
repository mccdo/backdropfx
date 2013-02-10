// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_RENDERING_EFFECTS_STAGE_H__
#define __BACKDROPFX_RENDERING_EFFECTS_STAGE_H__ 1


#include <osgUtil/RenderStage>
#include <osg/FrameBufferObject>
#include <osg/Version>

#include <string>



namespace backdropFX
{


class RenderingEffects;

/** \brief Draw traversal support for the RenderingEffects node.

This is a custom osgUtil::RenderStage. The relationship between the
RenderingEffects class and RenderingEffectsStage is analogous to the
relationship between the osg::Camera and osgUtil::RenderStage.

RenderingEffectsStage overrides the base class draw() function to process
the RenderingEffects EffectVector. RenderingEffectsStage sequencially draws each Effect
in the Effects vector.
*/
class RenderingEffectsStage : public osgUtil::RenderStage
{
public:
    RenderingEffectsStage();
    RenderingEffectsStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    RenderingEffectsStage( const RenderingEffectsStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new RenderingEffectsStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new RenderingEffectsStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const RenderingEffectsStage*>(obj)!=0L; }
    virtual const char* className() const { return "RenderingEffectsStage"; }

    virtual void draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    void setRenderingEffects( RenderingEffects* renderingEffects );
    RenderingEffects* getRenderingEffects() const { return( _renderingEffects ); }


protected:
    ~RenderingEffectsStage();
    void internalInit();

    RenderingEffects* _renderingEffects;
};


// namespace backdropFX
}

// __BACKDROPFX_RENDERING_EFFECTS_STAGE_H__
#endif
