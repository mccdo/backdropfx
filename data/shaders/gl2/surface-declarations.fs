 
// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/surface-declarations.fs


//
// Input uniforms
uniform float fScale;
uniform float fDirtGravelDistScale;
uniform float fGravelScale;
uniform float fDirtScale;
uniform vec4 fvBaseColorA;
uniform vec4 fvBaseColorB;

uniform float fMacroToMicroScale;

uniform sampler2D baseMap;
uniform sampler2D bumpMap;


//
// Function declarations
void computeSurface();


//
// Local variables
// bdfx_surfaceNormal written by computeSurface(), read by computePerPixelNormal().
vec3 bdfx_surfaceNormal;


// END gl2/surface-declarations.fs

