// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SHADER_LIBRARY_CONSTANTS_H__
#define __BACKDROPFX_SHADER_LIBRARY_CONSTANTS_H__ 1


namespace backdropFX {


/** \defgroup ShaderConstants Shader Library Constants
*/
/*@{*/


/** For applying textures using FFP texture mapping, behave as if
there are no more than BDFX_MAX_TEXTURE_UNITS hardware texture units.
This value sizes the array holding texture environment color values.
This value doesn't prevent a shader from sampling more/greater units.

<b>Shader uniform:</b> \c bdfx_maxTextureUnits */
#define BDFX_MAX_TEXTURE_UNITS 4

/** Used in texture coordinate generation to size the arrays for
tex gen enable, eye planes, and object planes.

<b>Shader uniform:</b> \c bdfx_maxTextureCoords */
#define BDFX_MAX_TEXTURE_COORDS 8

/** Used to size the arrays for clip planes and clip plane
enables.

<b>Shader uniform:</b> \c bdfx_maxClipPlanes */
#define BDFX_MAX_CLIP_PLANES 8

/** Maximum number of supported forward lighting light sources.
Sizes the arrays for light enables and light source parameters.
In the general lighting shader, this value controls the number of
iterations through the lighting loop.

<b>Shader uniform:</b> \c bdfx_maxLights */
#define BDFX_MAX_LIGHTS 8

/** When rendering streamlines, the shader library code samples this texture 
unit to obtain texels for rendering each streamline point sprite.

<b>Shader uniform:</b> \c bdfx_streamlineImageUnit */
#define BDFX_STREAMLINE_IMAGE_UNIT 7



/*@}*/

// backdropFX
}


// __BACKDROPFX_SHADER_LIBRARY_CONSTANTS_H__
#endif
