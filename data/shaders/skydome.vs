// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

uniform mat4 viewProj;
uniform mat4 celestialOrientation;

varying vec3 oTC;

uniform vec3 up;
uniform vec3 bdfx_sunPosition;
varying float dayCoeff, duskCoeff, nightCoeff;

void main()
{
    // TBD GL3
    vec4 t = celestialOrientation * vec4(gl_Normal, 1.0);
    oTC = t.xyz;
    
    // TBD GL3
    gl_Position = viewProj * celestialOrientation * gl_Vertex;

    dayCoeff = 1.0;
    duskCoeff = 0.0;
    nightCoeff = 0.0;

    float dotSun = dot( bdfx_sunPosition, up );
    const float horizLimit = 0.2;
    if( dotSun < horizLimit )
    {
        if( dotSun > 0.0 )
        {
            float dayDuskMix = clamp( dotSun, 0.0, horizLimit ) / horizLimit;
            dayCoeff = dayDuskMix;
            duskCoeff = 1.0 - dayDuskMix;
            nightCoeff = 0.0;
        }
        else
        {
            float duskNightMix = clamp( dotSun, -horizLimit, 0.0 ) / horizLimit + 1.0;
            // Night
            dayCoeff = 0.0;
            duskCoeff = duskNightMix;
            nightCoeff = 1.0 - duskNightMix;
        }
    }
}
