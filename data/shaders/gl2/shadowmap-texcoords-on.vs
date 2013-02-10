#version 120

BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs
BDFX INCLUDE shaders/gl2/shadowmap-declarations.common
BDFX INCLUDE shaders/gl2/shadowmap-declarations.vs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/shadowmap-texcoords-on.vs

void computeShadowTexCoords()
{
    // This is basically FFP TexGen for EYE_LINEAR, but we're taking the eye planes
    // from the rows of bdfx_shadowViewProj (the matrix used to create the depth map).
    int idx;
    for( idx=0; idx<4; idx++ )
    {
        vec4 ep = vec4( bdfx_shadowViewProj[ 0 ][ idx ],
                        bdfx_shadowViewProj[ 1 ][ idx ],
                        bdfx_shadowViewProj[ 2 ][ idx ],
                        bdfx_shadowViewProj[ 3 ][ idx ] );
        bdfx_outShadowTexCoord[ idx ] = dot( bdfx_eyeVertex, ( ep * osg_ViewMatrixInverse ) );
    }
}

// END gl2/shadowmap-texcoords-on.vs

