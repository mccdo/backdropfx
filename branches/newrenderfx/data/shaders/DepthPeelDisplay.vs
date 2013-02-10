// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

uniform vec2 depthPeelTexturePercent;
varying vec2 oTC;

void main( void )
{
    // Create tex coords in the range 0 to 1.
    oTC = (gl_Vertex.xy + 1.0) * 0.5;
    // Limit tex coords by the visible area of the texture.
    oTC *= depthPeelTexturePercent;
    
    gl_Position = gl_Vertex;
}
