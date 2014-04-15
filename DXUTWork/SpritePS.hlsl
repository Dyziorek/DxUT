
Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

cbuffer Parameters : register(b0)
{
	float4 keyColor;
	float4 pixelUnit;
	float  circle;
}


float4 main(float4 color : COLOR0,  float2 texCoord : TEXCOORD0) : SV_TARGET
{
	 float4 colorTex = Texture.Sample(TextureSampler, texCoord);
	 colorTex -= keyColor;



	 //float resultMulX2 = frac(texCoord.x* 16);
	 //float resultMulY2 = frac(texCoord.y* 16);

	 if (circle.x > 0)
	 {
	 	 float resultFloorX = floor(texCoord.x* pixelUnit.x* pixelUnit.z);
		 float resultFloorY = floor(texCoord.y* pixelUnit.y* pixelUnit.w);
		 float2 center =  float2( resultFloorX/(pixelUnit.x* pixelUnit.z) + 0.5/(pixelUnit.x* pixelUnit.z), resultFloorY/(pixelUnit.y* pixelUnit.w)+ 0.5/(pixelUnit.y* pixelUnit.w));

	     float distanced = distance(center, texCoord);
		 if ( (distanced - circle) > -0.0015)
		 {
			 return float4(0.0,0.0,0.0,0.0);
		 }

		 //if ((resultMulX2 < 0.02 || resultMulX2 > 0.98) && (resultMulY2 < 0.02 || resultMulY2 > 0.98))
		 //{
			// if ( resultMulX < 0.02 || resultMulY < 0.02 || resultMulX > 0.98 || resultMulY > 0.98)
			// {
			//	 //return float4(0.9,0.1,0.9,1);
			// }
			// else
			// {
			//	 return float4(0.6,0.6,0.6,1);
			// }
		 //}
	 }
	 else
	 {
		 float resultMulX = frac(texCoord.x* pixelUnit.x * pixelUnit.z);
		 float resultMulY = frac(texCoord.y* pixelUnit.y * pixelUnit.w);

		 if ( resultMulX < 0.01 || resultMulY < 0.01 || resultMulX > 0.99 || resultMulY > 0.99)
		 {
			 return float4(0.3,1,0.3,1);
		 }
	 }
	 /*if (colorTex.x < 1.98 && colorTex.y < 1.98 && colorTex.z < 1.98)
	 {
		 return float4(0,0,0,0);
	 }*/

	 
	 return Texture.Sample(TextureSampler, texCoord) * color;
}