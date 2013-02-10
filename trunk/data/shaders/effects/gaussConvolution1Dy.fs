
BDFX INCLUDE shaders/effects/declarations.common

//These could be uniforms
const float WT9_0 = 0.5;
const float WT9_1 = 0.4;
const float WT9_2 = 0.3;
const float WT9_3 = 0.2;
const float WT9_4 = 0.1;

varying vec4 TC[ 2 ];

void main()
{
    float WT9_N = ( WT9_0 + 2.0 * ( WT9_1 + WT9_2 + WT9_3 + WT9_4 ) );
    float WT9_0N = WT9_0 / WT9_N;
    float WT9_1N = WT9_1 / WT9_N;
    float WT9_2N = WT9_2 / WT9_N;
    float WT9_3N = WT9_3 / WT9_N;
    float WT9_4N = WT9_4 / WT9_N;

    vec3 oC = texture2D( inputTexture0, oTC ).rgb * WT9_0N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 0 ].s ) ).rgb * WT9_1N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 0 ].t ) ).rgb * WT9_1N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 0 ].p ) ).rgb * WT9_2N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 0 ].q ) ).rgb * WT9_2N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 1 ].s ) ).rgb * WT9_3N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 1 ].t ) ).rgb * WT9_3N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 1 ].p ) ).rgb * WT9_4N;
    oC     += texture2D( inputTexture0, vec2( oTC.s, TC[ 1 ].q ) ).rgb * WT9_4N;

    gl_FragData[ 0 ] = vec4( oC, 1.0 );
}
