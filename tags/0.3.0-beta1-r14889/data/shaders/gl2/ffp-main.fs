#version 120

BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs

// Texture is currently inlined due to shader module's Geode/Drawable limitation.
// Inlining lets us enable/disable texture on a per Geode/Drawable basis.
// (cow.osg does this.)
BDFX INCLUDE shaders/gl2/ffp-texture.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-main.fs

void main()
{
    init();

    // Per pixel lighting

    if( bdfx_texture2dEnable0 > 0 ) // should test all units, but this is faster
        computeTexture();

    // color sum
    // fog
    computeFogFragment();


    finalize();
}

// END gl2/ffp-main.fs

