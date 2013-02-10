
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/glow1.fs


void main( void )
{
    // Main color image:
    vec4 color0 = texture2D( inputTexture0, oTC );
    // Blurred glow map:
    vec4 color1 = texture2D( inputTexture1, oTC );
    //color1 += vec4( .5, 0., 0., 0. );

    // TBD, in future, possibly support other operations besides ADD,
    // controled by a uniform.
    gl_FragColor = color0 + color1;
    //gl_FragColor = vec4( color1.rgb, 1.0 );
}
