
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/bdfx-depthpeel-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs

// Texture is currently inlined due to shader module's Geode/Drawable limitation.
// Inlining lets us enable/disable texture on a per Geode/Drawable basis.
// (cow.osg does this.)
BDFX INCLUDE shaders/gl2/ffp-texture.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-main.fs

void main()
{
    init();

    if( bdfx_depthPeelEnable == 1 )
    {
        // In addition to testing against the depth maps, this function
        // also sets the alpha value (bdfx_processedColod.a).
        depthPeel();
    }

    // Per pixel lighting

    if( bdfx_texture > 0 )
        computeTexture();

    // color sum

    computeFogFragment();

    // TBD Remove this and just use alpha test.
    if( bdfx_processedColor.a < 0.005 )
        discard;

    finalize();
}

// END gl2/bdfx-main.fs

