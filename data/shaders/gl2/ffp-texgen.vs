
// shaders/gl2/ffp-declarations.common
// shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-texgen.vs


vec4 computeTexGenLayer(vec4 inputCoord, int idx, int texGenMode)
{
    vec3 ecPosition3;

    if( texGenMode == 0 ) return(inputCoord);
    if( texGenMode == GL_EYE_LINEAR ) // EYE LINEAR
    {
        /*
        // from orange book, listing 9.22
        gl_TexCoord[i].s = dot(ecPosition, gl_EyePlaneS[i]);
        gl_TexCoord[i].t = dot(ecPosition, gl_EyePlaneT[i]);
        gl_TexCoord[i].p = dot(ecPosition, gl_EyePlaneR[i]);
        gl_TexCoord[i].q = dot(ecPosition, gl_EyePlaneQ[i]);
        */
        // ecPosition   == bdfx_eyeVertex (precalculated by ffp-eyecoords)
        // gl_EyePlane* == bdfx_eyePlane* (preloaded by ShaderModuleVisitor)

        // bdfx_eyeVertex relies on ffp-eyecoords-on.vs being run, no way to enforce this from here
        if( bdfx_eyeAbsolute == 1 )
        {
            float s, t, p, q;
            s = dot(bdfx_eyeVertex, bdfx_eyePlaneS[idx]);
            t = dot(bdfx_eyeVertex, bdfx_eyePlaneT[idx]);
            p = dot(bdfx_eyeVertex, bdfx_eyePlaneR[idx]);
            q = dot(bdfx_eyeVertex, bdfx_eyePlaneQ[idx]);
            return(vec4( s, t, p, q ));
        }
        else
        {
            float s, t, p, q;
            s = dot(bdfx_eyeVertex, ( bdfx_eyePlaneS[idx] * osg_ViewMatrixInverse ) );
            t = dot(bdfx_eyeVertex, ( bdfx_eyePlaneT[idx] * osg_ViewMatrixInverse ) );
            p = dot(bdfx_eyeVertex, ( bdfx_eyePlaneR[idx] * osg_ViewMatrixInverse ) );
            q = dot(bdfx_eyeVertex, ( bdfx_eyePlaneQ[idx] * osg_ViewMatrixInverse ) );
            return(vec4( s, t, p, q ));
        }
    } // GL_EYE_LINEAR
    else if( texGenMode == GL_OBJECT_LINEAR ) // OBJECT LINEAR
    {
        // cf. orange book, listing 9.22
        float s, t, p, q;
        s = dot(bdfx_vertex, bdfx_objectPlaneS[idx]);
        t = dot(bdfx_vertex, bdfx_objectPlaneT[idx]);
        p = dot(bdfx_vertex, bdfx_objectPlaneR[idx]);
        q = dot(bdfx_vertex, bdfx_objectPlaneQ[idx]);
        return(vec4( s, t, p, q ));
    } // GL_OBJECT_LINEAR
    else if( texGenMode == GL_SPHERE_MAP ) // sphere
    {
        vec3 u = normalize( bdfx_eyeVertex.xyz );
        vec3 r = u - 2.0 * bdfx_eyeNormal * dot( u, bdfx_eyeNormal );
        float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
        return(vec4( r.x/m + 0.5, r.y/m + 0.5, 0., 1. ));
    } // GL_SPHERE_MAP
    if( texGenMode == GL_NORMAL_MAP ) // NORMAL MAP
    {
		ecPosition3 = (vec3(bdfx_eyeVertex)) / bdfx_eyeVertex.w;
        return(vec4(bdfx_eyeNormal, 1.0));
    } // GL_NORMAL_MAP
    if( texGenMode == GL_REFLECTION_MAP ) // REFLECTION MAP
    {
        // cf. orange book, listing 9.21
        vec3 u;
		ecPosition3 = (vec3(bdfx_eyeVertex)) / bdfx_eyeVertex.w;
        u = normalize(ecPosition3); // this could be hoisted out to where we compute ecPosition3 above
        return(vec4(reflect(u, bdfx_eyeNormal), 0.0)); // explicitly promote vec3 up to vec4 by appending 0
    } // GL_REFLECTION_MAP

return(inputCoord);

} // computeTexGenLayer

void computeTexGen()
{
    bdfx_outTexCoord0 = bdfx_multiTexCoord0;
    bdfx_outTexCoord1 = bdfx_multiTexCoord1;
    bdfx_outTexCoord2 = bdfx_multiTexCoord2;
    bdfx_outTexCoord3 = bdfx_multiTexCoord3;

	// only 4 texgens are supported here
	bdfx_outTexCoord0 = computeTexGenLayer(bdfx_outTexCoord0, 0, bdfx_texGen0);
	bdfx_outTexCoord1 = computeTexGenLayer(bdfx_outTexCoord1, 1, bdfx_texGen1);
	bdfx_outTexCoord2 = computeTexGenLayer(bdfx_outTexCoord2, 2, bdfx_texGen2);
	bdfx_outTexCoord3 = computeTexGenLayer(bdfx_outTexCoord3, 3, bdfx_texGen3);

}

// END gl2/ffp-texgen.vs

