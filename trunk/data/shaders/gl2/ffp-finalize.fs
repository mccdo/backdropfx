
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-finalize.fs

void finalize()
{
    gl_FragData[ 0 ] = bdfx_processedColor;
}

// END gl2/ffp-finalize.fs

