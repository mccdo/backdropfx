
const vec3 lightDir = vec3( 0, 0, 1 );

vec3 computeDiffuse( vec4 color, vec3 nPrime )
{
    float dotProd = max( dot( nPrime, lightDir ), 0.0 ) * .75;
    return( color.rgb * dotProd );
}
vec3 computeSpecular( vec3 nPrime )
{
    const vec3 eyeVec = vec3( 0, 0, -1 );
    vec3 reflectVec = reflect( lightDir, nPrime );
    float dotProd = max( dot( reflectVec, eyeVec ), 0.0 );
    dotProd = pow( dotProd, 16.0 ) * .7;
    return( vec3( dotProd, dotProd, dotProd ) );
}


vec4 computeLight( vec4 color, vec3 normal )
{
    const vec3 ambient = vec3( .25, .25, .25 );
    vec3 diffuse = computeDiffuse( color, normal );
    vec3 spec = computeSpecular( normal );
    vec3 sum = ambient + diffuse + spec;
    return( vec4( sum, color.a ) );
}
