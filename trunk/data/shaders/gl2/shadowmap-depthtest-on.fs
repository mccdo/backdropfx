//#version 120

BDFX INCLUDE shaders/gl2/shadowmap-declarations.common
BDFX INCLUDE shaders/gl2/shadowmap-declarations.fs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/shadowmap-depthtest-on.fs


// Returns 1.0 if fully illuminated, 0.0 if fully shadowed,
// or in range 0.0,1.0 for partial shadowing.
float computeShadowDepthTest()
{
    vec3 tc = bdfx_outShadowTexCoord.stp + bdfx_outShadowTexCoord.q;
    tc *= .5;
    if( (tc.s>0.) && (tc.t>0.) && (tc.s<bdfx_outShadowTexCoord.q) && (tc.t<bdfx_outShadowTexCoord.q) )
        return( shadow2DProj( bdfx_shadowDepthMap, vec4( tc, bdfx_outShadowTexCoord.q ) ).a );
    else
        return( 1.0 );
}

// END gl2/shadowmap-depthtest-on.fs

