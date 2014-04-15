// Pixel shader applies a one dimensional gaussian blur filter.
// This is used twice by the bloom postprocess, first to
// blur horizontally, and then again to blur vertically.


Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);


cbuffer Parameters : register(b0)
{
	float blurAmount;
	float2 offsetBase;
	float BloomIntensity;
	float BaseIntensity;
	float BloomSaturation;
	float BaseSaturation;
	float BloomThreshold;
	float4x4 WorldViewProj;
	float weights[15];
	float2 offsets[15];
};


struct VSInput
{
	float2 TexCoord   : TEXCOORD0;
    float4 Position : SV_Position;
};

struct VSOutput
{
	float2 TexCoord   : TEXCOORD0;
    float4 PositionPS : SV_Position;
};

VSOutput VSBasic(VSInput vin)
{
    VSOutput vout;

	vout.PositionPS = mul(vin.Position, WorldViewProj);
	vout.TexCoord = vin.TexCoord;
    return vout;
}


