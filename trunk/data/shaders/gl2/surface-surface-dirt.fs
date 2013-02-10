
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

    // evaluate macro dirt-gravel distribution
    float fMacroDirtGravel = clampedfBmZeroToOne( 100.0 + ( TexCoordScaled * fDirtGravelDistScale ), 2.0 ); // 100: random offset

    // evaluate gravel properties
    float fGravelVal       = clampedfBmZeroToOne( TexCoordScaled * (fGravelScale * .25), 4.0 );
    vec4  fvGravelColor    = mix(vec4(.5, .4, .3, 1.0), vec4(.7, .6, .5, 1.0), fGravelVal);

    // evaluate dirt properties
    float fDirtVal         = clampedfBmZeroToOne(500.0 - (TexCoordScaled * fDirtScale), 4.0); // -500: random offset
    vec4  fvDirtColor      = mix(fvBaseColorA, fvBaseColorB, fDirtVal);

    bdfx_processedColor = mix( fvGravelColor, fvDirtColor, fMacroDirtGravel );


    // Compute the normal.

    vec3  fvNormalGravel   = fBMToNormal( TexCoordScaled * fGravelScale, 1.0, 0.02, localDFD );
    vec3  fvNormalDirt     = vec3(0.0, 0.0, 1.0);
    bdfx_surfaceNormal = mix( fvNormalGravel, fvNormalDirt, fMacroDirtGravel );
}

// END gl2/surface-surface-on.fs

