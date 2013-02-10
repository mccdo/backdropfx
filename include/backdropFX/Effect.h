// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_EFFECT_H__
#define __BACKDROPFX_EFFECT_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/RenderingEffectsStage.h>
#include <osg/Referenced>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osg/RenderInfo>

#include <vector>
#include <map>


namespace backdropFX {


/** \brief Used in conjunction with the RenderingEffects class as one element of a post-rendering effects pipeline.

The application stores multiple Effect instances in the RenderingEffects
EffectVector. The RenderingEffectsStage processes each Effect in order during the draw traversal.

In its simplest usage (as in the non-specialized base class Effect), an Effect
simply draws a fullscreen triangle pair using the specified Program and input
texture. Applications can customize the base class Effect with their own
Program and one or more input textures.

Apps can also specify the output as an FBO. If set to NULL, the base class
Effect uses the RenderingEffects FBO. If both the Effect output and
RenderingEffects FBO are NULL, then Effect draws into the default framebuffer
(ID 0).

Some rendering effects require multiple passes to produce the final result. To
perform multiple passes, apps can add multiple Effects to RenderingEffect,
with each processing sequentially. However, a better solution is to
create a specialization of Effect that overrides Effect::draw(). The custom
draw function can call Effect::internalDraw() iteratively, varying the
texture, FBO, and program bindings for each pass.
*/
class BACKDROPFX_EXPORT Effect : public osg::Object
{
    friend class CompositeEffect;

public:
    Effect();
    Effect( const Effect& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,Effect)

    virtual void draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo, bool last=false );

    virtual void setTextureWidthHeight( unsigned int texW, unsigned int texH );

    virtual void addInput( const unsigned int unit, osg::Texture* texture );
    virtual osg::Texture* getInput( const unsigned int unit ) const;
    virtual int getInputUnit( const osg::Texture* texture ) const;
    virtual bool removeInput( const unsigned int unit );
    virtual bool removeInput( osg::Texture* texture );

    virtual void setOutput( osg::FrameBufferObject* fbo );
    virtual osg::FrameBufferObject* getOutput() const;

    /** \brief Attach the color buffer output to the input of an effect.
    */
    virtual bool attachOutputTo( Effect* effect, unsigned int unit );

    void setProgram( osg::Program* program );
    osg::Program* getProgram() const { return _program.get(); }


    typedef std::vector< osg::ref_ptr< osg::Uniform > > UniformVector;

    /** Per-Effect osg::Uniforms for use by the effects Camera.
    The Manager or RenderingEffects object calls this function to obtain a list
    of uniforms, and adds them to the effects Camera StateSet. This allows each Effect
    to control how the effects Camera creates the texture that are used as inputs to the
    Effects. For an example, see EffectDOF::setFocalDistance.
    */
    UniformVector& getEffectsPassUniforms() { return( _effectsPassUniforms ); }

protected:
    ~Effect();

    /** Disables depth testing and draws a fullscreen triangle pair.
    */
    virtual void internalDraw( osg::RenderInfo& renderInfo );

    /** Reads a block of pixels using glReadPixels, and writes it to
    a file using the backdropFX debugDumpImage() utility.

    The output file name has the form:
    \code
    <base>effect-<obj>.png
    \endcode
    where:
    \li <base> comes from RenderingEffects::debugImageBaseFileName()
    \li <obj> is the Effect's OSG Object name, from Object::getName()
    */
    virtual void dumpImage( const osg::Viewport* vp, const std::string baseFileName );


    osg::ref_ptr< osg::Program > _program;
    UniformVector _textureUniform;
    UniformVector _effectsPassUniforms;
    UniformVector _uniforms;

    typedef std::map< unsigned int, osg::ref_ptr< osg::Texture > > IntTextureMap;
    IntTextureMap _inputs;

    osg::ref_ptr< osg::FrameBufferObject > _output;

    unsigned int _width, _height;

    // TBD need to share these with all Effect classes (a static, perhaps?)
    osg::ref_ptr< osg::Geometry > _fstp;
    osg::ref_ptr< osg::Depth > _depth;
};



/** \defgroup EffectVector EffectVector operations. */
/*@{*/

/** \brief STL vector of Effect reference pointers.
Used by the RenderingEffects and CompositeEffects classes.
*/
typedef std::vector< osg::ref_ptr< Effect > > EffectVector;

/** Looks in the EffectVector for an Effect with the specified class name.
\param className Class name to search for, e.g., "EffectBasicGlow".
\return If found, it's the address of the class name. Otherwise NULL.
*/
BACKDROPFX_EXPORT Effect* getEffect( const std::string& className, EffectVector& effectVector );

/** Removes the Effect with the specified object name, if found in the EffectVector.
Deletes a single instance only (it does not look for multiple instances). To delete all
instances of the same Effect, call removeEffect in a loop, checking the return value.
\param objectName Object name to search for, e.g., "Glow-Blur".
\return True if found and deleted, otherwise false.
*/
BACKDROPFX_EXPORT bool removeEffect( const std::string& objectName, EffectVector& effectVector );


/*@}*/



class BACKDROPFX_EXPORT CompositeEffect : public backdropFX::Effect
{
public:
    CompositeEffect();
    CompositeEffect( const CompositeEffect& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,CompositeEffect);

    virtual void draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo, bool last=false );

    virtual void setTextureWidthHeight( unsigned int texW, unsigned int texH );

    virtual void addInput( const unsigned int unit, osg::Texture* texture );
    virtual osg::Texture* getInput( const unsigned int unit ) const;
    virtual int getInputUnit( const osg::Texture* texture ) const;
    virtual bool removeInput( const unsigned int unit );
    virtual bool removeInput( osg::Texture* texture );

    virtual void setOutput( osg::FrameBufferObject* fbo );
    virtual osg::FrameBufferObject* getOutput() const;

    /** \brief Attach the color buffer output to the input of an effect.
    */
    virtual bool attachOutputTo( Effect* effect, unsigned int unit );

protected:
    ~CompositeEffect();

    /** Reads a block of pixels using glReadPixels, and writes it to
    a file using the backdropFX debugDumpImage() utility.

    The output file name has the form:
    \code
    <base>effect-<obj>.png
    \endcode
    where:
    \li <base> comes from RenderingEffects::debugImageBaseFileName()
    \li <obj> is the Effect's OSG Object name, from Object::getName()
    */
    virtual void dumpImage( const osg::Viewport* vp, const std::string baseFileName );

    EffectVector _subEffects;
};



// namespace backdropFX
}

// __BACKDROPFX_EFFECT_H__
#endif
