 
// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/noise-noise.vs.fs



/*
 * 2D, 3D and 4D Perlin noise, classic and simplex, in a GLSL fragment shader.
 *
 * Classic noise is implemented by the functions:
 * float noise(vec2 P)
 * float noise(vec3 P)
 * float noise(vec4 P)
 *
 * Simplex noise is implemented by the functions:
 * float snoise(vec2 P)
 * float snoise(vec3 P)
 * float snoise(vec4 P)
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 * Simplex indexing functions by Bill Licea-Kane, ATI (bill@ati.com)
 *
 * You may use, modify and redistribute this code free of charge,
 * provided that the author's names and this notice appear intact.
 */

/*
 * NOTE: there is a formal problem with the dependent texture lookups.
 * A texture coordinate of exactly 1.0 will wrap to 0.0, so strictly speaking,
 * an error occurs every 256 units of the texture domain, and the same gradient
 * is used for two adjacent noise cells. One solution is to set the texture
 * wrap mode to "CLAMP" and do the wrapping explicitly in GLSL with the "mod"
 * operator. This could also give you noise with repetition intervals other
 * than 256 without any extra cost.
 * This error is not even noticeable to the eye even if you isolate the exact
 * position in the domain where it occurs and know exactly what to look for.
 * The noise pattern is still visually correct, so I left the bug in there.
 * 
 * The value of classic 4D noise goes above 1.0 and below -1.0 at some
 * points. Not much and only very sparsely, but it happens. This is a
 * bug from the original software implementation, so I left it untouched.
 */


/*
 * "permTexture" is a 256x256 texture that is used for both the permutations
 * and the 2D and 3D gradient lookup. For details, see the main C program.
 * "gradTexture" is a 256x256 texture with 4D gradients, similar to
 * "permTexture" but with the permutation index in the alpha component
 * replaced by the w component of the 4D gradient.
 * 2D classic noise uses only permTexture.
 * 2D simplex noise uses only permTexture.
 * 3D classic noise uses only permTexture.
 * 3D simplex noise uses only permTexture.
 * 4D classic noise uses permTexture and gradTexture.
 * 4D simplex noise uses permTexture and gradTexture.
 */
uniform sampler2D permTexture;
uniform float time; // Used for texture animation

/*
 * Both 2D and 3D texture coordinates are defined, for testing purposes.
 */
varying vec2 v_texCoord2D;
varying vec3 v_texCoord3D;


/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another texture size.


/*
 * The interpolation function. This could be a 1D texture lookup
 * to get some more speed, but it's not the main part of the algorithm.
 */
float fade(float t)
{
    // return t*t*(3.0-2.0*t); // Old fade, yields discontinuous second derivative
    return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
}

/*
 * Efficient simplex indexing functions by Bill Licea-Kane, ATI. Thanks!
 * (This was originally implemented as a texture lookup. Nice to avoid that.)
 */
void simplex( const in vec3 P, out vec3 offset1, out vec3 offset2 )
{
    vec3 offset0;

    vec2 isX = step( P.yz, P.xx );         // P.x >= P.y ? 1.0 : 0.0;  P.x >= P.z ? 1.0 : 0.0;
    offset0.x  = dot( isX, vec2( 1.0 ) );  // Accumulate all P.x >= other channels in offset.x
    offset0.yz = 1.0 - isX;                // Accumulate all P.x <  other channels in offset.yz

    float isY = step( P.z, P.y );          // P.y >= P.z ? 1.0 : 0.0;
    offset0.y += isY;                      // Accumulate P.y >= P.z in offset.y
    offset0.z += 1.0 - isY;                // Accumulate P.y <  P.z in offset.z

    // offset0 now contains the unique values 0,1,2 in each channel
    // 2 for the channel greater than other channels
    // 1 for the channel that is less than one but greater than another
    // 0 for the channel less than other channels
    // Equality ties are broken in favor of first x, then y
    // (z always loses ties)

    offset2 = clamp(   offset0, 0.0, 1.0 );
    // offset2 contains 1 in each channel that was 1 or 2
    offset1 = clamp( --offset0, 0.0, 1.0 );
    // offset1 contains 1 in the single channel that was 1
}

void simplex( const in vec4 P, out vec4 offset1, out vec4 offset2, out vec4 offset3 )
{
    vec4 offset0;

    vec3 isX = step( P.yzw, P.xxx );        // See comments in 3D simplex function
    offset0.x = dot( isX, vec3( 1.0 ) );
    offset0.yzw = 1.0 - isX;

    vec2 isY = step( P.zw, P.yy );
    offset0.y += dot( isY, vec2( 1.0 ) );
    offset0.zw += 1.0 - isY;

    float isZ = step( P.w, P.z );
    offset0.z += isZ;
    offset0.w += 1.0 - isZ;

    // offset0 now contains the unique values 0,1,2,3 in each channel

    offset3 = clamp(   offset0, 0.0, 1.0 );
    offset2 = clamp( --offset0, 0.0, 1.0 );
    offset1 = clamp( --offset0, 0.0, 1.0 );
}


/*
 * 2D classic Perlin noise. Fast, but less useful than 3D noise.
 */
float noise( vec2 P )
{
    vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
    vec2 Pf = fract(P);             // Fractional part for interpolation

    // Noise contribution from lower left corner
    vec2 grad00 = texture2D(permTexture, Pi).rg * 4.0 - 1.0;
    float n00 = dot(grad00, Pf);

    // Noise contribution from lower right corner
    vec2 grad10 = texture2D(permTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
    float n10 = dot(grad10, Pf - vec2(1.0, 0.0));

    // Noise contribution from upper left corner
    vec2 grad01 = texture2D(permTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
    float n01 = dot(grad01, Pf - vec2(0.0, 1.0));

    // Noise contribution from upper right corner
    vec2 grad11 = texture2D(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
    float n11 = dot(grad11, Pf - vec2(1.0, 1.0));

    // Blend contributions along x
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade(Pf.x));

    // Blend contributions along y
    float n_xy = mix(n_x.x, n_x.y, fade(Pf.y));

    // We're done, return the final noise value.
    return n_xy;
}


/*
 * 3D classic noise. Slower, but a lot more useful than 2D noise.
 */
float noise( vec3 P )
{
    vec3 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled so +1 moves one texel
                                  // and offset 1/2 texel to sample texel centers
    vec3 Pf = fract(P);     // Fractional part for interpolation

    // Noise contributions from (x=0, y=0), z=0 and z=1
    float perm00 = texture2D(permTexture, Pi.xy).a ;
    vec3  grad000 = texture2D(permTexture, vec2(perm00, Pi.z)).rgb * 4.0 - 1.0;
    float n000 = dot(grad000, Pf);
    vec3  grad001 = texture2D(permTexture, vec2(perm00, Pi.z + ONE)).rgb * 4.0 - 1.0;
    float n001 = dot(grad001, Pf - vec3(0.0, 0.0, 1.0));

    // Noise contributions from (x=0, y=1), z=0 and z=1
    float perm01 = texture2D(permTexture, Pi.xy + vec2(0.0, ONE)).a ;
    vec3  grad010 = texture2D(permTexture, vec2(perm01, Pi.z)).rgb * 4.0 - 1.0;
    float n010 = dot(grad010, Pf - vec3(0.0, 1.0, 0.0));
    vec3  grad011 = texture2D(permTexture, vec2(perm01, Pi.z + ONE)).rgb * 4.0 - 1.0;
    float n011 = dot(grad011, Pf - vec3(0.0, 1.0, 1.0));

    // Noise contributions from (x=1, y=0), z=0 and z=1
    float perm10 = texture2D(permTexture, Pi.xy + vec2(ONE, 0.0)).a ;
    vec3  grad100 = texture2D(permTexture, vec2(perm10, Pi.z)).rgb * 4.0 - 1.0;
    float n100 = dot(grad100, Pf - vec3(1.0, 0.0, 0.0));
    vec3  grad101 = texture2D(permTexture, vec2(perm10, Pi.z + ONE)).rgb * 4.0 - 1.0;
    float n101 = dot(grad101, Pf - vec3(1.0, 0.0, 1.0));

    // Noise contributions from (x=1, y=1), z=0 and z=1
    float perm11 = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a ;
    vec3  grad110 = texture2D(permTexture, vec2(perm11, Pi.z)).rgb * 4.0 - 1.0;
    float n110 = dot(grad110, Pf - vec3(1.0, 1.0, 0.0));
    vec3  grad111 = texture2D(permTexture, vec2(perm11, Pi.z + ONE)).rgb * 4.0 - 1.0;
    float n111 = dot(grad111, Pf - vec3(1.0, 1.0, 1.0));

    // Blend contributions along x
    vec4 n_x = mix(vec4(n000, n001, n010, n011),
                 vec4(n100, n101, n110, n111), fade(Pf.x));

    // Blend contributions along y
    vec2 n_xy = mix(n_x.xy, n_x.zw, fade(Pf.y));

    // Blend contributions along z
    float n_xyz = mix(n_xy.x, n_xy.y, fade(Pf.z));

    // We're done, return the final noise value.
    return n_xyz;
}



/*
 * 2D simplex noise. Somewhat slower but much better looking than classic noise.
 */
float snoise( vec2 P )
{
// Skew and unskew factors are a bit hairy for 2D, so define them as constants
// This is (sqrt(3.0)-1.0)/2.0
#define F2 0.366025403784
// This is (3.0-sqrt(3.0))/6.0
#define G2 0.211324865405

    // Skew the (x,y) space to determine which cell of 2 simplices we're in
    float s = (P.x + P.y) * F2;   // Hairy factor for 2D skewing
    vec2 Pi = floor(P + s);
    float t = (Pi.x + Pi.y) * G2; // Hairy factor for unskewing
    vec2 P0 = Pi - t; // Unskew the cell origin back to (x,y) space
    Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

    vec2 Pf0 = P - P0;  // The x,y distances from the cell origin

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Find out whether we are above or below the x=y diagonal to
    // determine which of the two triangles we're in.
    vec2 o1;
    if(Pf0.x > Pf0.y) o1 = vec2(1.0, 0.0);  // +x, +y traversal order
    else o1 = vec2(0.0, 1.0);               // +y, +x traversal order

    // Noise contribution from simplex origin
    vec2 grad0 = texture2D(permTexture, Pi).rg * 4.0 - 1.0;
    float t0 = 0.5 - dot(Pf0, Pf0);
    float n0;
    if (t0 < 0.0) n0 = 0.0;
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad0, Pf0);
    }

    // Noise contribution from middle corner
    vec2 Pf1 = Pf0 - o1 + G2;
    vec2 grad1 = texture2D(permTexture, Pi + o1*ONE).rg * 4.0 - 1.0;
    float t1 = 0.5 - dot(Pf1, Pf1);
    float n1;
    if (t1 < 0.0) n1 = 0.0;
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad1, Pf1);
    }

    // Noise contribution from last corner
    vec2 Pf2 = Pf0 - vec2(1.0-2.0*G2);
    vec2 grad2 = texture2D(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
    float t2 = 0.5 - dot(Pf2, Pf2);
    float n2;
    if(t2 < 0.0) n2 = 0.0;
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad2, Pf2);
    }

    // Sum up and scale the result to cover the range [-1,1]
    return 70.0 * (n0 + n1 + n2);
}


/*
 * 3D simplex noise. Comparable in speed to classic noise, better looking.
 */
float snoise( vec3 P )
{
// The skewing and unskewing factors are much simpler for the 3D case
#define F3 0.333333333333
#define G3 0.166666666667

    // Skew the (x,y,z) space to determine which cell of 6 simplices we're in
    float s = (P.x + P.y + P.z) * F3; // Factor for 3D skewing
    vec3 Pi = floor(P + s);
    float t = (Pi.x + Pi.y + Pi.z) * G3;
    vec3 P0 = Pi - t; // Unskew the cell origin back to (x,y,z) space
    Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

    vec3 Pf0 = P - P0;  // The x,y distances from the cell origin

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // To find out which of the six possible tetrahedra we're in, we need to
    // determine the magnitude ordering of x, y and z components of Pf0.
    vec3 o1;
    vec3 o2;
    simplex(Pf0, o1, o2);

    // Noise contribution from simplex origin
    float perm0 = texture2D(permTexture, Pi.xy).a;
    vec3  grad0 = texture2D(permTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
    float t0 = 0.6 - dot(Pf0, Pf0);
    float n0;
    if (t0 < 0.0) n0 = 0.0;
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad0, Pf0);
    }

    // Noise contribution from second corner
    vec3 Pf1 = Pf0 - o1 + G3;
    float perm1 = texture2D(permTexture, Pi.xy + o1.xy*ONE).a;
    vec3  grad1 = texture2D(permTexture, vec2(perm1, Pi.z + o1.z*ONE)).rgb * 4.0 - 1.0;
    float t1 = 0.6 - dot(Pf1, Pf1);
    float n1;
    if (t1 < 0.0) n1 = 0.0;
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad1, Pf1);
    }

    // Noise contribution from third corner
    vec3 Pf2 = Pf0 - o2 + 2.0 * G3;
    float perm2 = texture2D(permTexture, Pi.xy + o2.xy*ONE).a;
    vec3  grad2 = texture2D(permTexture, vec2(perm2, Pi.z + o2.z*ONE)).rgb * 4.0 - 1.0;
    float t2 = 0.6 - dot(Pf2, Pf2);
    float n2;
    if (t2 < 0.0) n2 = 0.0;
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad2, Pf2);
    }

    // Noise contribution from last corner
    vec3 Pf3 = Pf0 - vec3(1.0-3.0*G3);
    float perm3 = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a;
    vec3  grad3 = texture2D(permTexture, vec2(perm3, Pi.z + ONE)).rgb * 4.0 - 1.0;
    float t3 = 0.6 - dot(Pf3, Pf3);
    float n3;
    if(t3 < 0.0) n3 = 0.0;
    else
    {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad3, Pf3);
    }

    // Sum up and scale the result to cover the range [-1,1]
    return( 32.0 * (n0 + n1 + n2 + n3) );
}



// octaves can be 1...5
// returns roughly +-1.0, but can go out of range, so clamp if needed
float fBm( vec2 P, float octaves )
{
    float result = 0.0;
    float scale = 1.0;
    float n = 0.0;

    // unrolled 5-step octave loop
    n += snoise(P * scale * 10.0) / scale;
    if(octaves == 1.0) return n;
    scale *= 2.0;
    n += snoise(P * scale * 10.0) / scale;
    if(octaves == 2.0) return n;
    scale *= 2.0;
    n += snoise(P * scale * 10.0) / scale;
    if(octaves == 3.0) return n;
    scale *= 2.0;
    n += snoise(P * scale * 10.0) / scale;
    if(octaves == 4.0) return n;
    scale *= 2.0;
    n += snoise(P * scale * 10.0) / scale;
    return n;
}


float clampedfBm(vec2 P, float octaves)
{
    return(clamp(fBm(P, octaves), -1.0, 1.0));
} 


float clampedfBmZeroToOne(vec2 P, float octaves)
{
    float n = clamp(fBm(P, octaves), -1.0, 1.0); // -1...+1
    float v = (n * 0.5) + 0.5; // convert to 0...1 range
    return(v);
} 





float SampleAmplitude( sampler2D Map, vec2 MapCoord )
{
    return( texture2D( Map, MapCoord ).r );
}

// convert two slope directions into a normal
vec3 SlopeToNormal(vec2 slope)
{
    if( slope.x == 0.0 && slope.y == 0.0 )
        return( vec3(0.0, 0.0, 1.0) ); // prevent abnormal results
        
    vec2 angle = atan(slope); // Convert to angles with atan
    vec2 dispUV = sin(angle); // Convert to displacements
    float angle_h = atan(dispUV.y, dispUV.x); // horizontal angle in tangent/bitangent plane
    dispUV *= abs(vec2(cos(angle_h), sin(angle_h))); // circularize
    return vec3(dispUV.x, dispUV.y, 1.0 - sqrt(max(dispUV.x * dispUV.x + dispUV.y * dispUV.y, 0.0)));
}

// convert one point with two axis-offset points to a slope and thence a normal
vec3 SamplesToNormal( float BaseAmplitude, float DeltaAmplitudeX,
        float DeltaAmplitudeY, float Delta, float scale )
{
    vec2  DeltaAmplitude = BaseAmplitude - vec2(DeltaAmplitudeX, DeltaAmplitudeY);
    vec2 slope   = DeltaAmplitude * (1.0 / Delta);  // slope of amplitude fn at this point
    return(SlopeToNormal(slope * scale));
}


vec3 MapToNormal( sampler2D Map, vec2 MapCoord, float scale, vec2 localDFD )
{
    float BaseAmplitude = SampleAmplitude(Map, MapCoord);  // amplitude at txcoord point

    vec2  DeltaAmplitudeXY = 
        vec2(SampleAmplitude(Map, MapCoord + vec2(localDFD.x, 0.0)),
            SampleAmplitude(Map, MapCoord + vec2(0.0, localDFD.y)));
    float Delta = (localDFD.x + localDFD.y) * .5; // average is better than nothing
    return(SamplesToNormal(BaseAmplitude, DeltaAmplitudeXY.x, DeltaAmplitudeXY.y, Delta, scale));
}


vec3 fBMToNormal( vec2 MapCoord, float octaves, float scale, vec2 localDFD )
{
    float BaseAmplitude = clampedfBmZeroToOne(MapCoord, octaves); // amplitude at txcoord point

    vec2  DeltaAmplitudeXY = 
        vec2( clampedfBmZeroToOne( MapCoord + vec2( localDFD.x, 0.0 ), octaves ),
            clampedfBmZeroToOne( MapCoord + vec2( 0.0, localDFD.y ), octaves ) );
    float Delta = (localDFD.x + localDFD.y) * .5; // average is better than nothing
    return(SamplesToNormal(BaseAmplitude, DeltaAmplitudeXY.x, DeltaAmplitudeXY.y, Delta, scale));
}


// END gl2/noise-noise.vs.fs

