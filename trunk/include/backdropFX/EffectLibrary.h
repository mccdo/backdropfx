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


/** \brief A blur effect, separated into horizontal and vertical passes.
*/
class BACKDROPFX_EXPORT EffectSeperableBlur : public Effect
{
    // TBD This should be a CompositeEffect.

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


/** \brief A Gaussian blur effect.
*/
class BACKDROPFX_EXPORT GaussConvolution : public Effect
{
public:
    GaussConvolution();

    GaussConvolution(
        GaussConvolution const& rhs,
        osg::CopyOp const& copyop = osg::CopyOp::SHALLOW_COPY );

    META_Object( backdropFX, GaussConvolution );

    virtual void draw(
        RenderingEffectsStage* rfxs,
        osg::RenderInfo& renderInfo,
        bool last = false );

    virtual void setTextureWidthHeight(
        unsigned int texW, unsigned int texH );

protected:
    ~GaussConvolution();

private:
    osg::ref_ptr< osg::Uniform > m_viewportSize;

    osg::ref_ptr< osg::Viewport > m_viewport;

    osg::ref_ptr< osg::Program > m_1Dx;

    osg::ref_ptr< osg::Program > m_1Dy;

    osg::ref_ptr< osg::Texture2D > m_tex2D;

    osg::ref_ptr< osg::FrameBufferObject > m_fbo;
};


/** \brief Resample an input texture.
*/
class BACKDROPFX_EXPORT Resample : public Effect
{
public:
    Resample();

    Resample(
        Resample const& rhs,
        osg::CopyOp const& copyop = osg::CopyOp::SHALLOW_COPY );

    META_Object( backdropFX, Resample );

    virtual void draw(
        RenderingEffectsStage* rfxs,
        osg::RenderInfo& renderInfo,
        bool last = false );

    virtual void setTextureWidthHeight(
        unsigned int texW, unsigned int texH );

    void setFactor( float xFactor, float yFactor );

protected:
    ~Resample();

private:
    osg::Vec2 m_factor;

    osg::ref_ptr< osg::Viewport > m_viewport;

    osg::ref_ptr< osg::Texture2D > m_tex2D;

};

/** \brief Combine scene output with glow output.
*/
class BACKDROPFX_EXPORT GlowCombine : public Effect
{
public:
    GlowCombine();

    GlowCombine(
        GlowCombine const& rhs,
        osg::CopyOp const& copyop = osg::CopyOp::SHALLOW_COPY );

    META_Object( backdropFX, GlowCombine );

protected:
    ~GlowCombine();

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
This image shows the glow map produced by the effects camera.

\image html fig10-glow.jpg
This image shows an intermediate step in the EffectBasicGlow processing.
The glow map is an input texture. EffectBasicGlow processes this texture
with a separable blur, first horizontal, then vertical. Showing is the
blurred glow map.

\image html fig11-glow.jpg
As a final step, EffectBasicGlow adds the blurred glow map to
color buffer A.

Input 0: ColorBufferA
Input 1: GlowColorBuffer
*/
class BACKDROPFX_EXPORT EffectBasicGlow : public CompositeEffect
{
public:
    EffectBasicGlow();
    EffectBasicGlow( const EffectBasicGlow& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectBasicGlow);

    virtual void addInput( const unsigned int unit, osg::Texture* texture );
    /* Not a Doxygen comment
    TBD should probably implement these at some point.
    virtual osg::Texture* getInput( const unsigned int unit ) const;
    virtual int getInputUnit( const osg::Texture* texture ) const;
    virtual bool removeInput( const unsigned int unit );
    virtual bool removeInput( osg::Texture* texture );
    */

    /** Set the glow color. Default is rgba 0,0,0,0 (nothing glowing). */
    void setDefaultGlowColor( const osg::Vec4f& color );
    const osg::Vec4f& getDefaultGlowColor() const { return( _glowColor ); }

    /** By default, ignore glow color alpha and take alpha from shader processed color.
    This allows transparent glowing objects to have a transparent glow. Set to true to
    use the glow color alpha instead. */
    void setUseGlowAlpha( bool useGlowAlpha );
    bool getUseGlowAlpha() const { return( _useGlowAlpha ); }

protected:
    ~EffectBasicGlow();

    osg::Vec4f _glowColor;
    bool _useGlowAlpha;
    osg::ref_ptr< osg::Uniform > _glowColorUniform, _useGlowAlphaUniform;
};

/** \brief Improved glow with Gaussian blur and downsampling.

Resample
  Input 0: ColorBufferGlow
GaussConvolution
  Input 0: Resample output
GlowCombine
  Input 0: ColorBufferA
  Input 1: GaussConvolution output
  Input 2: ColorBufferGlow
*/
class BACKDROPFX_EXPORT EffectImprovedGlow : public CompositeEffect
{
public:
    EffectImprovedGlow();
    EffectImprovedGlow( const EffectImprovedGlow& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectImprovedGlow);

    virtual void setTextureWidthHeight( unsigned int texW, unsigned int texH );

    virtual void addInput( const unsigned int unit, osg::Texture* texture );
    /* Not a Doxygen comment
    TBD should probably implement these at some point.
    virtual osg::Texture* getInput( const unsigned int unit ) const;
    virtual int getInputUnit( const osg::Texture* texture ) const;
    virtual bool removeInput( const unsigned int unit );
    virtual bool removeInput( osg::Texture* texture );
    */

    /** Set the glow color. Default is rgba 0,0,0,0 (nothing glowing). */
    void setDefaultGlowColor( const osg::Vec4f& color );
    const osg::Vec4f& getDefaultGlowColor() const { return( _glowColor ); }

    /** By default, ignore glow color alpha and take alpha from shader processed color.
    This allows transparent glowing objects to have a transparent glow. Set to true to
    use the glow color alpha instead. */
    void setUseGlowAlpha( bool useGlowAlpha );
    bool getUseGlowAlpha() const { return( _useGlowAlpha ); }

protected:
    ~EffectImprovedGlow();

    osg::Vec2 _resampleFactor;

    osg::Vec4f _glowColor;
    bool _useGlowAlpha;
    osg::ref_ptr< osg::Uniform > _glowColorUniform, _useGlowAlphaUniform;
};


/** \brief Depth of field composite Effect.
TBD Currently, this effect has some rendering artifacts due to "leaking"
of blurred data into depth regions that should appear in sharp focus.
This issue could be addressed by replacing the simple blur with a shader
that adjusts the blur sample kernel size based on the focal distance
input texture.
*/
class BACKDROPFX_EXPORT EffectDOF : public CompositeEffect
{
public:
    EffectDOF();
    EffectDOF( const EffectDOF& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectDOF);

    virtual void addInput( const unsigned int unit, osg::Texture* texture );
    /* Not a Doxygen comment
    TBD should probably implement these at some point.
    virtual osg::Texture* getInput( const unsigned int unit ) const;
    virtual int getInputUnit( const osg::Texture* texture ) const;
    virtual bool removeInput( const unsigned int unit );
    virtual bool removeInput( osg::Texture* texture );
    */

    /** Set the focal distance in world.
    Rendered geometry at this distance will appear in sharp focus.
    Default focal distance is 2. */
    void setFocalDistance( float distance );
    float getFocalDistance() const { return( _distance ); }

    /** Set the focal range in world coordinates.
    Rendered geometry that is greater than or equal to \c distance
    world coordinate units away from the focal distance will appear
    fully blurred. Default focal range is 5d00.
    */
    void setFocalRange( float range );
    float getFocalRange() const { return( _range ); }

protected:
    ~EffectDOF();

    float _distance, _range;
    osg::ref_ptr< osg::Uniform > _focalDistance, _focalRange;
};


/** \brief A heat distortion effect.
*/
class BACKDROPFX_EXPORT EffectHeatDistortion : public Effect
{
public:
    EffectHeatDistortion();
    EffectHeatDistortion( const EffectHeatDistortion& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectHeatDistortion);

    /** Set the heat distortion range in world coordinates.
    Rendered geometry that is less that equal to \c range.x()
    distant will have no heat distortion, and greater than or equal to
    \c range.y() will have full heat distortion.
    Default range is (0,1000).
    */
    void setHeatRange( const osg::Vec2f& range );
    osg::Vec2f getHeatRange() const { return( _range ); }

    /** TBD. To be done: Possible intensity control. Not yet implemented.
    Specifies the intensity of the distortion at the far end of the heat range.
    \param intensity is a normalized value in the range 0.0 to 1.0.
    \c intensity of 0.0 effectively distabes heat distortion.
    */
    // void setHeatIntensity( const float intensity );
    // float getHeatIntensity() const { return( _intensity ); }

protected:
    ~EffectHeatDistortion();

    osg::Vec2f _range;
    osg::ref_ptr< osg::Uniform > _heatRange;
};



/*@}*/


// namespace backdropFX
}

// __BACKDROPFX_EFFECT_LIBRARY_H__
#endif
