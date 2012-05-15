const char* psz_textured_ps = {
"	varying lowp vec4 PrimColor;\
    varying mediump vec2 UVBase; \
    uniform sampler2D TextureBase; \
	\
	void main()\
	{\
        lowp vec4 BaseColor = texture2D(TextureBase, UVBase); \
		gl_FragColor = BaseColor;\
	}\
"};
