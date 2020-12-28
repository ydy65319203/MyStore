#version 330
in vec2 vec2TexCoord;
//out vec4 vec4PixelColor;
uniform sampler2D  texSampler2D_Y;
uniform sampler2D  texSampler2D_U;
uniform sampler2D  texSampler2D_V;

void main()
{
    //vec4PixelColor = vec4(vec2TexCoord.x, vec2TexCoord.y, 0.0f, 1.0f);
    //gl_FragColor = texture2D(texSampler2D_Y, vec2TexCoord);

    vec3 yuv;
    vec3 rgb;
    yuv.x = texture2D(texSampler2D_Y, vec2TexCoord).r;
    yuv.y = texture2D(texSampler2D_U, vec2TexCoord).r - 0.5;
    yuv.z = texture2D(texSampler2D_V, vec2TexCoord).r - 0.5;
    rgb = mat3( 1,       1,         1,
                0,       -0.39465,  2.03211,
                1.13983, -0.58060,  0) * yuv;
    gl_FragColor = vec4(rgb, 1);
}
