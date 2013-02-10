// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/RenderingEffects.h>
#include <backdropFX/Manager.h>
#include <backdropFX/EffectLibrary.h>
#include <osgUtil/CullVisitor>
#include <osgDB/FileUtils>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Depth>

#include <osgwTools/Shapes.h>
#include <backdropFX/Utils.h>


namespace backdropFX
{


const unsigned int RenderingEffects::effectGlow           ( 1u <<  0 );
const unsigned int RenderingEffects::effectDOF            ( 1u <<  1 );
const unsigned int RenderingEffects::effectToneMapping    ( 1u <<  2 );
const unsigned int RenderingEffects::effectHeatDistortion ( 1u <<  3 );
const unsigned int RenderingEffects::effectLensFlare      ( 1u <<  4 );


/** \cond */
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

class RenderingEffectsUpdate : public osg::NodeCallback
{
public:
    RenderingEffectsUpdate() {}

    void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        const osg::FrameStamp* fs = nv->getFrameStamp();
        if( fs == NULL )
        {
            osg::notify( osg::WARN ) << "bdfx: RenderingEffects: No FrameStamp." << std::endl;
            return;
        }

        backdropFX::RenderingEffects* rfx = static_cast< backdropFX::RenderingEffects* >( node );
        osg::Uniform* u;
        if( u = rfx->getGlobalUniform( "rfx_simulationTime" ) )
            u->set( (float)( fs->getSimulationTime() ) );

        traverse( node, nv );
    }
};
/** \endcond */



RenderingEffects::RenderingEffects()
  : _defaultEffect( NULL ),
    _effectSet( 0 ),
    _renderOrder( PRE_RENDER ),
    _texW( 0 ),
    _texH( 0 )
{
    internalInit();
}
RenderingEffects::RenderingEffects( const RenderingEffects& renderFX, const osg::CopyOp& copyop )
  : osg::Group( renderFX, copyop ),
    backdropFX::BackdropCommon( renderFX, copyop ),
    _effectVector( renderFX._effectVector ),
    _defaultEffect( renderFX._defaultEffect ),
    _effectSet( renderFX._effectSet ),
    _renderOrder( renderFX._renderOrder ),
    _texW( renderFX._texW ),
    _texH( renderFX._texH )
{
    internalInit();
}

void
RenderingEffects::internalInit()
{
    osg::ref_ptr< osg::Uniform > simTimeUniform = new osg::Uniform( osg::Uniform::FLOAT, "rfx_simulationTime" );
    UTIL_MEMORY_CHECK( simTimeUniform, "RenderFX simTimeUniform", );
    simTimeUniform->set( 0.f );
    getGlobalUniformVector().push_back( simTimeUniform );

    setUpdateCallback( new RenderingEffectsUpdate() );
}


RenderingEffects::~RenderingEffects()
{
}



void RenderingEffects::setEffectSet( unsigned int effectSet )
{
    if( _effectSet == effectSet )
        // No change.
        return;

    backdropFX::Manager& mgr = *( backdropFX::Manager::instance() );
    unsigned int texW, texH;
    mgr.getTextureWidthHeight( texW, texH );

    // General algoritm is to step through each bit in effectSet, and add
    // or remove each effect in _effectVector, setting or clearing corresponding
    // bits in _effectSet as we go, until _effectSet == effectSet.

    unsigned int testBit = 1u;
    EffectVector::iterator insertIt = _effectVector.begin();
    while( _effectSet != effectSet )
    {
        unsigned int newState = effectSet & testBit;
        unsigned int currentState = _effectSet & testBit;
        if( newState == currentState )
        {
            // The bits match (either set or clear), so no change.
            // If the bit is set, advance the iterator to reference the next Effect.
            if( currentState != 0 )
                insertIt++;
            testBit <<= 1;
            continue;
        }

        if( ( newState == 0 ) && ( currentState != 0 ) )
        {
            // Remove an Effect.
            // Remove any effects pass uniforms from the manages effects pass Camera.
            mgr.removeEffectsPassUniforms( (*insertIt)->getEffectsPassUniforms() );
            insertIt = _effectVector.erase( insertIt );
            _effectSet &= ~testBit;
        }
        else
        {
            // Insert an effect.
            Effect* insertEffect( NULL );
            switch( testBit )
            {
            case effectGlow:
                insertEffect = new backdropFX::EffectImprovedGlow();
                //insertEffect = new backdropFX::EffectBasicGlow();
                UTIL_MEMORY_CHECK( insertEffect, "setEffectSet EffectImprovedGlow", );
                insertEffect->setName( "EffectImprovedGlow" );
                insertEffect->addInput( 1, mgr.getColorBufferGlow() );
                break;
            case effectDOF:
                insertEffect = new backdropFX::EffectDOF();
                UTIL_MEMORY_CHECK( insertEffect, "setEffectSet EffectDOF", );
                insertEffect->setName( "EffectDOF" );
                break;
            case effectToneMapping:
                osg::notify( osg::WARN ) << "backdropFX: setEffectSet: effectLensFlare is TBD" << testBit << std::endl;
                break;
            case effectHeatDistortion:
                insertEffect = new backdropFX::EffectHeatDistortion();
                UTIL_MEMORY_CHECK( insertEffect, "setEffectSet EffectHeatDistortion", );
                insertEffect->setName( "EffectHeatDistortion" );
                break;
            case effectLensFlare:
                osg::notify( osg::WARN ) << "backdropFX: setEffectSet: effectLensFlare is TBD" << testBit << std::endl;
                break;
            default:
                osg::notify( osg::WARN ) << "backdropFX: setEffectSet: Invalid bit 0x" << std::hex << testBit << std::endl;
                break;
            }

            // Set any effects pass uniforms in the managed effects pass Camera.
            mgr.addEffectsPassUniforms( insertEffect->getEffectsPassUniforms() );

            // Update the Effect's texture size.
            insertEffect->setTextureWidthHeight( texW, texH );

            insertIt = _effectVector.insert( insertIt, insertEffect );
            // insert() returns an iterator point to what we just inserted, so advance it.
            insertIt++;

            _effectSet |= testBit;
        }
        testBit <<= 1;
    }

    // The managed effects Camera needs to be enabled for glow, DOF, and heat distortion.
    // Otherwise, it can be disabled.
    osg::Camera& effectsCam = mgr.getEffectsCamera();
    if( ( _effectSet & effectGlow ) || 
        ( _effectSet & effectDOF ) || 
        ( _effectSet & effectHeatDistortion ) )
    {
        effectsCam.setNodeMask( 0xffffffff );
    }
    else
    {
        effectsCam.setNodeMask( 0x0 );
    }

    // Sequentially attach preceding Effect output to succeeding Effect input 0.
    // First Effect input 0 is ColorBufferA. Last Effect's output is RenderingEffects FBO.
    attachAllEffects();
}

unsigned int RenderingEffects::getEffectSet() const
{
    return( _effectSet );
}

void RenderingEffects::attachAllEffects()
{
    backdropFX::Manager& mgr = *( backdropFX::Manager::instance() );
    EffectVector& ev = getEffectVector();

    EffectVector::iterator it, prevIt( ev.end() );
    for( it = ev.begin(); it != ev.end(); it++ )
    {
        if( it == ev.begin() )
        {
            // First Effect.
            (*it)->addInput( 0, mgr.getColorBufferA() );
        }
        else
        {
            (*prevIt)->attachOutputTo( (*it).get(), 0 );
        }
        prevIt = it;
    }

    if( prevIt != ev.end() )
        (*prevIt)->setOutput( getFBO() );
}



osg::Uniform* RenderingEffects::getGlobalUniform( const std::string& objectName )
{
    UniformVector::iterator it;
    for( it = _globalUniformVector.begin(); it != _globalUniformVector.end(); it++ )
    {
        if( std::string( (*it)->getName() ) == objectName )
            break;
    }

    if( it == _globalUniformVector.end() )
    {
        return( NULL );
    }
    else
    {
        return( (*it).get() );
    }
}
bool RenderingEffects::removeGlobalUniform( const std::string& objectName )
{
    UniformVector::iterator it;
    for( it = _globalUniformVector.begin(); it != _globalUniformVector.end(); it++ )
    {
        if( std::string( (*it)->getName() ) == objectName )
            break;
    }

    if( it == _globalUniformVector.end() )
    {
        // Not found.
        return( false );
    }
    else
    {
        _globalUniformVector.erase( it );
        return( true );
    }
}


void RenderingEffects::applyAllGlobalUniforms( osg::State& state, osg::GL2Extensions* gl2Ext )
{
    UniformVector::iterator it;
    for( it = _globalUniformVector.begin(); it != _globalUniformVector.end(); it++ )
    {
        osg::Uniform* uniform = (*it).get();
#if OSG_SUPPORTS_UNIFORM_ID
        GLint location = state.getUniformLocation( uniform->getNameID() );
#else
        GLint location = state.getUniformLocation( uniform->getName() );
#endif
        if( location >= 0 )
            uniform->apply( gl2Ext, location );
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

    rfxs->setViewport( cv->getCurrentCamera()->getViewport() );

    // PSC TBD Seems to be required, otherwise OSG defaults to
    // an 800x600 viewport.
    //previousStage->setPositionalStateContainer( NULL );

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
    if( _renderOrder == POST_RENDER )
        cv->getCurrentRenderBin()->getStage()->addPostRenderStage( rfxs.get(), camera->getRenderOrderNum() );
    else
        cv->getCurrentRenderBin()->getStage()->addPreRenderStage( rfxs.get(), camera->getRenderOrderNum() );
}


void
RenderingEffects::setTextureWidthHeight( unsigned int texW, unsigned int texH )
{
    if( ( _texW != texW ) || ( _texH != texH ) )
    {
        _texW = texW;
        _texH = texH;

        if( getDefaultEffect() != NULL )
            getDefaultEffect()->setTextureWidthHeight( _texW, _texH );

        backdropFX::EffectVector& ev = getEffectVector();
        backdropFX::EffectVector::iterator it;
        for( it = ev.begin(); it != ev.end(); it++ )
            (*it)->setTextureWidthHeight( _texW, _texH );
    }
}
void
RenderingEffects::getTextureWidthHeight( unsigned int& texW, unsigned int& texH ) const
{
    texW = _texW;
    texH = _texH;
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
