// Pixel shader extracts the brighter areas of an image.
// This is the first step in applying a bloom postprocess.

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


float4 BloomExtract(float2 texCoord : TEXCOORD0) : SV_TARGET
{
    // Look up the original image color.
    float4 c = Texture.Sample(TextureSampler, texCoord);

    // Adjust it to keep only values brighter than the specified threshold.
    return saturate((c - BloomThreshold) / (1 - BloomThreshold));
}

