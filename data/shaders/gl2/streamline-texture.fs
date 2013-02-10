
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/streamline-texture.fs


uniform sampler2D textureSplotch;

void computeTexture()
{
    vec2 tc = bdfx_outTexCoord1.st;

    vec4 texColor = texture2D( textureSplotch, tc );

    // Modulate
    bdfx_processedColor.rgb = texColor.rgb * bdfx_processedColor.rgb;

    // Combine texture alpha with streamline alpha.
    bdfx_processedColor.a *= texColor.a;
}

// END gl2/ffp-texture.fs

