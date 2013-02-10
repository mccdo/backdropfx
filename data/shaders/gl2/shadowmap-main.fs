#version 120

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/shadowmap-main.fs


// This shader is used during rendering via ShadowMapStage
// to create the depth maps. Rendering features such as lighting
// or texture mapping are irrelevant, as the only output we need
// is the depth buffer.

void main()
{
    gl_FragData[ 0 ] = vec4( 0., 0., 0., 1. );
}

// END gl2/shadowmap-main.fs

