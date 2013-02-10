
uniform mat4 viewProj;
uniform mat4 celestialOrientation;

varying vec3 oTC;

void main()
{
    // TBD GL3
    vec4 t = celestialOrientation * vec4(gl_Normal, 1.0);
    oTC = t.xyz;
    
    // TBD GL3
    gl_Position = viewProj * celestialOrientation * gl_Vertex;
}
