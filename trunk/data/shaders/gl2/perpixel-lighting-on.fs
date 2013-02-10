
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/perpixel-declarations.fs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/perpixel-lighting-on.fs


//
// Inputs
// TBD needs to use bdfx lighting interface.
uniform vec4 fvAmbient;
uniform vec4 fvDiffuse;

// TBD needs to go in .common file for access by frag shader.
varying vec3 LightDirection;

void computePerPixelLighting()
{
    vec3 fvNormal = computePerPixelNormal();

    vec3  fvLightDirection = normalize( LightDirection );

    float fNDotL           = dot( fvNormal, fvLightDirection );

    vec4  fvTotalAmbient   = fvAmbient * bdfx_processedColor;
    vec4  fvTotalDiffuse   = fvDiffuse * fNDotL * bdfx_processedColor;

    bdfx_processedColor = fvTotalAmbient + fvTotalDiffuse;
}

// END gl2/perpixel-lighting-on.fs

