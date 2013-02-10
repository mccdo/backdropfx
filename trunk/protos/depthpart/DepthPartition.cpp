// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include "DepthPartition.h" //<backdropFX/DepthPartition.h>
#include "DepthPartitionStage.h" //<backdropFX/DepthPartitionStage.h>
#include <osgDB/FileUtils>
#include <osgUtil/CullVisitor>
#include <osgwTools/Shapes.h>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/Texture2D>

#include <backdropFX/Utils.h>


namespace backdropFX
{



// Lifted directly from CullVisitor
class RenderStageCache : public osg::Object
{
public:
    RenderStageCache() {}
    RenderStageCache(const RenderStageCache&, const osg::CopyOp&) {}

    META_Object(myLib,RenderStageCache);

    void setRenderStage( osgUtil::CullVisitor* cv, DepthPartitionStage* rs)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        _renderStageMap[cv] = rs;
    }        

    DepthPartitionStage* getRenderStage( osgUtil::CullVisitor* cv )
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        return _renderStageMap[cv].get();
    }

    typedef std::map< osgUtil::CullVisitor*, osg::ref_ptr< DepthPartitionStage > > RenderStageMap;

    OpenThreads::Mutex  _mutex;
    RenderStageMap      _renderStageMap;
};


DepthPartition::DepthPartition()
  : _numPartitions( 0 )
{
    internalInit();
}
DepthPartition::DepthPartition( const DepthPartition& dp, const osg::CopyOp& copyop )
  : osg::Group( dp, copyop ),
    backdropFX::BackdropCommon( dp, copyop ),
    _numPartitions( dp._numPartitions )
{
    internalInit();
}
void
DepthPartition::internalInit()
{
}

DepthPartition::~DepthPartition()
{
}


void
DepthPartition::traverse( osg::NodeVisitor& nv )
{
    if( ( nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR ) )
    {
        // Not the cull visitor. Traverse as a Group and return.
        osg::Group::traverse( nv );
        return;
    }

    // Dealing with cull traversal from this point onwards.
    osgUtil::CullVisitor* cv = static_cast< osgUtil::CullVisitor* >( &nv );

    // BackdropCommon cull processing.
    processCull( cv );


    //
    // Basic idea of what follows was derived from CullVisitor::apply( Camera& ).
    //

    osgUtil::RenderStage* previousStage = cv->getCurrentRenderBin()->getStage();
    osg::Camera* camera = previousStage->getCamera();

    osg::ref_ptr< RenderStageCache > rsCache = dynamic_cast< RenderStageCache* >( getRenderingCache() );
    if( !rsCache )
    {
        rsCache = new RenderStageCache;
        UTIL_MEMORY_CHECK( rsCache, "DepthPartition DepthPartitionStage Cache", );
        setRenderingCache( rsCache.get() );
    }

    osg::ref_ptr< DepthPartitionStage > dpStage = rsCache->getRenderStage( cv );
    if( !dpStage )
    {
        dpStage = new DepthPartitionStage( *previousStage );
        UTIL_MEMORY_CHECK( rsCache, "DepthPartition DepthPartitionStage", );
        dpStage->setDepthPartition( this );
        rsCache->setRenderStage( cv, dpStage.get() );
    }
    else
    {
        // Reusing custom RenderStage. Reset it to clear previous cull's contents.
        dpStage->reset();
    }

    // PSC TBD. I don't think we need this.
    //dpStage->setInheritedPositionalStateContainer( previousStage->getPositionalStateContainer() );
    // Instead, remove PSC from parent. If we don't do this, OSG
    // seems to revert to a default 800x600 viewport before processing
    // the RenderFX stage.
    previousStage->setPositionalStateContainer( NULL );


    {
        // Save RenderBin
        osgUtil::RenderBin* previousRenderBin = cv->getCurrentRenderBin();
        cv->setCurrentRenderBin( dpStage.get() );

        // Add a per-cull uniform for the partition matrix.
        cv->pushStateSet( dpStage->getPerCullStateSet() );

        // Traverse
        osg::Group::traverse( nv );

        // Pop the per-cull uniform.
        cv->popStateSet();

        // Restore RenderBin
        cv->setCurrentRenderBin( previousRenderBin );
    }


    // Hook our RenderStage into the render graph.
    cv->getCurrentRenderBin()->getStage()->addPreRenderStage( dpStage.get(), camera->getRenderOrderNum() );
}


void
DepthPartition::setNumPartitions( unsigned int numPartitions )
{
    _numPartitions = numPartitions;
}
unsigned int
DepthPartition::getNumPartitions() const
{
    return( _numPartitions );
}


void
DepthPartition::resizeGLObjectBuffers( unsigned int maxSize )
{
    if( _renderingCache.valid() )
        const_cast< DepthPartition* >( this )->_renderingCache->resizeGLObjectBuffers( maxSize );

    osg::Group::resizeGLObjectBuffers(maxSize);
}

void
DepthPartition::releaseGLObjects( osg::State* state ) const
{
    if( _renderingCache.valid() )
        const_cast< DepthPartition* >( this )->_renderingCache->releaseGLObjects( state );

    osg::Group::releaseGLObjects(state);
}


// namespace backdropFX
}
