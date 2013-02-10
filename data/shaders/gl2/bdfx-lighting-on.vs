
BDFX INCLUDE shaders/gl2/bdfx-lighting-utilities.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-lighting-on.vs


// Output:
//   varying vec4 bdfx_outColor;
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

    vec4 outColor =
        bdfx_frontMaterial.emissive +
        ( Acm * bdfx_lightModel.ambient );
    outColor.a = 0.;
    vec4 outSpec = vec4( 0., 0., 0., 0. );

    // Calculate lighting from the Sun
    vec4 diffResult, specResult;
    vec4 VPli = ( osg_ViewMatrix * vec4( bdfx_sunPosition, 0. ) );
    internalSunLighting( diffResult, specResult, VPli, Dcm );
    outColor += diffResult;
    outSpec += specResult;

    int idx;
    for( idx=0; idx<bdfx_maxLights; idx++ )
    {
        // unrolled to avoid subscripting an array of bdfx_maxLights on the Mac
        if( idx==0 && bdfx_lightEnable0 == 0 ) continue;
        if( idx==1 && bdfx_lightEnable1 == 0 ) continue;
        if( idx==2 && bdfx_lightEnable2 == 0 ) continue;
        if( idx==3 && bdfx_lightEnable3 == 0 ) continue;
        if( idx==4 && bdfx_lightEnable4 == 0 ) continue;
        if( idx==5 && bdfx_lightEnable5 == 0 ) continue;
        if( idx==6 && bdfx_lightEnable6 == 0 ) continue;
        if( idx==7 && bdfx_lightEnable7 == 0 ) continue;

        vec4 VPli;
        if( bdfx_lightSource[ idx ].absolute == 1.0 )
            VPli = bdfx_lightSource[ idx ].position;
        else
            VPli = ( osg_ViewMatrix * bdfx_lightSource[ idx ].position );

        outColor += internalAmbientDiffuse( VPli,
            Acm, Dcm, bdfx_lightSource[ idx ].ambient, bdfx_lightSource[ idx ].diffuse );

        outSpec += internalSpecular( VPli.xyz, bdfx_lightSource[ idx ].specular );
    }

    bdfx_processedColor = outColor;
    if( bdfx_lightModel.separateSpecular == 0 )
    {
        bdfx_processedColor += outSpec;
        bdfx_outSecondaryColor = vec4( 0., 0., 0., 1. );
    }
    else
        bdfx_outSecondaryColor = outSpec;

    // TBD Hm, seems like kind of a hack
    //bdfx_processedColor.a = 1.0;
}

// END gl2/bdfx-lighting-on.vs

