

#include "ShaderCommon.fxh"

[maxvertexcount(36)]
void EdgeShader( triangleadj GSSceneIn input[6], inout TriangleStream<PSSceneIn> OutputStream )
{

	float4x4 MV = mul(World, View);

	// Regular Normal for Triangle against view
	float3 Edge1 = input[2].Pos - input[0].Pos;
	float3 Edge2 = input[4].Pos - input[0].Pos;
	float3 Normal = cross(Edge2, Edge1);
	Normal = mul(Normal, MV);
	Normal = normalize(Normal);

	float dotView = dot(Normal,input[0].Pos);

	if (dotView < 0.0)
	{
		// triangle is visible so it will be made (not visible simply discard)
		// walk through all adjacent around this triangle and find which is not visible
		for (int i = 0; i < 3; i++)
		{
			int inext = i+1%6;
			int inext2 = i+2%6;
			float3 AdjA1 = input[inext].Pos - input[i].Pos;
			float3 AdjA2 = input[inext2].Pos - input[i].Pos;
			float3 NormalAdj = cross(AdjA2, AdjA1);
			NormalAdj = mul(NormalAdj, MV);
			NormalAdj = normalize(NormalAdj);


			float dotProd = dot(NormalAdj,input[0].Pos);

			if (abs(dotProd) != 0.0)
			{
				PSSceneIn output;
				output.psPos = mul(input[0].Pos, World);
				output.psPos = mul(output.psPos, View);
				output.psPos = mul(output.psPos, Proj);
				output.wsNorm = float4(Normal, 1);
				output.Tex = input[0].Tex;
				output.EdgeFlag = REGULAR_EDGE;
				OutputStream.Append( output );

		
				output.psPos = mul(input[2].Pos, World);
				output.psPos = mul(output.psPos, View);
				output.psPos = mul(output.psPos, Proj);
				output.wsNorm = float4(Normal, 1);
				output.Tex = input[2].Tex;
				output.EdgeFlag = REGULAR_EDGE;
				OutputStream.Append( output );

		
				output.psPos = mul(input[4].Pos, World);
				output.psPos = mul(output.psPos, View);
				output.psPos = mul(output.psPos, Proj);
				output.wsNorm = float4(Normal, 1);
				output.Tex = input[4].Tex;
				output.EdgeFlag = REGULAR_EDGE;
				OutputStream.Append( output );


				OutputStream.RestartStrip();
			}

			//else
			//{
			//	PSSceneIn output;
			//	for(int v = 0; v < 2; v++)
			//	{
			//		float4 wsPos = float4(input[0].Pos.xyz,1) + (float4(input[0].Norm.xyz,0) * fEdgeLength * v);
			//		wsPos = mul(wsPos, World);
			//		float4 vsPos = mul(wsPos, View);
			//		output.psPos = mul(vsPos, Proj);
			//		output.wsNorm = input[0].Norm;
			//		output.EdgeFlag = SILHOUETTE_EDGE;
			//		output.Tex = input[0].Tex;
			//		OutputStream.Append(output);
			//	}
			//	for(int v1 = 0; v1 < 2; v++)
			//	{
			//		float4 wsPos = input[2].Pos +
			//		v1 * float4(input[2].Norm.xyz,0) * fEdgeLength;
			//		wsPos = mul(wsPos, World);
			//		float4 vsPos = mul(wsPos, View);
			//		output.psPos = mul(vsPos, Proj);
			//		output.wsNorm = input[2].Norm;
			//		output.EdgeFlag = SILHOUETTE_EDGE;
			//		output.Tex = input[2].Tex;
			//		OutputStream.Append(output);
			//	}
			//	OutputStream.RestartStrip();
		



			//	//PSSceneIn output;
			//	output.psPos = mul(input[0].Pos, World);
			//	output.psPos = mul(output.psPos, View);
			//	output.psPos = mul(output.psPos, Proj);
			//	output.wsNorm = input[0].Norm;
			//	output.Tex = float2(1-input[0].Tex.x,input[0].Tex.y);
			//	output.EdgeFlag = REGULAR_EDGE;
			//	OutputStream.Append( output );

		
			//	output.psPos = mul(input[2].Pos, World);
			//	output.psPos = mul(output.psPos, View);
			//	output.psPos = mul(output.psPos, Proj);
			//	output.wsNorm = input[2].Norm;
			//	output.Tex = float2(1-input[2].Tex.x,input[2].Tex.y);
			//	output.EdgeFlag = REGULAR_EDGE;
			//	OutputStream.Append( output );

		
			//	output.psPos = mul(input[4].Pos, World);
			//	output.psPos = mul(output.psPos, View);
			//	output.psPos = mul(output.psPos, Proj);
			//	output.wsNorm = input[4].Norm;
			//	output.Tex = float2(1-input[4].Tex.x,input[4].Tex.y);
			//	output.EdgeFlag = REGULAR_EDGE;
			//	OutputStream.Append( output );


			//	OutputStream.RestartStrip();
			//}

		}
	}

}

//[maxvertexcount(18)]
//void EdgeShader( triangleadj GSSceneIn input[6], inout TriangleStream<PSSceneIn> OutputStream )
//{	
//
//	float3 faceNormal = normalize(cross(
//		input[2].Pos - input[0].Pos,
//		input[4].Pos - input[0].Pos)
//	);
//
//	float4 viewDirection = -input[0].Pos;
//
//	float dotView = dot(faceNormal, viewDirection);
//
//   if(dotView < 0)
//   {
//	   // front-face triangle
//	   // now check adjecent triangle which shares edge on vertex 4 and 0
//	   for (int vAdj = 0; vAdj < 3; vAdj++)
//	   {
//			GSSceneIn vertA = input[0];
//			GSSceneIn vertB = input[4];
//			GSSceneIn vertC = input[5];
//			if (vAdj == 1)
//			{
//				vertA = input[0];
//				vertB = input[2];
//				vertC = input[1];
//			}
//			if (vAdj == 2)
//			{
//				vertA = input[4];
//				vertB = input[2];
//				vertC = input[3];
//			}
//
//			float3 wsAdjFaceNormal = normalize(cross(normalize(vertA.Pos.xyz - vertC.Pos.xyz),normalize(vertB.Pos.xyz-vertC.Pos.xyz)));
//			float4 wsView = mul(vertA.Pos, World);
//			wsView = mul(wsView, View);
//			float dotViewAdj = dot(wsAdjFaceNormal, wsView.xyz);
//
//			if(dotViewAdj >= 0.0)
//			{
//				for(int v = 0; v < 2; v++)
//				{
//					float4 wsPos = vertB.Pos + v * float4(vertB.Norm.xyz,0) * fEdgeLength;
//					float4 vsPos = mul(wsPos, View);
//					PSSceneIn output;
//					output.psPos = mul(vsPos, Proj);
//					output.wsNorm = vertB.Norm;
//					output.EdgeFlag = SILHOUETTE_EDGE;
//					output.Tex = float2(0,0);
//					OutputStream.Append(output);
//				}
//				for(int v1 = 0; v1 < 2; v++)
//				{
//					float4 wsPos = vertC.Pos + v1 * float4(vertC.Norm.xyz,0) * fEdgeLength;
//					float4 vsPos = mul(wsPos, View);
//					PSSceneIn output;
//					output.psPos = mul(vsPos, Proj);
//					output.wsNorm = vertC.Norm;
//					output.EdgeFlag = SILHOUETTE_EDGE;
//					output.Tex = float2(0,0);
//					OutputStream.Append(output);
//				}
//				OutputStream.RestartStrip();
//			}
//	   }
//	
//   }
//   else
//   {
//	for( uint i=0; i<6; i+=2 )
//	{
//		PSSceneIn output;
//		output.psPos = input[i].Pos;
//		output.wsNorm = input[i].Norm;
//		output.Tex = input[i].Tex;
//		output.EdgeFlag = REGULAR_EDGE;
//		OutputStream.Append( output );
//	}
//	OutputStream.RestartStrip();
//   }
//}
//
