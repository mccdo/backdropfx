
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/surface-declarations.fs
BDFX INCLUDE shaders/gl2/noise-declarations.common

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/surface-surface-on.fs


vec2 localDFD;

// Compute both the base color and the surface normal for per-pixel lighting.
void computeSurface()
{
    vec2 TexCoordScaled = bdfx_outTexCoord0.st * fScale;
    localDFD.x = 0.005; // dFdx(Texcoord);
    localDFD.y = 0.005; // dFdy(Texcoord);


    // Compute the color.

    float fGradVal         = clampedfBmZeroToOne( TexCoordScaled * 0.2, 4.0 );
    vec4  fvBaseColorMixed = mix( fvBaseColorA, fvBaseColorB, fGradVal );
    vec4  fvBaseShade      = texture2D( baseMap, TexCoordScaled );

    bdfx_processedColor = fvBaseColorMixed * fvBaseShade;


    // Compute the normal.

    vec3 fvNormalMap = MapToNormal( bumpMap, TexCoordScaled, .005, localDFD );
    vec3 fvNormalfBM = fBMToNormal( vec2( TexCoordScaled.x, TexCoordScaled.y * 9.0 ), 2.0, 0.001, localDFD );

    bdfx_surfaceNormal = ( fvNormalMap + fvNormalfBM ) * .5;
}

// END gl2/surface-surface-on.fs

