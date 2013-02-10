//#version 120
//#extension GL_ARB_draw_instanced : require

BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

BDFX INCLUDE shaders/gl2/bdfx-depthpeel-declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/streamline-transform.vs


// Vertex processing for streamline rendering, compatible
// with backdropFX depth peeling feature.

// Orient the streamline "point" billboards to face the viewer.
mat3
makeOrientMat( const in vec3 dir )
{
    // Compute a vector at a right angle to the direction.
    // First try projecting direction into xy rotated -90 degrees.
    // If that gives us a very short vector,
    // then project into yz instead, rotated -90 degrees.
    vec3 c = vec3( dir.y, -dir.x, 0.0 );
    if( dot( c, c ) < 0.1 )
        c = vec3( 0.0, dir.z, -dir.y );
        
    // Appears to be a bug in normalize when z==0
    //normalize( c.xyz );
    float l = length( c );
    c /= l;

    vec3 up = normalize( cross( dir, c ) );

    // Orientation uses the cross product vector as x,
    // the up vector as y, and the direction vector as z.
    return( mat3( c, up, dir ) );
}


struct streamlineParams {
    vec2 sizes;
    sampler2D positions;
    int totalInstances;
    int numTraces;
    float traceInterval;
    int traceLength;
};
uniform streamlineParams streamline;

void computeTransform()
{
    // Pass texture coords of tri pair to fragment processing.
    // TBD do we _know_ we need to use bdfx_multiTexCoord1? Can we make this
    // more flexible for the client app?
    bdfx_outTexCoord1 = bdfx_multiTexCoord1;

    // Using the instance ID, generate "texture coords" for this instance.
    float fInstanceID = float( gl_InstanceID );
    float r = fInstanceID / streamline.sizes.x;
    vec2 tC;
    tC.s = fract( r ); tC.t = floor( r ) / streamline.sizes.y;

    // Get position from the texture.
    vec4 instancePos = texture2D( streamline.positions, tC );

    // Compute orientation
    vec4 eye = bdfx_modelViewMatrixInverse * vec4( 0., 0., 0., 1. );
    vec3 direction = normalize( eye.xyz - instancePos.xyz );
    mat3 orient = makeOrientMat( direction );
    // Orient the incoming vertices and translate by the instance position.
    vec4 modelPos = vec4( orient * bdfx_vertex.xyz, 0. ) + instancePos;
    // Transform into clip coordinates.
    vec4 cc = bdfx_projection * bdfx_modelViewMatrix * modelPos;
    gl_Position = cc;



    // Compute the length of a trace segment, in points.
    float segLength = float( streamline.totalInstances ) / float( streamline.numTraces );
    // Use current time to compute an offset in points for the animation.
    float time = mod( osg_SimulationTime, streamline.traceInterval );
    float pointOffset = ( time / streamline.traceInterval ) * segLength;

    // Find the segment tail for this point's relavant segment.
    float segTail = floor( (fInstanceID - pointOffset) / segLength ) * segLength + pointOffset;
    // ...and the head, which will have full intensity alpha.
    float segHead = floor( segTail + segLength );

#if 1
    // Use smoothstep to fade from the head to the traceLength.
    float alpha = smoothstep( segHead - float( streamline.traceLength ), segHead, fInstanceID );
#else
    // Alternative: Use step() instead for no fade.
    float alpha = step( segHead - streamline.traceLength, fInstanceID );
#endif

    vec4 color = bdfx_color;
    color.a *= alpha;
    bdfx_processedColor = color;

    // For debugging:
    //bdfx_processedColor = vec4( vec3( alpha ), 1.0 );
}

// END gl2/streamline-transform.vs

