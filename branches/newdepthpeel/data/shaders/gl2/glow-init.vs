
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/glow-init.vs


void init()
{
    // Normal init
    bdfx_processedColor = bdfx_color;

    // For glow/DOF pass, use the projection matrix.
    // (We are not doing depth partitioning.)
    bdfx_projection = bdfx_projectionMatrix;
}

// END gl2/ffp-init.vs

