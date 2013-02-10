// Copyright (c) 2011 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_SHADOW_MAP_STAGE_H__
#define __BACKDROPFX_SHADOW_MAP_STAGE_H__ 1


#include <backdropFX/Export.h>
#include <osgUtil/RenderStage>
#include <osg/FrameBufferObject>
#include <osg/Version>

#include <string>



namespace backdropFX
{


class ShadowMap;

class BACKDROPFX_EXPORT ShadowMapStage : public osgUtil::RenderStage
{
public:
    ShadowMapStage();
    ShadowMapStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    ShadowMapStage( const ShadowMapStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new ShadowMapStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new ShadowMapStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ShadowMapStage*>(obj)!=0L; }
    virtual const char* className() const { return "ShadowMapStage"; }

    virtual void draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    void setShadowMapNode( ShadowMap* shadowMapNode );
    ShadowMap* getShadowMapNode() const { return( _shadowMapNode ); }

    void setViewProjStateSet( osg::StateSet* ss );
    osg::StateSet* getViewProjStateSet() const;
    void setDepthTexStateSet( osg::StateSet* ss );
    osg::StateSet* getDepthTexStateSet() const;

    void setFBO( osg::FrameBufferObject* fbo );
    const osg::FrameBufferObject& getFBO() const;


    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;

protected:
    ~ShadowMapStage();

    void internalInit();

    static osg::Matrixf computeLightMatrix( const osg::Vec4 pos, const osg::Matrix& view );
    osg::ref_ptr< osg::StateSet > _viewProjStateSet;
    osg::ref_ptr< osg::StateSet > _depthTexStateSet;

    backdropFX::ShadowMap* _shadowMapNode;


    struct ShadowInfo : public osg::Object
    {
        ShadowInfo();
        ShadowInfo( const ShadowInfo& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
        META_Object(backdropFX,ShadowInfo);

        void internalInit();

        void resizeGLObjectBuffers( unsigned int maxSize );
        void releaseGLObjects( osg::State* state );

        osg::ref_ptr< osg::Texture2D > _depthTex;
        osg::ref_ptr< osg::FrameBufferObject > _depthFBO;
        osg::ref_ptr< osg::Viewport > _viewport;
    };
    typedef std::vector< osg::ref_ptr< ShadowInfo > > ShadowInfoVec;

    ShadowInfoVec _shadowInfoVec;


    osg::ref_ptr< osg::FrameBufferObject > _fbo;
};



// namespace backdropFX
}

// __BACKDROPFX_SHADOW_MAP_STAGE_H__
#endif
