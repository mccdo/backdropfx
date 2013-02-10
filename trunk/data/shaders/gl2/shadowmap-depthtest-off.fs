#version 120

BDFX INCLUDE shaders/gl2/shadowmap-declarations.common
BDFX INCLUDE shaders/gl2/shadowmap-declarations.fs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/shadowmap-depthtest-off.fs


// Returns 1.0 if fully illuminated, 0.0 if fully shadowed,
// or in range 0.0,1.0 for partial shadowing.
float computeShadowDepthTest()
{
    return( 1.0 );
}

// END gl2/shadowmap-depthtest-off.fs

