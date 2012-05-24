const char* psz_textured_vs = {
"	uniform mat4 Transform;\
	\
	attribute vec4 Position;\
    attribute vec2 TexCoords0; \
    attribute vec3 Normal; \
	\
	varying lowp vec4 PrimColor;\
    varying mediump vec2 UVBase; \
	\
    uniform mediump float DISPLACEMENT_COEFFICIENT;\
    uniform sampler2D DispMap;\
    uniform float HighlightEnabled; \
	void main()\
	{\
        mediump float Displacement = texture2D(DispMap, TexCoords0).r * DISPLACEMENT_COEFFICIENT; \
        highp vec3 DisplacedPosition = Position.xyz + (Normal * Displacement); \
        gl_Position = Transform * vec4(DisplacedPosition, Position.w);\
        PrimColor = vec4(1.0,1.0,1.0,1.0) - ((HighlightEnabled*Displacement) * vec4(0, 1, 1, 0));\
        UVBase = TexCoords0; \
	}\
"};
