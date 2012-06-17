uniform mat4 Transform;

attribute vec4 Position;
attribute vec2 TexCoords0;

varying lowp vec4 PrimColor;
varying mediump vec2 UVBase;

void main()
{
    gl_Position = Transform * Position;
    PrimColor = vec4(1.0,1.0,1.0,1.0);
    UVBase = TexCoords0;
}
