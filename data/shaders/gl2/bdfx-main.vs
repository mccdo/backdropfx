
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.vs
BDFX INCLUDE shaders/gl2/bdfx-depthpeel-declarations.common

BDFX INCLUDE shaders/gl2/ffp-texgen.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-main.vs


void main()
{
    init();

    computeEyeCoords();
    if( bdfx_normalize == 1 )
        bdfx_eyeNormal = normalize( bdfx_eyeNormal );

    computeLighting();

    computeTexGen();

    // model space clipping

    computeTransform();
    // For depth peeling. Use clip coord as depth map coord.
    bdfx_depthTC = gl_Position;
    bdfx_depthTCBias = vec2( 0., 0. );

    computeFogVertex();
    
    finalize();
}

// END gl2/bdfx-main.vs

