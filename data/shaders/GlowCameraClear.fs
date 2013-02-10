
// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/GlowCameraClear.fs


void main( void )
{
    // Clear the glow map. Set alpha to 1.0 to avoid alpha test discard.
    gl_FragData[ 0 ] = vec4( 0., 0., 0., 1. );

    // Clear the depth map. When DOF is enabled, 0.0 indicates fully sharp,
    // and 1.0 indicates fully blurred. The values written here effect DOF
    // on the sky dome (outside the app scene graph).
    // Note: DOF alpha blend only considers the red value.
    // TBD, we might want to put these values under app control. For very
    // large depthRange, sky dome might come into focus. For now, make it
    // fully blurred, so that the sky is blurred with DOF is enabled.
    gl_FragData[ 1 ] = vec4( 1., 1., 1., 1. );
}
