
BDFX INCLUDE shaders/gl2/bdfx-lighting-utilities.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-lighting-sunonly.vs


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

    vec4 diffResult, specResult;
    vec4 VPli = ( osg_ViewMatrix * vec4( bdfx_sunPosition, 0. ) );
    internalSunLighting( diffResult, specResult, VPli, Dcm );

    vec3 emissiveAmbient = bdfx_frontMaterial.emissive.rgb +
        ( Acm.rgb * bdfx_lightModel.ambient.rgb );
    bdfx_processedColor =
        vec4( emissiveAmbient, 0. ) +
        diffResult + specResult;
    // No support for separate specular in this shader.
    bdfx_outSecondaryColor = vec4( 0., 0., 0., 1. );

    // TBD Hm, seems like kind of a hack
    //bdfx_processedColor.a = 1.0;
}

// END gl2/bdfx-lighting-sunonly.vs

