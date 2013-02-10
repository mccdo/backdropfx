
uniform samplerCube tex;

uniform vec3 up;

varying vec3 oTC;

void main()
{
    vec4 cloudColor = textureCube( tex, oTC );

    // TBD GL3
    gl_FragColor = cloudColor;
}
