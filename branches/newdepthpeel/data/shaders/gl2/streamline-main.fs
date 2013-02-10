
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-main.fs

void main()
{
    init();

    depthPeel();

    // Per pixel lighting

    computeTexture();

    // color sum
    // fog
    computeFogFragment();

    // Debug
    //dfx_processedColor.rgb = bdfx_debugfs.rgb;
    //bdfx_processedColor.a = 1.0;

    // Alpha rejection (alpha test)
    // Too faint to see?
    if( bdfx_processedColor.a < 0.005 )
        discard;

    finalize();
}

// END gl2/bdfx-main.fs

