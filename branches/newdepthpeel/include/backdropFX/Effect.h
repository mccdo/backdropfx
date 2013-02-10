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
class Effect : public osg::Object
{
public:
    Effect();
    Effect( const Effect& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,Effect)

    virtual void draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo );

    virtual void setTextureWidthHeight( unsigned int texW, unsigned int texH );

    void addInput( const unsigned int unit, osg::Texture* texture );
    osg::Texture* getInput( const unsigned int unit ) const;
    int getInputUnit( const osg::Texture* texture ) const;
    bool removeInput( const unsigned int unit );
    bool removeInput( osg::Texture* texture );

    void setOutput( osg::FrameBufferObject* fbo );
    osg::FrameBufferObject* getOutput() const;

    void setProgram( osg::Program* program );
    osg::Program* getProgram() const { return _program.get(); }

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

    typedef std::map< unsigned int, osg::ref_ptr< osg::Texture > > IntTextureMap;
    IntTextureMap _inputs;
    osg::ref_ptr< osg::FrameBufferObject > _output;

    unsigned int _width, _height;

    // TBD need to share these with all Effect classes (a static, perhaps?)
    osg::ref_ptr< osg::Geometry > _fstp;
    osg::ref_ptr< osg::Depth > _depth;
    osg::ref_ptr< osg::Uniform > _textureUniform;
};

typedef std::vector< osg::ref_ptr< Effect > > EffectVector;


// namespace backdropFX
}

// __BACKDROPFX_EFFECT_H__
#endif
