#include "stdafx.h"
#include "SimpleMath.h"
#include "BlurEffect.h"
#include <wrl.h>
#include "ConstantBuffer.h"

#include <algorithm>

using namespace DirectX;
struct ShaderBytecode
{
    void const* code;
    size_t length;
};

struct BlurEffectConstants
{
	float blurAmount;
    XMFLOAT2 BlurOffsetBase;
	float BloomIntensity;
	float BaseIntensity;
	float BloomSaturation;
	float BaseSaturation;
	float BloomThreshold;
	XMMATRIX WorldViewProj;
	XMFLOAT4 blurWeights[15];
	XMFLOAT4 blurOffsets[15];
};


namespace 
{
	#include "VSShader.inc"
	#include "PixelShader.inc"
	#include "BloomCombine.inc"
	#include "BloomExtract.inc"
}

const ShaderBytecode BlurEffectShaderCode[] =
{
	{ g_VSBasic, sizeof(g_VSBasic)},
	{ g_GaussianBlurPS, sizeof(g_GaussianBlurPS)},
	{ g_BloomCombine, sizeof(g_BloomCombine)},
	{ g_BloomExtract, sizeof(g_BloomExtract)}
};

class BlurEffect::Impl : public DirectX::AlignedNew<BlurEffect::Impl>
{
public:
    Impl(_In_ ID3D11Device* device); 
	virtual ~Impl();
	BlurEffectConstants constants;
    void Apply(_In_ ID3D11DeviceContext* deviceContext, int pass);
	void SetBufferMatrix(CXMMATRIX matrix);
	void SetTexture(ID3D11ShaderResourceView* externalTexture, ID3D11ShaderResourceView* ext2 = nullptr);
	void SetData(float blurAmount, XMFLOAT2 delta, float BaseIntensity,	float BloomSaturation,	float BaseSaturation,	float BloomThreshold);
	void CalculateGaussBlur();
	Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
	ConstantBuffer<BlurEffectConstants> mBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vss;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pss[3];
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> gss;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture2;

};

BlurEffect::Impl::~Impl()
{
	
}

void BlurEffect::Impl::Apply(_In_ ID3D11DeviceContext* device, int pass)
{

	mBuffer.SetData(device, constants);

	ID3D11ShaderResourceView* textures[2] = { texture.Get(), texture2.Get() };

	if (texture2.Get() != nullptr)
	{
		device->PSSetShaderResources(0, 2, textures);
		ID3D11SamplerState* defSampler;
		device->PSGetSamplers(0, 1, &defSampler);
		device->PSSetSamplers(1,1, &defSampler);
	}
	else
	{
		device->PSSetShaderResources(0, 1, textures);
	}


	 // Set shaders.
	if (vss.Get() == nullptr)
	{
		HRESULT hr = mDevice->CreateVertexShader(BlurEffectShaderCode[0].code, BlurEffectShaderCode[0].length, nullptr, vss.GetAddressOf());
		if (FAILED(hr))
		{
			vss.Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "BlurVS";
			vss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
	if (pss[0].Get() == nullptr)
	{
		
		HRESULT hr = mDevice->CreatePixelShader(BlurEffectShaderCode[1].code, BlurEffectShaderCode[1].length, nullptr, pss[0].GetAddressOf());
		if (FAILED(hr))
		{
			pss[0].Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "GaussBlurPSCode";
			vss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
     
	if (pss[1].Get() == nullptr)
	{
		
		HRESULT hr = mDevice->CreatePixelShader(BlurEffectShaderCode[2].code, BlurEffectShaderCode[2].length, nullptr, pss[1].GetAddressOf());
		if (FAILED(hr))
		{
			pss[1].Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "BlurCombinePS";
			vss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
	if (pss[2].Get() == nullptr)
	{
		
		HRESULT hr = mDevice->CreatePixelShader(BlurEffectShaderCode[3].code, BlurEffectShaderCode[3].length, nullptr, pss[2].GetAddressOf());
		if (FAILED(hr))
		{
			pss[2].Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "BlurExtractPS";
			vss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
	device->VSSetShader(vss.Get(), nullptr, 0);
	device->PSSetShader(pss[pass].Get(), nullptr, 0);
	mBuffer.SetData(device, constants);

	ID3D11Buffer* buffer = mBuffer.GetBuffer();

    device->VSSetConstantBuffers(0, 1, &buffer);
    device->PSSetConstantBuffers(0, 1, &buffer);
}

BlurEffect::Impl::Impl(_In_ ID3D11Device* device) : mDevice(device), mBuffer(device)
{
	ZeroMemory(&constants, sizeof(constants));
	constants.WorldViewProj = XMMatrixIdentity();
}

void BlurEffect::Impl::SetBufferMatrix(CXMMATRIX result)
{
	constants.WorldViewProj = result;
}

void BlurEffect::Impl::SetData(float blurAmount, XMFLOAT2 delta,float BaseIntensity,
	float BloomSaturation,
	float BaseSaturation,
	float BloomThreshold)
{
	constants.blurAmount = blurAmount;
	constants.BloomIntensity = BaseIntensity;
	constants.BaseIntensity = BaseIntensity;
	constants.BaseSaturation = BaseSaturation;
	constants.BloomSaturation = BloomSaturation;
	constants.BloomThreshold = BloomThreshold;
	constants.BlurOffsetBase = delta;
	CalculateGaussBlur();
}

void BlurEffect::Impl::CalculateGaussBlur()
{
	float halfBlur = constants.blurAmount * 0.5f;
	float deviation = constants.blurAmount * 0.35f;
	
	int i = 0;
	

	std::for_each(std::begin(constants.blurWeights), std::end(constants.blurWeights), [&](XMFLOAT4& vecElem)
	{
		double x = ++i;
		if (i % 2)
		{
			vecElem.x = (1.0f / sqrtf(2.0f * XM_PI * deviation)) * exp(-((x * x) / (2.0 * deviation)));  
		}
		else
		{
			x = i - 1;
			vecElem.x = (1.0f / sqrtf(2.0f * XM_PI * deviation)) * exp(-((x * x) / (2.0 * deviation)));
		}
	});

	i =0;

	//	offsets[0] = float2(0.0f, 0.0f);
	//weights[0] = Gaussian(0, deviation);
	//totalWeight = weights[0];
	//for (int i = 0; i < 7; i++)
	//{
	//	weight = Gaussian( i + 1, deviation);
	//	weights[i * 2 + 1] = weight;
	//	weights[i * 2 + 2] = weight;
	//	totalWeight += weight * 2;
	//	sampleOffset = i * 2 + 1.5f;
	//	delta = offsetBase * sampleOffset;
	//	offsets[i+1] = delta;
	//	offsets[i+2] = -delta;
	//}


	//	sampleOffset = i * 2 + 1.5f;
	//	delta = offsetBase * sampleOffset;
	//	offsets[i+1] = delta;
	//	offsets[i+2] = -delta;

	std::for_each(std::begin(constants.blurOffsets), std::end(constants.blurOffsets), [&](XMFLOAT4& float2Elem) {
		XMVECTOR delta = XMLoadFloat2(&constants.BlurOffsetBase);
		XMFLOAT4 valuset;
		if (i % 2 == 0)
		{
			XMVECTOR offsetdelta  = XMVectorScale(delta, (i * 2 + 1.5f));
			XMStoreFloat4(&valuset, offsetdelta);
		}
		else
		{
			XMVECTOR offsetdelta  = XMVectorScale(delta, ((i - 1) * 2 + 1.5f));
			XMStoreFloat4(&valuset, -offsetdelta);
		}
		float2Elem = valuset;
		i++;
	});
	constants.blurOffsets[0] = XMFLOAT4(0.0f, 0.0f, 0.f,0.0f);


}

void BlurEffect::SetData(float blurAmount, XMFLOAT2 delta, float BaseIntensity,	float BloomSaturation,	float BaseSaturation,	float BloomThreshold)
{
	pImpl->SetData(blurAmount, delta, BaseIntensity, BloomSaturation,BaseSaturation, BloomThreshold);
}

void BlurEffect::SetTexture(_In_ ID3D11ShaderResourceView* texture, _In_ ID3D11ShaderResourceView* texture2)
{
	pImpl->SetTexture(texture, texture2);
}

void BlurEffect::Impl::SetTexture(ID3D11ShaderResourceView* externalTexture, ID3D11ShaderResourceView* externalTexture2)
{
	texture = externalTexture;
	texture2 = externalTexture2;
}

BlurEffect::BlurEffect(_In_ ID3D11Device* device) : pImpl(new Impl(device))
{
	lworld = lview = lproject = XMMatrixIdentity();
}


BlurEffect::~BlurEffect(void)
{
}

// Move constructor.
BlurEffect::BlurEffect(BlurEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
	lworld = lview = lproject = XMMatrixIdentity();
}


// Move assignment.
BlurEffect& BlurEffect::operator= (BlurEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}

void BlurEffect::Apply(_In_ ID3D11DeviceContext* deviceContext) 
{
	pImpl->Apply(deviceContext, (int)currentPS);
}

void BlurEffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
	*pShaderByteCode = BlurEffectShaderCode[0].code;
	*pByteCodeLength = BlurEffectShaderCode[0].length;
}

void BlurEffect::SetWorld(CXMMATRIX value)
{
	lworld= value;
}
void BlurEffect::SetView(CXMMATRIX value)
{
	lview = value;
}
void BlurEffect::SetProjection(CXMMATRIX value)
{
	lproject = value;
}


void BlurEffect::SetMatrices( CXMMATRIX world,  CXMMATRIX view,  CXMMATRIX proj)
{
	if (!XMMatrixIsIdentity(lproject))
	{
		SimpleMath::Matrix wvp(world);
		SimpleMath::Matrix v(view);
		wvp *= v;
		wvp *= SimpleMath::Matrix(lproject);
		pImpl->SetBufferMatrix( wvp);
	}
	else
	{
		pImpl->SetBufferMatrix(world * view * proj);
	}

}

float BlurEffect::ComputeGauss(float factor, int BlurAmount)
{
	float theta = (float)BlurAmount;

	return (float)((1.0 / sqrtf(2 * XM_PI * theta)) *
		expf(-(factor * factor) / (2 * theta * theta)));
}
