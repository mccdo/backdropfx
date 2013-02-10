
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs
BDFX INCLUDE shaders/gl2/bdfx-depthpeel-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-main.fs

void main()
{
    init();

    // Per pixel lighting

    computeTexture();

    // color sum

    // Do not allow fog to affect streamline alpha.
    float saveAlpha = bdfx_processedColor.a;
    computeFogFragment();
    bdfx_processedColor.a = saveAlpha;

    // This code should be _after_ the above fragment processing.
    // Depth peeling changes bdfx_processedColor.a, and we want this
    // done after things like texture mapping and fog.
    depthPeel();

    // Debug
    //bdfx_processedColor.rgb = bdfx_debugfs.rgb;
    //bdfx_processedColor.a = 1.0;

    finalize();
}

// END gl2/bdfx-main.fs

