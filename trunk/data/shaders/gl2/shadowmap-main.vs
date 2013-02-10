#version 120

BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/shadowmap-declarations.vs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/shadowmap-main.vs


// This shader is used during rendering via ShadowMapStage
// to create the depth maps. Rendering features such as lighting
// or texture mapping are irrelevant, as the only output we need
// is the depth buffer.

void main()
{
    mat4 modelMatrix = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    gl_Position = bdfx_shadowViewProj * modelMatrix * gl_Vertex;
}

// END gl2/shadowmap-main.vs

