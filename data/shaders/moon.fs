
uniform sampler2D tex;

varying vec2 oTC;
varying vec3 oNormal;

uniform vec3 bdfx_sunPosition;

void main()
{
    vec3 normal = normalize( oNormal );
    float diffuse = max( dot( bdfx_sunPosition, normal ), 0.1 );

    // TBD GL3
    gl_FragColor = texture2D( tex, oTC ) * diffuse;
}
