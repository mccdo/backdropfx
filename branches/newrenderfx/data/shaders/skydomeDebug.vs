
uniform mat4 viewProj;
uniform mat4 celestialOrientation;

void main()
{
    // TBD GL3
    gl_Position = viewProj * celestialOrientation * gl_Vertex;
}
