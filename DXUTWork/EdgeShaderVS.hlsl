#include "ShaderCommon.fxh"

Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);



GSSceneIn EdgeShaderVS(VSSceneIn vin)
{
    GSSceneIn vout;
	vout.Pos = vin.Pos;
	//vout.Pos = mul(vin.Pos, World);
	//vout.Pos = mul(vout.Pos, View);
	//vout.Pos = mul(vout.Pos, Proj);
	vout.Norm = mul(float4(vin.Norm.xyz, 1), WVP);
	vout.Tex = vin.Tex;
	//vout.EdgeFlag = 0;
    return vout;
}



