// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_EFFECT_LIBRARY_H__
#define __BACKDROPFX_EFFECT_LIBRARY_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/Effect.h>
#include <osg/Program>

#include <string>


namespace backdropFX {


/** \defgroup EffectLibrary Library of Rendering Effects */
/*@{*/

/** \brief A glow rendering effect.

This effect has two texture inputs:
\li 0 is the normal color buffer
\li 1 is a glow map

This class overrides the base-class draw() to perform a separable filter blur
on the input glow map, then finally combines the blurred glow map with the
color buffer.

We collected the following screen shots using the
\ref debugflags "Manager debug flags" \c (debugImages).

\image html fig08-glow.jpg
This image shows the output of color buffer A. (The SkyDome, DepthPartition,
and DepthPeel classes have rendered.)

\image html fig09-glow.jpg
This image shows the glow map produced by the glow camera.

\image html fig10-glow.jpg
This image shows an intermediate step in the EffectGlow processing.
The glow map is an input texture. EffectGlow processes this texture
with a separable blur, first horizontal, then vertical. Showing is the
blurred glow map.

\image html fig11-glow.jpg
As a final step, EffectGlow adds the blurred glow map to
color buffer A.

*/

/** \brief A blur effect, separated into horizontal and vertical passes.
*/
class BACKDROPFX_EXPORT EffectSeperableBlur : public Effect
{
public:
    EffectSeperableBlur();
    EffectSeperableBlur( const EffectSeperableBlur& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectSeperableBlur);

    virtual void draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo, bool last=false );

    virtual void setTextureWidthHeight( unsigned int texW, unsigned int texH );

protected:
    ~EffectSeperableBlur();

    osg::ref_ptr< osg::Program > _horizontal;
    osg::ref_ptr< osg::Program > _vertical;

    osg::ref_ptr< osg::Texture2D > _hBlur;
    osg::ref_ptr< osg::FrameBufferObject > _hBlurFBO;
};


/** \brief Combine two input textures.
*/
class BACKDROPFX_EXPORT EffectCombine : public Effect
{
public:
    EffectCombine();
    EffectCombine( const EffectCombine& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectCombine);

protected:
    ~EffectCombine();
};


/** \brief Blend two textures using alpha from a third texture.
*/
class BACKDROPFX_EXPORT EffectAlphaBlend : public Effect
{
public:
    EffectAlphaBlend();
    EffectAlphaBlend( const EffectAlphaBlend& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectAlphaBlend);

protected:
    ~EffectAlphaBlend();
};


/** \brief Scale and bias an image.
*/
class BACKDROPFX_EXPORT EffectScaleBias : public Effect
{
public:
    EffectScaleBias();
    EffectScaleBias( const EffectScaleBias& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectScaleBias);

    void setScaleBias( const osg::Vec2f& scaleBias );
    osg::Vec2f getScaleBias() const { return( _scaleBias ); }

protected:
    ~EffectScaleBias();

    osg::Vec2f _scaleBias;
};



/*@}*/


// namespace backdropFX
}

// __BACKDROPFX_EFFECT_LIBRARY_H__
#endif
