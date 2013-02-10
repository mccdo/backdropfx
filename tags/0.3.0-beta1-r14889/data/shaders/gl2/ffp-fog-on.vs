
BDFX INCLUDE shaders/gl2/ffp-declarations.vs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-fog-on.vs

// Input:
//   uniform float bdfx_fogCoord; // aka gl_FogCoord

// Output:
//   varying float bdfx_outFogFragCoord;

void computeFogVertex()
{
    //bdfx_outFogFragCoord = bdfx_fogCoord // <<<>>> not used yet
    bdfx_outFogFragCoord = abs(bdfx_eyeVertex.z);

}

// END gl2/ffp-fog-oon.vs



