#version 120


uniform bool displayTC;

uniform mat4 shadowViewProj;

uniform sampler2DShadow depthTex;

varying vec4 outTexCoords;

void main()
{
    if( displayTC )
    {
        vec3 tc = outTexCoords.stp + outTexCoords.q;
        tc /= ( 2. * outTexCoords.q );
        if( (tc.s<0.) || (tc.t<0.) || (tc.s>1.) || (tc.t>1.) )
            // Outside texture range, render black.
            gl_FragData[ 0 ] = vec4( 0., 0., 0., 1. );
        else
            // stp displayed as rgb values.
            gl_FragData[ 0 ] = vec4( tc, 1. );
    }
    else
    {
        vec3 tc = outTexCoords.stp + outTexCoords.q;
        tc *= .5;
        if( (tc.s>0.) && (tc.t>0.) && (tc.s<outTexCoords.q) && (tc.t<outTexCoords.q) )
        {
            float fullyLit = shadow2DProj( depthTex, vec4( tc, outTexCoords.q ) ).a;
            if( fullyLit == 1.0 )
                gl_FragData[ 0 ] = vec4( .8, .8, 1., 1. );
            else
                gl_FragData[ 0 ] = vec4( .25, .1, .1, 1. );
        }
        else
            gl_FragData[ 0 ] = vec4( .8, .8, 1., 1. );
    }
}
