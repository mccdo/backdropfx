// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_EFFECT_LIBRARY_UTILS_H__
#define __BACKDROPFX_EFFECT_LIBRARY_UTILS_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/Effect.h>

#include <string>


namespace backdropFX {


/** \brief A convenience routine to toggle glow on or off.

Interacts directly with the Manager to toggle glow on or off.
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



// namespace backdropFX
}

// __BACKDROPFX_EFFECT_LIBRARY_UTILS_H__
#endif
