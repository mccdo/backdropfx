
varying vec2 oTC;

void main()
{
    oTC = ( gl_Vertex.st + 1.0 ) * 0.5;
    gl_Position = gl_Vertex;
}
