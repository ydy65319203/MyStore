#version 330
in vec2 vec2TexCoord;
//out vec4 vec4PixelColor;
uniform sampler2D texSampler2D;

void main()
{
    //vec4PixelColor = vec4(vec2TexCoord.x, vec2TexCoord.y, 0.0f, 1.0f);
    gl_FragColor = texture2D(texSampler2D, vec2TexCoord);
}
