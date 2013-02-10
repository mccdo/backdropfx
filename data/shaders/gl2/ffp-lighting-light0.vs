
BDFX INCLUDE shaders/gl2/ffp-lighting-utilities.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-lighting-light0.vs


// Output:
//   varying vec4 bdfx_processedColor;
//   varying vec4 bdfx_outSecondaryColor;

void computeLighting()
{
    // TBD currently don't support 2-sided lighting.

    vec4 Acm, Dcm;
    if( bdfx_colorMaterial == 1 )
    {
        // Simplified color material support: either on or off for
        // both ambient and diffuse.
        Acm = bdfx_color;
        Dcm = bdfx_color;
    }
    else
    {
        Acm = bdfx_frontMaterial.ambient;
        Dcm = bdfx_frontMaterial.diffuse;
    }

    // Most of what follows is from OpenGL 2.1 spec, sec 2.14, pps 61-62

    // This shader supports only light 0 enabled.
    const int idx = 0;

    vec4 VPli;
    if( bdfx_lightSource[ idx ].absolute == 1 )
        VPli = bdfx_lightSource[ idx ].position;
    else
        VPli = ( osg_ViewMatrix * bdfx_lightSource[ idx ].position );

    vec4 lightResult = internalAmbientDiffuse( VPli,
        Acm, Dcm, bdfx_lightSource[ idx ].ambient, bdfx_lightSource[ idx ].diffuse );

    vec4 specResult = vec4( 0., 0., 0., 0. );
    specResult += internalSpecular( VPli.xyz, bdfx_lightSource[ idx ].specular );

    vec3 emissiveAmbient = bdfx_frontMaterial.emissive.rgb +
        ( Acm.rgb * bdfx_lightModel.ambient.rgb );
    bdfx_processedColor =
        vec4( emissiveAmbient, 0. ) +
        lightResult + specResult;
    // No support for separate specular in this shader.
    bdfx_outSecondaryColor = vec4( 0., 0., 0., 1. );

    // TBD Hm, seems like kind of a hack
    //bdfx_processedColor.a = .25;
}

// END gl2/ffp-lighting-light0.vs

