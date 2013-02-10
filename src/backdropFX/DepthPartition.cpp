// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/DepthPartition.h>
#include <backdropFX/DepthPartitionStage.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/Manager.h>
#include <backdropFX/ShadowMap.h>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
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
  : _numPartitions( 0 ),
    _ratio( 0.0005 ),
    _proto( false )
{
    internalInit();
}
DepthPartition::DepthPartition( const DepthPartition& dp, const osg::CopyOp& copyop )
  : osg::Group( dp, copyop ),
    backdropFX::BackdropCommon( dp, copyop ),
    _numPartitions( dp._numPartitions ),
    _ratio( 0.0005 ),
    _proto( false )
{
    internalInit();
}
void
DepthPartition::internalInit()
{
    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *this );
    UTIL_MEMORY_CHECK( smccb, "DepthPartition internalInit SMCCB", );

    osg::ref_ptr< osg::Shader > shader;
    std::string fileName( "shaders/gl2/bdfx-init.vs" );
    __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
    UTIL_MEMORY_CHECK( shader, "DepthPartition internalInit bdfx-init.vs", );
    smccb->setShader( getShaderSemantic( fileName ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );
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

    dpStage->setViewport( cv->getCurrentCamera()->getViewport() );
    dpStage->setCamera( camera );


    // See comments in ShadowMap.cpp where it allocates StateSetCache
    // for a further explanation of what we're doing in this code.
    ShadowMap& smn = Manager::instance()->getShadowMap();
    osg::ref_ptr< osg::StateSet > smStateSet = smn.getDepthTexStateSet( cv );
    if( smStateSet.valid() )
    {
        // This works well: if the StateSet isn't valid, we do minimal additional work.
        // An alternate solution is that Manager could keep its rebuild flags, and we'd
        // know that shadows are enabled or disabled. But this is good enough for now.
        cv->pushStateSet( smStateSet.get() );
    }


    // PSC TBD. I don't think we need this.
    //dpStage->setInheritedPositionalStateContainer( previousStage->getPositionalStateContainer() );
    // Instead, remove PSC from parent. If we don't do this, OSG
    // seems to revert to a default 800x600 viewport before processing
    // the RenderFX stage.
    //previousStage->setPositionalStateContainer( NULL );


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

    if( smStateSet.valid() )
        cv->popStateSet();

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
DepthPartition::setRatio( double ratio )
{
    _ratio = ratio;
}
double
DepthPartition::getRatio() const
{
    return( _ratio );
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
