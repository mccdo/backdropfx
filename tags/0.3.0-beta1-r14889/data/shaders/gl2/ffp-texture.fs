
// shaders/gl2/ffp-declarations.common
// shaders/gl2/ffp-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-texture.fs


vec4 computeTextureLayer2D(vec2 tc, sampler2D texture2dSampler, int textureEnvMode, int idx)
{
	vec4 texColor = texture2D( texture2dSampler, tc );

	// TBD more TexEnv
	// Modulate
	if(textureEnvMode == GL_DECAL)
	{
		vec3 col = mix(bdfx_processedColor.rgb, texColor.rgb, texColor.a);
		return(vec4(col, bdfx_processedColor.a));
	} // DECAL
	else if(textureEnvMode == GL_BLEND)
	{
		vec3 col = mix(bdfx_processedColor.rgb, gl_TextureEnvColor[0].rgb, texColor.rgb);
		return(vec4(col, bdfx_processedColor.a * texColor.a));
	} // BLEND
	else if(textureEnvMode == GL_REPLACE)
	{
		return(vec4(texColor.rgb, bdfx_processedColor.a));
	} // REPLACE
	else if(textureEnvMode == GL_ADD)
	{
		bdfx_processedColor.rgb += texColor.rgb;
		bdfx_processedColor.a   *= texColor.a;
		return(clamp(bdfx_processedColor, 0.0, 1.0));
	} // ADD
	else // "GL_TEXTURE_ENV_MODE defaults to GL_MODULATE" http://www.opengl.org/sdk/docs/man/xhtml/glTexEnv.xml
	{
		bdfx_processedColor.rgb = texColor.rgb * bdfx_processedColor.rgb;
		// Combine texture alpha with texture alpha. (for streamlines only?)
		bdfx_processedColor.a *= texColor.a;
		return(bdfx_processedColor);
	} // MODULATE

} // computeTextureLayer2D


void computeTexture()
{
	vec2 tc;
	
	if(bdfx_texture2dEnable0 == 1)
	{
		if( bdfx_pointSprite == 1 )
			tc = gl_PointCoord.st;
		else
			tc = bdfx_outTexCoord0.st;
		bdfx_processedColor = computeTextureLayer2D(tc, texture2dSampler0, bdfx_textureEnvMode0, 0);
	} 

	if(bdfx_texture2dEnable1 == 1)
	{
		if( bdfx_pointSprite == 1 )
			tc = gl_PointCoord.st;
		else
			tc = bdfx_outTexCoord1.st;
		bdfx_processedColor = computeTextureLayer2D(tc, texture2dSampler1, bdfx_textureEnvMode1, 1);
	} 

	if(bdfx_texture2dEnable2 == 1)
	{
		if( bdfx_pointSprite == 1 )
			tc = gl_PointCoord.st;
		else
			tc = bdfx_outTexCoord2.st;
		bdfx_processedColor = computeTextureLayer2D(tc, texture2dSampler2, bdfx_textureEnvMode2, 2);
	} 

	if(bdfx_texture2dEnable3 == 1)
	{
		if( bdfx_pointSprite == 1 )
			tc = gl_PointCoord.st;
		else
			tc = bdfx_outTexCoord3.st;
		bdfx_processedColor = computeTextureLayer2D(tc, texture2dSampler3, bdfx_textureEnvMode3, 3);
	} 

}

// END gl2/ffp-texture.fs

