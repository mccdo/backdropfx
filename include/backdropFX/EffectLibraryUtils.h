// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_EFFECT_LIBRARY_UTILS_H__
#define __BACKDROPFX_EFFECT_LIBRARY_UTILS_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/Effect.h>

#include <string>


namespace backdropFX {


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



/** \brief A convenience routine to toggle glow on and off.

Interacts directly with the Manager to toggle glow on and off.
When on, it creates an EffectGlow instance and adds it to
the RenderingEffects EffectVector and sets the Manager glow camera
to have a node mask of 0xffffffff. When off, it deletes the
EffectGlow instance from the EffectVector and sets the glow camera
node mask to 0.

\return True on success. But if there is no work to do (requires no
state change to fulfill the request), this function
returns false.
*/
BACKDROPFX_EXPORT bool configureGlowEffect( const bool enable );

/** \brief TBD
TBD
*/
BACKDROPFX_EXPORT bool configureDOFEffect( const bool enable );


// namespace backdropFX
}

// __BACKDROPFX_EFFECT_LIBRARY_UTILS_H__
#endif
