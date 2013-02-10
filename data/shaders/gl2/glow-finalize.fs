
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs

BDFX INCLUDE shaders/gl2/glow-declarations.common


// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/glow-finalize.fs

void finalize()
{
    // This is the glow color, going into the backdropFX
    // color buffer glow.
    gl_FragData[ 0 ] = bdfx_processedColor;

    // This is the normalized distance from the focal point,
    // 0. for in focus and 1.0 for fully blurred, going into the
    // backdropFX depth buffer
    gl_FragData[ 1 ] = vec4( vec3( bdfx_normalizedDistanceToFocus ), 1.0 );
}

// END gl2/glow-finalize.fs

