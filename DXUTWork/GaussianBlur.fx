// Pixel shader applies a one dimensional gaussian blur filter.
// This is used twice by the bloom postprocess, first to
// blur horizontally, and then again to blur vertically.

sampler TextureSampler : register(s0);


cbuffer Parameters : register(b0)
{
    float2 SampleOffsets[15]  : packoffset(c0);
	float SampleWeights[15]   : packoffset(c15); 
};

float4 GaussianBlurPS(float2 texCoord : TEXCOORD0) : SV_Target0
{
    float4 c = 0;
    
    // Combine a number of weighted image filter taps.
    for (int i = 0; i < 15; i++)
    {
        c += tex2D(TextureSampler, texCoord + SampleOffsets[i]) * SampleWeights[i];
    }
    
    return c;
}
