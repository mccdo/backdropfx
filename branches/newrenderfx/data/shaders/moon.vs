
uniform mat4 viewProj;
uniform mat4 celestialOrientation;

uniform mat4 moonTransform;
uniform mat3 moonOrientation;

varying vec2 oTC;
varying vec3 oNormal;

void main()
{
    // TBD GL3
    vec4 normal = celestialOrientation * vec4( moonOrientation * gl_Normal, 1.0 );
    oNormal = normal.xyz;

    // TBD GL3
    oTC = gl_MultiTexCoord0.st;
    
    // TBD GL3
    gl_Position = viewProj * celestialOrientation * moonTransform * gl_Vertex;
}
