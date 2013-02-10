
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-transform.vs


void computeTransform()
{
    gl_Position = bdfx_projection * bdfx_modelViewMatrix * bdfx_vertex;
}

// END gl2/bdfx-transform.vs

