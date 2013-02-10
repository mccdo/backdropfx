// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <backdropFX/ShadowMap.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <osgDB/FileUtils>
#include <osgUtil/CullVisitor>
#include <osgwTools/Shapes.h>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/PolygonOffset>
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

    void setRenderStage( osgUtil::CullVisitor* cv, ShadowMapStage* rs)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        _renderStageMap[cv] = rs;
    }        
    ShadowMapStage* getRenderStage( osgUtil::CullVisitor* cv )
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        return _renderStageMap[cv].get();
    }

    void resizeGLObjectBuffers( unsigned int maxSize )
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        RenderStageMap::iterator itr;
        for( itr=_renderStageMap.begin(); itr != _renderStageMap.end(); itr++ )
            itr->second->resizeGLObjectBuffers( maxSize );
    }
    void releaseGLObjects( osg::State* state ) const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        RenderStageMap::const_iterator itr;
        for( itr=_renderStageMap.begin(); itr != _renderStageMap.end(); itr++ )
            itr->second->releaseGLObjects( state );
    }

    typedef std::map< osgUtil::CullVisitor*, osg::ref_ptr< ShadowMapStage > > RenderStageMap;

    mutable OpenThreads::Mutex  _mutex;
    RenderStageMap      _renderStageMap;
};



ShadowMap::ShadowMap()
{
    internalInit();
}
ShadowMap::ShadowMap( const ShadowMap& shadowMap, const osg::CopyOp& copyop )
  : osg::Group( shadowMap, copyop ),
    backdropFX::BackdropCommon( shadowMap, copyop )
{
    internalInit();
}
void ShadowMap::internalInit()
{
    osg::StateSet* stateSet = getOrCreateStateSet();

    // When we render, push back into the depth buffer slightly
    // to avoid aliasing during the main rendering pass.
    osg::PolygonOffset* po = new osg::PolygonOffset( 3., 6. );
    stateSet->setAttributeAndModes( po );


    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *this );

    osg::ref_ptr< osg::Shader > shader;
    std::string fileName = std::string( "shaders/gl2/shadowmap-main.vs" );
    __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
    UTIL_MEMORY_CHECK( shader, "ShadowMap internalInit shadowmap-main.vs", );
    smccb->setShader( backdropFX::getShaderSemantic( shader->getName() ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );

    fileName = std::string( "shaders/gl2/shadowmap-main.fs" );
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "ShadowMap internalInit shadowmap-main.fs", );
    smccb->setShader( backdropFX::getShaderSemantic( shader->getName() ), shader.get(),
        ShaderModuleCullCallback::InheritanceOverride );
}

ShadowMap::~ShadowMap()
{
}


void ShadowMap::traverse( osg::NodeVisitor& nv )
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
        UTIL_MEMORY_CHECK( rsCache, "ShadowMap ShadowMapStage Cache", );
        setRenderingCache( rsCache.get() );
    }

    osg::ref_ptr< ShadowMapStage > sms = rsCache->getRenderStage( cv );
    if( !sms )
    {
        sms = new ShadowMapStage( *previousStage );
        UTIL_MEMORY_CHECK( sms, "ShadowMap ShadowMapStage", );
        sms->setShadowMapNode( this );
        rsCache->setRenderStage( cv, sms.get() );
    }
    else
    {
        // Reusing custom RenderStage. Reset it to clear previous cull's contents.
        sms->reset();
    }

    sms->setCamera( camera );

    cv->pushStateSet( sms->getViewProjStateSet() );


    // PSC TBD. I don't think we need this.
    //ShadowMaps->setInheritedPositionalStateContainer( previousStage->getPositionalStateContainer() );
    // Instead, remove PSC from parent. If we don't do this, OSG
    // seems to revert to a default 800x600 viewport before processing
    // the RenderFX stage.
    //previousStage->setPositionalStateContainer( NULL );


    {
        // Save RenderBin
        osgUtil::RenderBin* previousRenderBin = cv->getCurrentRenderBin();
        cv->setCurrentRenderBin( sms.get() );

        // Traverse
        osg::Group::traverse( nv );

        // Restore RenderBin
        cv->setCurrentRenderBin( previousRenderBin );
    }

    cv->popStateSet();

    // Hook our RenderStage into the render graph.
    cv->getCurrentRenderBin()->getStage()->addPreRenderStage( sms.get(), camera->getRenderOrderNum() );
}


osg::StateSet* ShadowMap::getViewProjStateSet( osgUtil::CullVisitor* cv )
{
    osg::ref_ptr< RenderStageCache > rsCache = dynamic_cast< RenderStageCache* >( getRenderingCache() );
    if( !rsCache )
        return( NULL );
    osg::ref_ptr< ShadowMapStage > sms = rsCache->getRenderStage( cv );
    if( !sms )
        return( NULL );

    return( sms->getViewProjStateSet() );
}
osg::Uniform* ShadowMap::getViewProjUniform( osgUtil::CullVisitor* cv )
{
    osg::StateSet* stateSet = getViewProjStateSet( cv );

    const std::string viewProjName( "bdfx_shadowViewProj" );
    return( stateSet->getUniform( viewProjName ) );
}

osg::StateSet* ShadowMap::getDepthTexStateSet( osgUtil::CullVisitor* cv )
{
    osg::ref_ptr< RenderStageCache > rsCache = dynamic_cast< RenderStageCache* >( getRenderingCache() );
    if( !rsCache )
        return( NULL );
    osg::ref_ptr< ShadowMapStage > sms = rsCache->getRenderStage( cv );
    if( !sms )
        return( NULL );

    return( sms->getDepthTexStateSet() );
}



void ShadowMap::resizeGLObjectBuffers( unsigned int maxSize )
{
    if( _renderingCache.valid() )
        const_cast< ShadowMap* >( this )->_renderingCache->resizeGLObjectBuffers( maxSize );

    osg::Group::resizeGLObjectBuffers(maxSize);
}

void ShadowMap::releaseGLObjects( osg::State* state ) const
{
    if( _renderingCache.valid() )
        const_cast< ShadowMap* >( this )->_renderingCache->releaseGLObjects( state );

    osg::Group::releaseGLObjects(state);
}


// namespace backdropFX
}
