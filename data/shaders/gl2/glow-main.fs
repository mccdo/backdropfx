
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/glow-main.fs

void main()
{
    init();

    // No depth peeling in glow/DOF pass.

    // Per pixel lighting - Probably not for glow.

    // Hm. We could potentially apply a glow map here.
    if( bdfx_texture2dEnable0 > 0 ) // should test all units, but this is faster
        computeTexture();

    // color sum - Probably not for glow.

    // fog - Maybe we have to fog the glow color, not sure.
    computeFogFragment();

    // Render the glow color.
    bdfx_processedColor.rgb = bdfx_glowColor.rgb;

    // Compare against 0.99 rather than 1.0 to avoid roundoff error.
    if( bdfx_processedColor.a < 0.99 )
    {
        // To allow glowing objects to glow through transparent
        // objects, discard transparent objects with 0 glow color.
        // (Another way to do this would be with NodeMasks during the
        // cull traversal: cull would skip objects with non-1.0
        // transparency and 0 glow.)
        if( bdfx_glowColor.rgb == vec3( 0., 0., 0. ) )
            bdfx_processedColor.a = 0.0; // discard using alpha test
    }

    finalize();
}

// END gl2/bdfx-main.fs

