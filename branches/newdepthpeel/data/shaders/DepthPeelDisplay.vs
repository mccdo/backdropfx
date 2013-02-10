// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

varying vec2 oTC;

void main( void )
{
    oTC = (gl_Vertex.xy + 1.0) * 0.5;
    gl_Position = gl_Vertex;
}
