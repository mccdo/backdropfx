// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_RENDERING_EFFECTS_H__
#define __BACKDROPFX_RENDERING_EFFECTS_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/BackdropCommon.h>
#include <backdropFX/Effect.h>
#include <osg/Group>
#include <osg/Uniform>
#include <osg/FrameBufferObject>

#include <backdropFX/RenderingEffectsStage.h>

#include <vector>


namespace backdropFX {


/** \class backdropFX::RenderingEffects RenderingEffects.h backdropFX/RenderingEffects.h

\brief An OSG Node for processing a list of Effects.

The application adds Effects to the EffectVector.
During cull, RenderingEffects inserts a RenderingEffectsStage into
the OSG render graph. During draw, RenderingEffectsStage processes
each Effect in the EffectVector in sequence.

Typical usage is as a post-rendering effects pipeline to implement
glow, depth of field, tone mapping, glare/bloom, and other effects
that a screen space algorithm can implement.

\b Note: The code in RenderingEffects.cpp/.h, RenderingEffectsStage.cpp/.h,
Effect.cpp/.h, and EffectLibrary.cpp/.h has minimal
dependencies on backdropFX, so you can potentially port it to a
standalone library. The RenderingEffectsUtils.cpp/.h supplies functions  
to bridge the gap between this rendering effects system and the backdropFX
Manager class.
*/
class BACKDROPFX_EXPORT RenderingEffects : public osg::Group, public backdropFX::BackdropCommon
{
public:
    RenderingEffects();
    RenderingEffects( const RenderingEffects& renderFX, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    META_Node( backdropFX, RenderingEffects );


    //
    // Managed interface
    //

    /** Effect flag definitions. Pass these flags to setEffectSet() to specify
    the Effects to use. Manager works with its managed RenderingEffects
    class to ensure the desired effects are added to the EffectVector, their
    outputs and inputs are correctly attached, and their effects pass uniforms
    are set in the effect camera StateSet.
    */
    static const unsigned int effectGlow;
    static const unsigned int effectDOF;
    static const unsigned int effectToneMapping;
    static const unsigned int effectHeatDistortion;
    static const unsigned int effectLensFlare;

    /** Managed access to supported Effects. Specify a bitwise OR of the
    flags for each desired Effect.
    */
    void setEffectSet( unsigned int effectSet );

    /** Get the current set of Effect flags.
    */
    unsigned int getEffectSet() const;

    /** Called by setEffectSet() to attach the
    output of Effect E(i-1) to input unit 0 of Effect E(i).
    For E(0), ColorBufferA is attached to input unit 0.
    Your application should not need to call this function directly.
    */
    void attachAllEffects();

    /** Control how the RenderingEffectsStage is attached into the render graph.*/
    typedef enum {
        PRE_RENDER,
        POST_RENDER
    } RenderOrder;
    /** \brief Control how the RenderingEffectsStage is attached into the render graph.
    We can attach RenderingEffectsState as either a pre- or post-render stage.

    Pre-render works under the following conditions:
    \li The parent camera must not perform a clear (because it will execute
    after all pre-render stages, which would erase what we've drawn).
    \li For osgViewer slave camera scenarios, *all* slaves must have clearing
    disabled.

    Note that pre-render is required for RTT usage. The top-level Camera
    will display a textured tri pair, probably, and RenderingEffects must
    render before the tri pair.

    Post-render can be used in this situation:
    \li Using osgViewer slave cameras, and not performing RTT.

    The default is to attach as a pre-render stage. */
    void setRenderOrder( RenderOrder renderOrder ) { _renderOrder = renderOrder; }
    RenderOrder getRenderOrder() const { return( _renderOrder ); }

    //
    // Direct (unmanaged) interface
    //

    /** Direct access to the EffectVector. Applications can add
    and remove Effects from the EffectVector while outside the
    cull and draw phases.
    */
    EffectVector& getEffectVector() { return( _effectVector ); }



    /** Set and get the default Effect. RenderingEffects uses the default Effect only when
    the EffectVector is empty, and is otherwise ignores it. Initially, the default
    Effect is NULL. When the default Effect is NULL and the EffectVector is empty,
    RenderingEffects behaves like a RenderBin and draws any attached children.
    (Note RenderingEffects ingnores attached children if either the EffectVector is not empty,
    or there is a default Effect.)
    */
    void setDefaultEffect( Effect* defaultEffect ) { _defaultEffect = defaultEffect; }
    Effect* getDefaultEffect() { return( _defaultEffect.get() ); }

    /** Direct access to the global uniform vector. Applications can add,
    update, and remove uniforms while outside the cull and draw phases.
    */
    typedef std::vector< osg::ref_ptr< osg::Uniform > > UniformVector;
    UniformVector& getGlobalUniformVector() { return( _globalUniformVector ); }

    osg::Uniform* getGlobalUniform( const std::string& objectName );
    bool removeGlobalUniform( const std::string& objectName );
    void applyAllGlobalUniforms( osg::State& state, osg::GL2Extensions* gl2Ext );

    /** Override for base class traverse(). During cull, this function inserts
    a RenderingEffectsStage (custom RenderStage) into the OSG render graph.
    */
    void traverse( osg::NodeVisitor& nv );

    /** \brief Specify texture dimensions. This function calls Effect::setTextureWidthHeight() on all Effects in the EffectVector.
    This function is called implicitly by Manager::setTextureWidthHeight(), so applications should
    not have to call this function directly. In typical usage, pass the window width and height.
    For SceneView multi-view rendering, pass the largest viewport width and height, ignoring x and y.
    */
    void setTextureWidthHeight( unsigned int texW, unsigned int texH );
    void getTextureWidthHeight( unsigned int& texW, unsigned int& texH ) const;


    //
    // For internal use
    //

    void setRenderingCache( osg::Object* rc ) { _renderingCache = rc; }
    osg::Object* getRenderingCache() { return _renderingCache.get(); }
    const osg::Object* getRenderingCache() const { return _renderingCache.get(); }

    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;

protected:
    ~RenderingEffects();
    void internalInit();


    EffectVector _effectVector;
    osg::ref_ptr< Effect > _defaultEffect;
    unsigned int _effectSet;

    UniformVector _globalUniformVector;

    RenderOrder _renderOrder;

    unsigned int _texW, _texH;

    osg::ref_ptr< osg::Object > _renderingCache;
};


// namespace backdropFX
}

// __BACKDROPFX_RENDERING_EFFECTS_H__
#endif
