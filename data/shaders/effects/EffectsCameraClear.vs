
// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// GlowCameraClear.vs


void main( void )
{
    // Input verts are already in clip coord / NDC space. No transform.
    gl_Position = gl_Vertex;
}
