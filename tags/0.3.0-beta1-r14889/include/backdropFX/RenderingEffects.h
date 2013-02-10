// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_RENDERING_EFFECTS_H__
#define __BACKDROPFX_RENDERING_EFFECTS_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/BackdropCommon.h>
#include <backdropFX/Effect.h>
#include <osg/Group>
#include <osg/FrameBufferObject>

#include <backdropFX/RenderingEffectsStage.h>


namespace backdropFX {


/** \class backdropFX::RenderingEffects RenderingEffects.h backdropFX/RenderingEffects.h

\brief An OSG Node for processing a list of Effects.

The application adds Effects to the EffectVector.
During cull, RenderingEffects inserts a RenderingEffectsStage into
the OSG render graph. During draw, RenderingEffectsStage processes
each Effect in the EffectsVector in sequence.

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

    /** Direct access to the EffectVector. Applications can add
    and remove Effects from the EffectVector while outside the
    cull and draw phases.
    */
    EffectVector& getEffectVector() { return( _effectVector ); }

    /** Looks in the EffectVector for an Effect with the specified class name.
    \param className Class name to search for, e.g., "EffectGlow".
    \return If found, it's the address of the class name. Otherwise NULL.
    */
    Effect* getEffect( const std::string& className );

    /** Removes the Effect with the specified object name, if found in the EffectVector.
    Deletes a single instance only (it does not look for multiple instances). To delete all
    instances of the same Effect, call removeEffect in a loop, checking the return value.
    \param objectName Object name to search for, e.g., "Glow-Blur".
    \return True if found and deleted, otherwise false.
    */
    bool removeEffect( const std::string& objectName );

    /** Set and get the default Effect. RenderingEffects uses the default Effect only when
    the EffectVector is empty, and is otherwise ignores it. Initially, the default
    Effect is NULL. When the default Effect is NULL and the EffectVector is empty,
    RenderingEffects behaves like a RenderBin and draws any attached children.
    (Note RenderingEffects ingnores attached children if either the EffectVector is not empty,
    or there is a default Effect.)
    */
    void setDefaultEffect( Effect* defaultEffect ) { _defaultEffect = defaultEffect; }
    Effect* getDefaultEffect() { return( _defaultEffect.get() ); }

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

    unsigned int _texW, _texH;

    osg::ref_ptr< osg::Object > _renderingCache;
};


// namespace backdropFX
}

// __BACKDROPFX_RENDERING_EFFECTS_H__
#endif
