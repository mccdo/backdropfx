
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs

// Texture is currently inlined due to shader module's Geode/Drawable limitation.
// Inlining lets us enable/disable texture on a per Geode/Drawable basis.
// (cow.osg does this.)
BDFX INCLUDE shaders/gl2/ffp-texture.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-main-nodepthpeel.fs

void main()
{
#if 0
    gl_FragData[ 0 ] = bdfx_outColor;
#else
    init();

    // Don't call depthPeel; should be a no-op anyway
    // because bdfx-depthpeel-off.fs should be active.
    //depthPeel();

    // Per pixel lighting

    if( bdfx_texture > 0 )
        computeTexture();

    // color sum
    // fog
    computeFogFragment();

    // Debug
    //bdfx_processedColor.rgb = bdfx_debugfs.rgb;
    //bdfx_processedColor.a = 1.0;

    // Alpha rejection (alpha test)
    // Too faint to see?
    if( bdfx_processedColor.a < 0.005 )
        discard;

    finalize();
#endif
}

// END gl2/bdfx-main-nodepthpeel.fs

