
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/glow0a.vs


void main( void )
{
    // Create tex coords in the range 0 to 1.
    oTC = (gl_Vertex.xy + 1.0) * 0.5;
    // Limit tex coords by the visible area of the texture.
    oTC *= texturePercent;

    gl_Position = gl_Vertex;
}
