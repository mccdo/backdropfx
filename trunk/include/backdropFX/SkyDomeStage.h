// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_SKY_DOME_STAGE_H__
#define __BACKDROPFX_SKY_DOME_STAGE_H__ 1


#include <osgUtil/RenderStage>
#include <osg/FrameBufferObject>
#include <osg/Version>

#include <string>



namespace backdropFX
{


class BackdropCommon;

class SkyDomeStage : public osgUtil::RenderStage
{
public:
    SkyDomeStage();
    SkyDomeStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    SkyDomeStage( const SkyDomeStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new SkyDomeStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new SkyDomeStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const SkyDomeStage*>(obj)!=0L; }
    virtual const char* className() const { return "SkyDomeStage"; }

    virtual void draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    void setBackdropCommon( BackdropCommon* backdropCommon );

    /** When set to false, SkyDomeStage only clears and renders nothing.
    The default is true (enabled). */
    void setEnable( bool enable=true );
    bool getEnable() const { return( _enable ); }

protected:
    ~SkyDomeStage();
    void internalInit();

    std::string createFileName( unsigned int contextID );

    BackdropCommon* _backdropCommon;

    bool _enable;
};


// namespace backdropFX
}

// __BACKDROPFX_SKY_DOME_STAGE_H__
#endif
