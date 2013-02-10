// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/Stub.h>
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

    void setRenderStage( osgUtil::CullVisitor* cv, StubStage* rs)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        _renderStageMap[cv] = rs;
    }        

    StubStage* getRenderStage( osgUtil::CullVisitor* cv )
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        return _renderStageMap[cv].get();
    }

    typedef std::map< osgUtil::CullVisitor*, osg::ref_ptr< StubStage > > RenderStageMap;

    OpenThreads::Mutex  _mutex;
    RenderStageMap      _renderStageMap;
};


Stub::Stub()
{
    internalInit();
}
Stub::Stub( const Stub& stub, const osg::CopyOp& copyop )
  : osg::Group( stub, copyop ),
    backdropFX::BackdropCommon( stub, copyop )
{
    internalInit();
}
void
Stub::internalInit()
{
}

Stub::~Stub()
{
}


void
Stub::traverse( osg::NodeVisitor& nv )
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
        UTIL_MEMORY_CHECK( rsCache, "Stub StubStage Cache", );
        setRenderingCache( rsCache.get() );
    }

    osg::ref_ptr< StubStage > stubs = rsCache->getRenderStage( cv );
    if( !stubs )
    {
        stubs = new StubStage( *previousStage );
        UTIL_MEMORY_CHECK( rsCache, "Stub StubStage", );
        stubs->setBackdropCommon( this );
        rsCache->setRenderStage( cv, stubs.get() );
    }
    else
    {
        // Reusing custom RenderStage. Reset it to clear previous cull's contents.
        stubs->reset();
    }

    // PSC TBD. I don't think we need this.
    //stubs->setInheritedPositionalStateContainer( previousStage->getPositionalStateContainer() );
    // Instead, remove PSC from parent. If we don't do this, OSG
    // seems to revert to a default 800x600 viewport before processing
    // the RenderFX stage.
    previousStage->setPositionalStateContainer( NULL );


    {
        // Save RenderBin
        osgUtil::RenderBin* previousRenderBin = cv->getCurrentRenderBin();
        cv->setCurrentRenderBin( stubs.get() );

        // Traverse
        osg::Group::traverse( nv );

        // Restore RenderBin
        cv->setCurrentRenderBin( previousRenderBin );
    }


    // Hook our RenderStage into the render graph.
    // TBD Might need to support RenderOrder a la Camera node. For now, post-render.
    cv->getCurrentRenderBin()->getStage()->addPreRenderStage( stubs.get(), camera->getRenderOrderNum() );
}



void
Stub::resizeGLObjectBuffers( unsigned int maxSize )
{
    if( _renderingCache.valid() )
        const_cast< Stub* >( this )->_renderingCache->resizeGLObjectBuffers( maxSize );

    osg::Group::resizeGLObjectBuffers(maxSize);
}

void
Stub::releaseGLObjects( osg::State* state ) const
{
    if( _renderingCache.valid() )
        const_cast< Stub* >( this )->_renderingCache->releaseGLObjects( state );

    osg::Group::releaseGLObjects(state);
}


// namespace backdropFX
}
