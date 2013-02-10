
BDFX INCLUDE shaders/effects/declarations.common

void main( void )
{
    oTC = (gl_Vertex.xy + 1.0) * 0.5;
    oTC *= texturePercent;

    gl_Position = gl_Vertex;
}
