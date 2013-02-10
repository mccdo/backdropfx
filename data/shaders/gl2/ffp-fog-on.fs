
BDFX INCLUDE shaders/gl2/ffp-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.common
BDFX INCLUDE shaders/gl2/ffp-declarations-fog.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-fog-on.fs

// Input:
//   varying float bdfx_outFogFragCoord
//   uniform struct bdfx_fogParameters bdfx_fog

// Output:
// vec4 bdfx_processedColor;

void computeFogFragment()
{
	float fog;

	if(bdfx_fog.mode == FOG_LINEAR)
	{
		fog = (gl_Fog.end - bdfx_outFogFragCoord) * gl_Fog.scale;
	} // if FOG_LINEAR
	else if(bdfx_fog.mode == FOG_EXP)
	{
		fog = exp(-gl_Fog.density * bdfx_outFogFragCoord);
	} // if FOG_EXP
	else if(bdfx_fog.mode == FOG_EXP2)
	{
		fog = exp(-gl_Fog.density * gl_Fog.density * bdfx_outFogFragCoord * bdfx_outFogFragCoord);
	} // if FOG_EXP2

	fog = clamp(fog, 0.0, 1.0);

	// mix in fog
	bdfx_processedColor = mix(bdfx_fog.color, bdfx_processedColor, fog);

} // computeFogFragment

// END gl2/ffp-fog-on.fs

