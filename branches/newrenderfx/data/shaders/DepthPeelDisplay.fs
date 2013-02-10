// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

uniform sampler2D depthPeelTexture;
varying vec2 oTC;

void main( void )
{
    gl_FragColor = texture2D( depthPeelTexture, oTC );
}
