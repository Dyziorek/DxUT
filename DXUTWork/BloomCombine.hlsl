// Pixel shader combines the bloom image with the original
// scene, using tweakable intensity levels and saturation.
// This is the final step in applying a bloom postprocess.

Texture2D<float4> Texture : register(t0);
Texture2D<float4> Texture2 : register(t1);
sampler BloomSampler : register(s0);
sampler BaseSampler : register(s1);

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


// Helper for modifying the saturation of a color.
float4 AdjustSaturation(float4 color, float saturation)
{
    // The constants 0.3, 0.59, and 0.11 are chosen because the
    // human eye is more sensitive to green light, and less to blue.
    float grey = dot(color, float3(0.3, 0.59, 0.11));

    return lerp(grey, color, saturation);
}


float4 BloomCombine(float2 texCoord : TEXCOORD0) : SV_TARGET
{
    // Look up the bloom and original base image colors.
    float4 bloom = Texture.Sample(BloomSampler, texCoord);
    float4 base = Texture2.Sample(BaseSampler, texCoord);
    
    // Adjust color saturation and intensity.
    bloom = AdjustSaturation(bloom, BloomSaturation) * BloomIntensity;
    base = AdjustSaturation(base, BaseSaturation) * BaseIntensity;
    
    // Darken down the base image in areas where there is a lot of bloom,
    // to prevent things looking excessively burned-out.
    base *= (1 - saturate(bloom));
    
    // Combine the two images.
    return base + bloom;
}


