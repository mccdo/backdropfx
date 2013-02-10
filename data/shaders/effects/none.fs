
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/none.fs


void main( void )
{
    gl_FragColor = texture2D( inputTextures[0], oTC );
    //gl_FragColor = vec4( 1, 0, 0, 1 );
}
