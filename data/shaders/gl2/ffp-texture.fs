
// shaders/gl2/ffp-declarations.common
// shaders/gl2/ffp-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-texture.fs


uniform sampler2D texture0;

void computeTexture()
{
    vec2 tc;
    if( bdfx_pointSprite == 1 )
        tc = gl_PointCoord.st;
    else
        tc = bdfx_outTexCoord[ 0 ].st;
    vec4 texColor = texture2D( texture0, tc );

    // TBD more TexEnv
    // Modulate
    bdfx_processedColor.rgb = texColor.rgb * bdfx_processedColor.rgb;

    // Combine texture alpha with streamline alpha.
    bdfx_processedColor.a *= texColor.a;
}

// END gl2/ffp-texture.fs

