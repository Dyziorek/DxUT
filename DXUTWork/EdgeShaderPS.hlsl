

#include "ShaderCommon.fxh"


Texture2D    g_txDiffuse : register( t0 );
sampler TextureSampler : register(s0);




float4 EdgeShaderPS(PSSceneIn GSInput ) : SV_TARGET
{
	if (GSInput.EdgeFlag == SILHOUETTE_EDGE)
	{
		return  g_txDiffuse.Sample(TextureSampler, GSInput.Tex) * float4(0.1f,0.1f,0.1f,1.0f);
	}
	else
	{
		return g_txDiffuse.Sample(TextureSampler, GSInput.Tex);
	}
}