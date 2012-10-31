// yuv42201_laplace.frag
//
// convert from YUV422 to RGB
//
// This code is in the public domain. If it breaks, you get
// to keep both pieces.

// current_texture_image - the texture that has the video image
//   initialized by the CPU program to contain the number of the 
//   texture unit that contains the video texture.
//   the data indicated by this is assumed to be in the form of
//   a luminance/alpha texture (two components per pixel). in the
//   shader we extract those components from each pixel and turn
//   them into RGB to draw the picture.

// texture_width - the width of the base texture for the window
//   (that is, not the size of the window or the size of the image
//   being textured onto it; but that power-of-two size texture width.)

// texel_width - the width of a texel in texture coordinates. 
//   the texture coordinates in the shader go from 0 to 1.0.
//   so each texel is (1.0 / texture_width) wide.

precision mediump float;
uniform sampler2D s_baseMap;
uniform float texture_width;
uniform float texel_width;
varying vec2 v_texCoord;

void main()
{
	float red, green, blue;
	vec4 luma_chroma;
	float luma, chroma_u,  chroma_v;
	float pixelx, pixely;
	float xcoord, ycoord;
	vec3 yuv;

	// note: pixelx, pixely are 0.0 to 1.0 so "next pixel horizontally"
	//  is not just pixelx + 1; rather pixelx + texel_width.
	pixelx = v_texCoord.s;
	pixely = v_texCoord.t;

	// if pixelx is even, then that pixel contains [Y U] and the 
	//    next one contains [Y V] -- and we want the V part.
	// if  pixelx is odd then that pixel contains [Y V] and the 
	//     previous one contains  [Y U] -- and we want the U part.

	// note: only valid for images whose width is an even number of
	// pixels

	xcoord = floor (pixelx * texture_width);

	luma_chroma = texture2D(s_baseMap, vec2(pixelx, pixely));

	// just look up the brightness
	luma = (luma_chroma.r - 0.0625) * 1.1643;

	if (0.0 == mod(xcoord , 2.0)) // even
	{
		chroma_u = luma_chroma.a;
		chroma_v = texture2D(s_baseMap, 
		vec2(pixelx + texel_width, pixely)).a;
	}
	else // odd
	{
		chroma_v = luma_chroma.a;
		chroma_u = texture2D(s_baseMap, 
		vec2(pixelx - texel_width, pixely)).a;     
	}
	chroma_u = chroma_u - 0.5;
	chroma_v = chroma_v - 0.5;

	red = luma + 1.5958 * chroma_v;
	green = luma - 0.39173 * chroma_u - 0.81290 * chroma_v;
	blue = luma + 2.017 * chroma_u;

	// set the color based on the texture color
    gl_FragColor = vec4(red, green, blue, 1.0);
    //gl_FragColor = vec4(luma, luma, luma, 1.0);
}
