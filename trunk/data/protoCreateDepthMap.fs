#version 120


void main()
{
    // Output color doesn't matter. We just want the depth map.
    gl_FragData[ 0 ] = vec4( 0., 0., 0., 1. );
}
