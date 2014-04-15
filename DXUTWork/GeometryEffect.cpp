#include "stdafx.h"
#include "SimpleMath.h"
#include "GeometryEffect.h"
#include <wrl.h>
#include "ConstantBuffer.h"
#include "CommonStates.h"
#include "AlignedNew.h"

#include <algorithm>

// Vertex struct holding position, normal vector, and texture mapping information.
const D3D11_INPUT_ELEMENT_DESC VertexPositionNormalTextureStride::InputElements[] =
{
    { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};


using namespace DirectX;
struct ShaderBytecode
{
    void const* code;
    size_t length;
};

struct GeometryConstants : AlignedNew<GeometryConstants>
{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Proj;
	XMMATRIX WVP;
	float fEdgeLength;
};

namespace 
{
	#include "EdgeShader.inc"
	#include "EdgeShaderPS.inc"
	#include "EdgeShaderVS.inc"
}

const ShaderBytecode GeometryEffectShaderCode[] =
{
	{ g_EdgeShader, sizeof(g_EdgeShader)},
	{ g_EdgeShaderPS, sizeof(g_EdgeShaderPS)},
	{ g_EdgeShaderVS, sizeof(g_EdgeShaderVS)}
};

class GeometryEffect::Impl : public AlignedNew<GeometryConstants>
{
public:
    Impl(_In_ ID3D11Device* device); 
	virtual ~Impl();
	GeometryConstants constants;
	void SetWorld(CXMMATRIX value);
    void SetView(CXMMATRIX value);
    void SetProjection(CXMMATRIX value);
	void DisableGeometry(bool bEnable);
    void Apply(_In_ ID3D11DeviceContext* deviceContext);
	void SetTexture(ID3D11ShaderResourceView* externalTexture);
	Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
	ConstantBuffer<GeometryConstants> mGeometryBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vss;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pss;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> gss;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
   std::unique_ptr<CommonStates> stateObjects;
   bool geometryActive;
};

GeometryEffect::Impl::~Impl()
{
	
}

void GeometryEffect::Impl::Apply(_In_ ID3D11DeviceContext* device)
{

	XMMATRIX wvp = XMMatrixMultiply(constants.World, constants.View);
	wvp = XMMatrixMultiplyTranspose(wvp, constants.Proj);
	constants.WVP = wvp;

	mGeometryBuffer.SetData(device, constants);

	ID3D11ShaderResourceView* textures[1] = { texture.Get() };

	device->PSSetShaderResources(0, 1, textures);


	 // Set shaders.
	if (vss.Get() == nullptr)
	{
		HRESULT hr = mDevice->CreateVertexShader(GeometryEffectShaderCode[2].code, GeometryEffectShaderCode[2].length, nullptr, vss.GetAddressOf());
		if (FAILED(hr))
		{
			vss.Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "EdgeShaderVS";
			vss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
	if (gss.Get() == nullptr)
	{
		HRESULT hr = mDevice->CreateGeometryShader(GeometryEffectShaderCode[0].code, GeometryEffectShaderCode[0].length, nullptr, gss.GetAddressOf());
		if (FAILED(hr))
		{
			gss.Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "EdgeShaderGS";
			gss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
	if (pss.Get() == nullptr)
	{
		
		HRESULT hr = mDevice->CreatePixelShader(GeometryEffectShaderCode[1].code, GeometryEffectShaderCode[1].length, nullptr, pss.GetAddressOf());
		if (FAILED(hr))
		{
			pss.Detach();
		}
		else
		{
#ifdef _DEBUG
			const char* blurInfo = "EdgeShaderPS";
			pss->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(blurInfo), blurInfo);
#endif
		}
	}
	
	if (geometryActive)
	{
		device->GSSetShader(gss.Get(), nullptr, 0);
	}
	else
	{
		device->GSSetShader(nullptr, nullptr, 0);
	}
	device->VSSetShader(vss.Get(), nullptr, 0);
	device->PSSetShader(pss.Get(), nullptr, 0);
	mGeometryBuffer.SetData(device, constants);

	ID3D11Buffer* buffer = mGeometryBuffer.GetBuffer();

	device->GSSetConstantBuffers(0, 1, &buffer);
    device->VSSetConstantBuffers(0, 1, &buffer);
    device->PSSetConstantBuffers(0, 1, &buffer);
	
	device->RSSetState(stateObjects->CullCounterClockwise());

	ID3D11SamplerState* samplerState = stateObjects->LinearClamp();
	device->PSSetSamplers(0, 1, &samplerState);
}

GeometryEffect::Impl::Impl(_In_ ID3D11Device* device) : mDevice(device), mGeometryBuffer(device)
{
	ZeroMemory(&constants, sizeof(constants));
	constants.World = XMMatrixIdentity();
	constants.View = XMMatrixIdentity();
	constants.Proj = XMMatrixIdentity();
	constants.WVP =  XMMatrixIdentity();
	constants.fEdgeLength = 0.01f;
	geometryActive = false;
	stateObjects.reset(new CommonStates(device));
}



void GeometryEffect::SetTexture(_In_ ID3D11ShaderResourceView* texture)
{
	pImpl->SetTexture(texture);
}

void GeometryEffect::Impl::SetTexture(ID3D11ShaderResourceView* externalTexture)
{
	texture = externalTexture;
}

GeometryEffect::GeometryEffect(_In_ ID3D11Device* device) : pImpl(new Impl(device))
{
	lworld = lview = lproject = XMMatrixIdentity();
}


GeometryEffect::~GeometryEffect(void)
{
}

// Move constructor.
GeometryEffect::GeometryEffect(GeometryEffect&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
	lworld = lview = lproject = XMMatrixIdentity();
}


// Move assignment.
GeometryEffect& GeometryEffect::operator= (GeometryEffect&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}

void GeometryEffect::Apply(_In_ ID3D11DeviceContext* deviceContext) 
{
	pImpl->Apply(deviceContext);
}

void GeometryEffect::GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength)
{
	*pShaderByteCode = GeometryEffectShaderCode[2].code;
	*pByteCodeLength = GeometryEffectShaderCode[2].length;
}

void GeometryEffect::SetWorld(CXMMATRIX value)
{
	pImpl->SetWorld(value);
}
void GeometryEffect::SetView(CXMMATRIX value)
{
	pImpl->SetView(value);
}
void GeometryEffect::SetProjection(CXMMATRIX value)
{
	pImpl->SetProjection(value);
}


void GeometryEffect::SetMatrices( CXMMATRIX world,  CXMMATRIX view,  CXMMATRIX proj)
{
	// if (!XMMatrixIsIdentity(lproject))
	// {
		// SimpleMath::Matrix wvp(world);
		// SimpleMath::Matrix v(view);
		// wvp *= v;
		// wvp *= SimpleMath::Matrix(lproject);
		// pImpl->SetBufferMatrix( wvp);
	// }
	// else
	// {
		// pImpl->SetBufferMatrix(world * view * proj);
	// }

	pImpl->SetWorld(world);
	pImpl->SetView(view);
	pImpl->SetProjection(proj);
}

void GeometryEffect::Impl::SetWorld(CXMMATRIX value)
{
	constants.World = XMMatrixTranspose(value);
}
void GeometryEffect::Impl::SetView(CXMMATRIX value)
{
	constants.View = XMMatrixTranspose(value);
}
void GeometryEffect::Impl::SetProjection(CXMMATRIX value)
{
	
	constants.Proj = XMMatrixTranspose(value);
}

void GeometryEffect::Impl::DisableGeometry(bool bEnable)
{
	geometryActive = bEnable;

}

void GeometryEffect::DisableGeometry(bool bEnable)
{
	pImpl->DisableGeometry(!bEnable);
}