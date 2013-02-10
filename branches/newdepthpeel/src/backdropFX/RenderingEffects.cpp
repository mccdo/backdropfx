// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/RenderingEffects.h>
#include <osgUtil/CullVisitor>
#include <osgDB/FileUtils>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/Texture2D>

#include <osgwTools/Shapes.h>
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

    void setRenderStage( osgUtil::CullVisitor* cv, RenderingEffectsStage* rs)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        _renderStageMap[cv] = rs;
    }        

    RenderingEffectsStage* getRenderStage( osgUtil::CullVisitor* cv )
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        return _renderStageMap[cv].get();
    }

    typedef std::map< osgUtil::CullVisitor*, osg::ref_ptr< RenderingEffectsStage > > RenderStageMap;

    OpenThreads::Mutex  _mutex;
    RenderStageMap      _renderStageMap;
};


RenderingEffects::RenderingEffects()
  : _defaultEffect( NULL )
{
    internalInit();
}
RenderingEffects::RenderingEffects( const RenderingEffects& renderFX, const osg::CopyOp& copyop )
  : osg::Group( renderFX, copyop ),
    backdropFX::BackdropCommon( renderFX, copyop ),
    _effectVector( renderFX._effectVector ),
    _defaultEffect( renderFX._defaultEffect )
{
    internalInit();
}

void
RenderingEffects::internalInit()
{
}


RenderingEffects::~RenderingEffects()
{
}


backdropFX::Effect*
RenderingEffects::getEffect( const std::string& className )
{
    EffectVector::iterator it;
    for( it = _effectVector.begin(); it != _effectVector.end(); it++ )
    {
        if( std::string( (*it)->className() ) == className )
            break;
    }

    if( it == _effectVector.end() )
    {
        return( NULL );
    }
    else
    {
        return( (*it).get() );
    }
}
bool
RenderingEffects::removeEffect( const std::string& className )
{
    EffectVector::iterator it;
    for( it = _effectVector.begin(); it != _effectVector.end(); it++ )
    {
        if( std::string( (*it)->className() ) == className )
            break;
    }

    if( it == _effectVector.end() )
    {
        // Not found.
        return( false );
    }
    else
    {
        _effectVector.erase( it );
        return( true );
    }
}



void
RenderingEffects::traverse( osg::NodeVisitor& nv )
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
        UTIL_MEMORY_CHECK( rsCache, "RenderFX RenderingEffectsStage Cache", );
        setRenderingCache( rsCache.get() );
    }

    osg::ref_ptr< RenderingEffectsStage > rfxs = rsCache->getRenderStage( cv );
    if( !rfxs )
    {
        rfxs = new RenderingEffectsStage( *previousStage );
        UTIL_MEMORY_CHECK( rsCache, "RenderFX RenderingEffectsStage", );
        rfxs->setRenderingEffects( this );
        rsCache->setRenderStage( cv, rfxs.get() );
    }
    else
    {
        // Reusing custom RenderStage. Reset it to clear previous cull's contents.
        rfxs->reset();
    }

    // Seems to be required, otherwise OSG defaults to
    // an 800x600 viewport.
    previousStage->setPositionalStateContainer( NULL );

    {
        // Save RenderBin
        osgUtil::RenderBin* previousRenderBin = cv->getCurrentRenderBin();
        cv->setCurrentRenderBin( rfxs.get() );

        // Traverse
        osg::Group::traverse( nv );

        // Restore RenderBin
        cv->setCurrentRenderBin( previousRenderBin );
    }


    // Hook our RenderStage into the render graph.
    // TBD
    //   IF app has specified a destination to the Manager,
    // then we need to be prereneder. The app is then responsible for
    // splatting a fullscreen triangle pair into the window during the
    // top-level Camera/RenderStage draw operation.
    //   ELSE: If app hasn't specified a distination, and we are to
    // draw into the window, then add as postrender.
    cv->getCurrentRenderBin()->getStage()->addPostRenderStage( rfxs.get(), camera->getRenderOrderNum() );
}


void
RenderingEffects::resizeGLObjectBuffers( unsigned int maxSize )
{
    if( _renderingCache.valid() )
        const_cast< RenderingEffects* >( this )->_renderingCache->resizeGLObjectBuffers( maxSize );

    osg::Group::resizeGLObjectBuffers(maxSize);
}

void
RenderingEffects::releaseGLObjects( osg::State* state ) const
{
    if( _renderingCache.valid() )
        const_cast< RenderingEffects* >( this )->_renderingCache->releaseGLObjects( state );

    osg::Group::releaseGLObjects(state);
}


// namespace backdropFX
}
