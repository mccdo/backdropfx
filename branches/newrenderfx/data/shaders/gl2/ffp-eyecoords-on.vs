
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-eyecoords-on.vs


void computeEyeCoords()
{
    bdfx_eyeVertex = bdfx_modelViewMatrix * bdfx_vertex;
    bdfx_eyeNormal = bdfx_normalMatrix * bdfx_normal;
}

// END gl2/ffp-eyecoords-on.vs

