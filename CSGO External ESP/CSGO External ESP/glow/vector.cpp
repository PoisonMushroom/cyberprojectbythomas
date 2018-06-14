#include "stdafx.h"
#include "vector.h"
#include <math.h>
#include <windows.h>

//Prevents vector floating point errors
float NormalizeVectorValue(float vectorValue)
{
	if (!isfinite(vectorValue))
	{
		vectorValue = 0.0f;
	}
	return remainder(vectorValue, 360.0f);
}

Vector ClampVector(Vector &vector)
{
	vector.x = max(-89.0f, min(89.0f, NormalizeVectorValue(vector.x)));
	vector.y = NormalizeVectorValue(vector.y);
	vector.z = 0.f;
	return vector;
}
