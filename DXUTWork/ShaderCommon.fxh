
cbuffer Parameters : register(b0)
{
	matrix World;
	matrix View;
	matrix Proj;
	matrix WVP;
	float fEdgeLength;
};

struct VSSceneIn
{
	float4 Pos : SV_Position;
	float4 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};

struct GSSceneIn
{
	 float4 Pos : SV_Position;
	 float4 Norm : NORMAL;
	 float2 Tex : TEXCOORD0;
};

#define REGULAR_EDGE 0
#define SILHOUETTE_EDGE 1

struct PSSceneIn
{
	 float4 psPos : SV_Position;
	 float4 wsNorm : Normal;
	 float2 Tex : TEXCOORD0;
	 uint EdgeFlag :BLENDINDICES0;
};
