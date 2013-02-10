
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
    localDFD.x = 0.002; // dFdx(Texcoord);
    localDFD.y = 0.002; // dFdy(Texcoord);


    // Compute the color.

    // evaluate macro grass color distribution
    float fMacroGrassColor = clampedfBmZeroToOne( 100.0 + (TexCoordScaled * fMacroToMicroScale), 2.0 ); // 100: random offset
    vec4  fvGrassColor     = mix( fvBaseColorA, fvBaseColorB, fMacroGrassColor );
    float fGrassValue      = SampleAmplitude( permTexture, TexCoordScaled );
    fvGrassColor *= fGrassValue;

    bdfx_processedColor = fvGrassColor;


    // Compute the normal.

    vec3 fvNormal = MapToNormal( permTexture, -TexCoordScaled, 0.002, localDFD );
}

// END gl2/surface-surface-on.fs

