
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-finalize.fs

void finalize()
{
    gl_FragData[ 0 ] = bdfx_processedColor;

    // Debug
    //gl_FragData[ 0 ] = bdfx_processedColor + bdfx_partitionDebug;
}

// END gl2/bdfx-finalize.fs

