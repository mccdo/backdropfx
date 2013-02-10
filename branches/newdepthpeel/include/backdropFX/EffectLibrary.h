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

/** \brief A utility function to create an OSG Program for use with Effects.

This convenience function requires that Effect developers use a specific naming
scheme when creating their shaders. You should store Shader source files in
shaders/effects and name them <baseName>.vs and <baseName>.fs.

\param baseName Used to find the following two files:
\li shaders/effects/<baseName>.vs
\li shaders/effects/<baseName>.fs

After the createEffectProgram function loads the files, the backdropFX::shaderPreProcess 
processes their source and links them into an OSG Program, which returns to the calling code. 
*/
BACKDROPFX_EXPORT osg::Program* createEffectProgram( const std::string& baseName );



/** \brief A glow rendering effect.

This effect has two texture inputs:
\li 0 is the normal color buffer
\li 1 is a glow map

This class overrides the base-class draw() to perform a separable filter blur
on the input glow map, then finally combines the blurred glow map with the
color buffer.

The following screen shots were collected using the
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
class EffectGlow : public Effect
{
public:
    EffectGlow();
    EffectGlow( const EffectGlow& rhs, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    META_Object(backdropFX,EffectGlow)

    virtual void draw( RenderingEffectsStage* rfxs, osg::RenderInfo& renderInfo );

    virtual void setTextureWidthHeight( unsigned int texW, unsigned int texH );

protected:
    ~EffectGlow();

    osg::ref_ptr< osg::Program > _horizontal;
    osg::ref_ptr< osg::Program > _vertical;
    osg::ref_ptr< osg::Program > _combine;

    osg::ref_ptr< osg::Texture2D > _hBlur;
    osg::ref_ptr< osg::Texture2D > _vBlur;
    osg::ref_ptr< osg::FrameBufferObject > _hBlurFBO;
    osg::ref_ptr< osg::FrameBufferObject > _vBlurFBO;
};

/*@}*/


// namespace backdropFX
}

// __BACKDROPFX_EFFECT_LIBRARY_H__
#endif
