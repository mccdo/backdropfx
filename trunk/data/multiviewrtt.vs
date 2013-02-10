
varying vec2 oTC;

void main()
{
    oTC = ( gl_Vertex.st + 1.0 ) * 0.5;
    float x = ( gl_Vertex.x + 1. ) * .33 - 1.;
    float y = ( gl_Vertex.y + 1. ) * .5 - 1.;
    gl_Position = vec4( x, y, 0., 1. );
}
