
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-init.vs


void init()
{
    bdfx_processedColor = bdfx_color;

    // Projection is done with depth partitioning.
    bdfx_projection = bdfx_partitionMatrix;
}

// END gl2/ffp-init.vs

