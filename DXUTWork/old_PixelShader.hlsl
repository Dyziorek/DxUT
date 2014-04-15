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
};


struct VSInput
{
    float4 Position : SV_Position;
};

struct VSOutput
{
    float4 PositionPS : SV_Position;
};

VSOutput VSBasic(VSInput vin)
{
    VSOutput vout;

	vout.PositionPS = mul(vin.Position, WorldViewProj);

    return vout;
}


float Gaussian (float x, float deviation)
{
    return (1.0 / sqrt(2.0 * 3.141592 * deviation)) * exp(-((x * x) / (2.0 * deviation)));  
}

float4 GaussianBlurPS(float2 texCoord : TEXCOORD0) : SV_TARGET
{
    float4 c = 0;
    float halfBlur = blurAmount * 0.5f;
	float deviation = blurAmount * 0.35f;
	float2 delta;
	float2 offsets[15];
	float weights[15];
	float sampleOffset = 0;
	float totalWeight = 0;
	float weight = 0;
	// texColour = texture2D(Sample0, vUv + vec2(offset * TexelSize.x * BlurScale, 0.0)) * Gaussian(offset * strength, deviation);
    // Combine a number of weighted image filter taps.
	offsets[0] = float2(0.0f, 0.0f);
	weights[0] = Gaussian(0, deviation);
	totalWeight = weights[0];
	for (int i = 0; i < 7; i++)
	{
		weight = Gaussian( i + 1, deviation);
		weights[i * 2 + 1] = weight;
		weights[i * 2 + 2] = weight;
		totalWeight += weight * 2;
		sampleOffset = i * 2 + 1.5f;
		delta = offsetBase * sampleOffset;
		offsets[i+1] = delta;
		offsets[i+2] = -delta;
	}

	for (i = 0; i < 15; i++)
	{
		float offset = float(i) - halfBlur;
		c += Texture.Sample(TextureSampler, texCoord + offsets[i]) * weights[i];
	}
    /*c += tex2D(TextureSampler, texCoord + SampleOffsets[0]) * SampleWeights[0];
    c += tex2D(TextureSampler, texCoord + SampleOffsets[1]) * SampleWeights[1];
    c += tex2D(TextureSampler, texCoord + SampleOffsets[2]) * SampleWeights[2];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[3]) * SampleWeights[3];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[4]) * SampleWeights[4];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[5]) * SampleWeights[5];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[6]) * SampleWeights[6];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[7]) * SampleWeights[7];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[8]) * SampleWeights[8];
    c += tex2D(TextureSampler, texCoord + SampleOffsets[9]) * SampleWeights[9];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[10]) * SampleWeights[10];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[11]) * SampleWeights[11];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[12]) * SampleWeights[12];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[13]) * SampleWeights[13];
	c += tex2D(TextureSampler, texCoord + SampleOffsets[14]) * SampleWeights[14];*/
    return c;
}

