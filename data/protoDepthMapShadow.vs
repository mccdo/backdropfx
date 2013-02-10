#version 120


uniform mat4 osg_ViewMatrixInverse;
uniform mat4 shadowViewProj;

varying vec4 outTexCoords;

void main()
{
    gl_Position = ftransform();


    vec4 eyeVertex = gl_ModelViewMatrix * gl_Vertex;
    int idx;
    for( idx=0; idx<4; idx++ )
    {
        vec4 ep = vec4( shadowViewProj[ 0 ][ idx ],
                        shadowViewProj[ 1 ][ idx ],
                        shadowViewProj[ 2 ][ idx ],
                        shadowViewProj[ 3 ][ idx ] );
        outTexCoords[ idx ] = dot( eyeVertex, ( ep * osg_ViewMatrixInverse ) );
    }
}
