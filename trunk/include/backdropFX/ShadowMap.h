// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SHADOWMAP_H__
#define __BACKDROPFX_SHADOWMAP_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/BackdropCommon.h>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <backdropFX/ShadowMapStage.h>

#include <vector>



namespace backdropFX {



/**
We'll specify one or more LightInfo structs that contain the light position.
During draw, ShadowMapStage will do an RTT for each, storing the texture address.
It'll compute the view based on the light direction and a point computed to be
in front of the viewer. The view matrix also needs to be stored in the LightInfo
struct.

As the scene is rendered by DepthPeel, a vertex shader module will compute texture
coordinates using the stored view matrix per light. A fragment shader module will
then perform the depth map lookup and z compare, and color the fragment accordingly.
*/
class BACKDROPFX_EXPORT ShadowMap : public osg::Group, public backdropFX::BackdropCommon
{
public:
    ShadowMap();
    ShadowMap( const ShadowMap& ShadowMap, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    META_Node( backdropFX, ShadowMap );

    void traverse( osg::NodeVisitor& nv );

    osg::StateSet* getViewProjStateSet( osgUtil::CullVisitor* cv );
    osg::Uniform* getViewProjUniform( osgUtil::CullVisitor* cv );

    osg::StateSet* getDepthTexStateSet( osgUtil::CullVisitor* cv );


    //
    // For internal use
    //

    void setRenderingCache( osg::Object* rc ) { _renderingCache = rc; }
    osg::Object* getRenderingCache() { return _renderingCache.get(); }
    const osg::Object* getRenderingCache() const { return _renderingCache.get(); }

    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;

protected:
    ~ShadowMap();
    void internalInit();

    osg::ref_ptr< osg::Object > _renderingCache;
};



// namespace backdropFX
}

// __BACKDROPFX_ShadowMap_H__
#endif
