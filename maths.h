float Normalize(float afVout[3], float afVin[3]);
void MultMatrix(float psRes[4][4], float psSrcA[4][4], float psSrcB[4][4]);
void Persp(float pMatrix[4][4], float fovy, float aspect, float zNear, float zFar);
void Frustum(float pMatrix[4][4], float left, float right, float bottom, float top, float zNear, float zFar);
void Ortho(float m[4][4], float left, float right, float bottom,
			 float top, float zNear, float zFar);
void Scale(float pMatrix[4][4], float fX, float fY, float fZ);
void Translate(float pMatrix[4][4], float fX, float fY, float fZ);
void Rotate(float pMatrix[4][4], float fX, float fY, float fZ, float fAngle);
void Identity(float pMatrix[4][4]);
void InvertTransposeMatrix(float pDstMatrix[3][3], float pSrcMatrix[4][4]);
void LookAt(float pMatrix[4][4], float eyex, float eyey, float eyez, float centerx,
	  float centery, float centerz, float upx, float upy,
	  float upz);
void Cross(float v1[3], float v2[3], float result[3]);
