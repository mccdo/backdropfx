
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/glow1.fs


void main( void )
{
    // Main color image:
    vec4 color0 = texture2D( inputTextures[0], oTC );
    // Blurred glow map:
    vec4 color1 = texture2D( inputTextures[1], oTC );

    gl_FragColor = color0 + color1;
}
