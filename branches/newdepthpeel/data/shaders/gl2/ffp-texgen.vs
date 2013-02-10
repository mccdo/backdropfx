
// shaders/gl2/ffp-declarations.common
// shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-texgen.vs


void computeTexGen()
{
    bdfx_outTexCoord[ 0 ] = bdfx_multiTexCoord0;
    bdfx_outTexCoord[ 1 ] = bdfx_multiTexCoord1;
    bdfx_outTexCoord[ 2 ] = bdfx_multiTexCoord2;
    bdfx_outTexCoord[ 3 ] = bdfx_multiTexCoord3;

    int idx;
    for( idx=0; idx<bdfx_maxTextureCoords; idx++ )
    {
        if( bdfx_texGen[ idx ] == 1 )
        {
            vec2 tc = ( bdfx_eyeNormal.xy + vec2( 1.0, 1.0 ) ) * .5;
            bdfx_outTexCoord[ idx ] = vec4( tc, 0., 1. );
        }
    }
}

// END gl2/ffp-texgen.vs

