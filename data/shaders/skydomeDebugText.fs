
uniform vec3 up;

uniform sampler2D fontTex;
varying vec2 oTC;

void main()
{
    vec4 color = texture2D( fontTex, oTC );
    vec3 fColor = mix( vec3( 0.0, 0.1, 0.5 ),
        vec3( 0.6, 0.8, 1.0 ), color.a );

    // TBD GL3
    gl_FragColor = vec4( fColor, 1.0 );
}
