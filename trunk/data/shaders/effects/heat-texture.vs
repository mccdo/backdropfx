#version 120

BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// effects/heat-texture.vs


varying vec2 offsetTC;

void main( void )
{
    vec2 tc = (gl_Vertex.xy + 1.0) * 0.5;
    float s = tc.s;
    float t = tc.t - ( rfx_simulationTime * .35 );
    offsetTC = vec2( s, t );
    oTC = tc;

    gl_Position = gl_Vertex;
}

// effects/heat-texture.vs


