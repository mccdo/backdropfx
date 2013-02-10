
// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/shadowmap-declarations.fs


//
// Input uniforms
uniform sampler2DShadow bdfx_shadowDepthMap;


//
// Function declarations

// Returns 1.0 if fully illuminated, 0.0 if fully shadowed,
// or in range 0.0,1.0 for partial shadowing.
float computeShadowDepthTest();


// END gl2/shadowmap-declarations.fs

