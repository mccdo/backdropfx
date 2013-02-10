
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/perpixel-declarations.fs
BDFX INCLUDE shaders/gl2/noise-declarations.common
BDFX INCLUDE shaders/gl2/surface-declarations.fs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/perpixel-normal-dirt.fs


vec3 computePerPixelNormal()
{
    return( bdfx_surfaceNormal );
}

// END gl2/perpixel-normal-dirt.fs

