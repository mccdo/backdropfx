// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

uniform mat4 viewProj;
uniform mat4 celestialOrientation;

uniform mat4 sunTransform;

void main()
{
    // TBD GL3
    gl_Position = viewProj * celestialOrientation * sunTransform * gl_Vertex;
}
