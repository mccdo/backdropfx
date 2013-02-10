
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/point-finalize.vs


void finalize()
{
    bdfx_outColor = bdfx_processedColor;

    // TBD need to handle this better. Just want to make something
    // show up for now.
    gl_PointSize = 40.0;
}

// END gl2/point-finalize.vs

