// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_LIGHTINFO_H__
#define __BACKDROPFX_LIGHTINFO_H__ 1

#include <osg/Texture2D>
#include <osg/FrameBufferObject>
#include <osg/Light>

#include <backdropFX/ShadowMapStage.h>

#include <vector>



namespace backdropFX {



struct LightInfo : public osg::Object
{
    LightInfo();
    LightInfo( const LightInfo& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
    META_Object(backdropFX,LightInfo);

    // Inputs to ShadowMap.
    osg::ref_ptr< osg::Light > _light;
    bool _enable;
};
typedef std::vector< osg::ref_ptr< LightInfo > > LightInfoVec;



// namespace backdropFX
}

// __BACKDROPFX_LIGHTINFO_H__
#endif
