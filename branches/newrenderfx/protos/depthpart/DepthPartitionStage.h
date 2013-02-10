// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_DEPTH_PARTITION_STAGE_H__
#define __BACKDROPFX_DEPTH_PARTITION_STAGE_H__ 1


#include <osgUtil/RenderStage>
#include <osg/FrameBufferObject>
#include <osg/Version>

#include <string>



namespace backdropFX
{


class DepthPartition;

class DepthPartitionStage : public osgUtil::RenderStage
{
public:
    DepthPartitionStage();
    DepthPartitionStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    DepthPartitionStage( const DepthPartitionStage& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new DepthPartitionStage(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new DepthPartitionStage( *this, copyop ); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const DepthPartitionStage*>(obj)!=0L; }
    virtual const char* className() const { return "DepthPartitionStage"; }

    virtual void draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    void setDepthPartition( DepthPartition* depthPartition );

    osg::StateSet* getPerCullStateSet();

protected:
    ~DepthPartitionStage();
    void internalInit();

    DepthPartition* _depthPartition;

    osg::ref_ptr< osg::StateSet > _stateSet;
    osg::ref_ptr< osg::Uniform > _partitionMatrix;
};


// namespace backdropFX
}

// __BACKDROPFX_DEPTH_PARTITION_STAGE_H__
#endif
