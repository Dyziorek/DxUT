#include "stdafx.h"
#include "TextureElement.h"


using namespace DirectX;

TextureElement::TextureElement(void)
{
}


TextureElement::~TextureElement(void)
{
}


HRESULT TextureElement::Initialize(ID3D11Device* device, int width, int height, float screenNear, float screenDepth, bool stencilEnabled)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT result;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	// Store the width and height of the render texture.
	textureWidth = width;
	textureHeight = height;

	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

	// Create the render target texture.
	result = device->CreateTexture2D(&textureDesc, NULL, &renderTargetTexture);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Setup the description of the render target view.
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	result = device->CreateRenderTargetView(renderTargetTexture.Get(), &renderTargetViewDesc, &textureTargetView);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	result = device->CreateShaderResourceView(renderTargetTexture.Get(), &shaderResourceViewDesc, &shaderResourceView);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = textureWidth;
	depthBufferDesc.Height = textureHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = device->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilViewDesc, &depthStencilView);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	
	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	
	if (stencilEnabled)
	{
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	}

	result = device->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf());
	if(FAILED(result))
	{
		return E_FAIL;
	}

	depthStencilDesc.StencilEnable = false;
	result = device->CreateDepthStencilState(&depthStencilDesc, offDepthStencilState.GetAddressOf());
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Setup the viewport for rendering.
    viewport.Width = (float)textureWidth;
    viewport.Height = (float)textureHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

	// Setup the projection matrix.
	projectionMatrix = XMMatrixPerspectiveFovRH(((float)XM_PI / 4.0f), ((float)textureWidth / (float)textureHeight), screenNear, screenDepth);

	// Create an orthographic projection matrix for 2D rendering.
	orthoMatrix = XMMatrixOrthographicRH( (float)textureWidth, (float)textureHeight, screenNear, screenDepth);

	pdevice = device;

	return S_OK;
}

HRESULT TextureElement::BeginRenderTexture(ID3D11DeviceContext* deviceCtx, DirectX::IEffect *pShader, DirectX::IEffectMatrices* matrices)
{

	UINT numView = 1;
	deviceCtx->OMGetRenderTargets(1, &oldTargetView, &oldStencil);
	deviceCtx->RSGetViewports(&numView, &oldViewPort);
	deviceCtx->OMGetDepthStencilState(&olddepthStencilState, &numView);
	//matrices->SetWorld(txworld);
	//matrices->SetView(txview);
	//matrices->SetProjection(orthoMatrix);
	deviceCtx->OMSetRenderTargets(1, textureTargetView.GetAddressOf(), depthStencilView.Get());
	deviceCtx->OMSetDepthStencilState(depthStencilState.Get(), 1);
	deviceCtx->RSSetViewports(1, &viewport);

	deviceCtx->ClearRenderTargetView(textureTargetView.Get(), DirectX::Colors::SkyBlue);
	deviceCtx->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	

	return S_OK;
}

HRESULT TextureElement::ApplyShaders(ID3D11DeviceContext* deviceCtx, DirectX::IEffect *pShader, DirectX::IEffectMatrices* matrices)
{
	void const* pShaderBytes;
	size_t Count;

	pShader->GetVertexShaderBytecode(&pShaderBytes, &Count);

	pdevice->CreateInputLayout(VertexPositionTexture::InputElements,
			VertexPositionTexture::InputElementCount,
			pShaderBytes, Count,
			Layout.GetAddressOf());

	deviceCtx->IASetInputLayout(Layout.Get());

	pShader->Apply(deviceCtx);

	return S_OK;
}

HRESULT TextureElement::EndRenderTexture(ID3D11DeviceContext* deviceCtx)
{
	Layout.Reset();
	deviceCtx->OMSetRenderTargets(1, &oldTargetView, oldStencil.Get());
	deviceCtx->RSSetViewports(1, &oldViewPort);
	deviceCtx->OMSetDepthStencilState(olddepthStencilState.Get(), 1);
	return S_OK;
}

HRESULT TextureElement::RenderTexture(ID3D11DeviceContext* deviceCtx, IEffect *pShader, IEffectMatrices* matrices, GeometricPrimitive* pmesh)
{
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> oldTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> oldStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> oldStencilState;
	D3D11_VIEWPORT oldViewPort;
	UINT numView = 1;
	UINT depRef = 1;
	deviceCtx->OMGetRenderTargets(1, &oldTargetView, &oldStencil);
	deviceCtx->RSGetViewports(&numView, &oldViewPort);
	deviceCtx->OMGetDepthStencilState(oldStencilState.GetAddressOf(), &depRef);
	//matrices->SetWorld(txworld);
	//matrices->SetView(txview);
	//matrices->SetProjection(orthoMatrix);
	deviceCtx->OMSetRenderTargets(1, textureTargetView.GetAddressOf(), depthStencilView.Get());
	deviceCtx->OMSetDepthStencilState(depthStencilState.Get(), 1);
	deviceCtx->RSSetViewports(1, &viewport);

	deviceCtx->ClearRenderTargetView(textureTargetView.Get(), DirectX::Colors::SkyBlue);
	deviceCtx->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> Layout;
	pmesh->CreateInputLayout(pShader, &Layout);
	pmesh->Draw(pShader, Layout.Get() );
	
	deviceCtx->OMSetRenderTargets(1, &oldTargetView, oldStencil.Get());
	deviceCtx->OMSetDepthStencilState(oldStencilState.Get(), depRef);
	deviceCtx->RSSetViewports(1, &oldViewPort);

	return S_OK;
}

HRESULT TextureElement::SetMatrices(DirectX::CXMMATRIX w, DirectX::CXMMATRIX v, DirectX::CXMMATRIX p, DirectX::CXMMATRIX orth)
{
	txworld = w;
	txview = v;
	if (!XMMatrixIsIdentity(orth))
	{
		orthoMatrix = orth;
	}
	projectionMatrix = p;
	return S_OK;

}

HRESULT TextureElement::SetPrivateName(const char* name)
{
	SetDebugObjectName(renderTargetTexture.Get(), name);
	SetDebugObjectName(textureTargetView.Get(), name);
	SetDebugObjectName(shaderResourceView.Get(), name);
	SetDebugObjectName(depthStencilBuffer.Get(), name);
	SetDebugObjectName(depthStencilView.Get(), name);
	return S_OK;
}


HRESULT TextureElement::GetTextureData(D3D11_TEXTURE2D_DESC* pData, ID3D11Texture2D** textureWork)
{
	if (pData == nullptr)
		return E_FAIL;
	renderTargetTexture->GetDesc(pData);
	*textureWork = renderTargetTexture.Get();
	return S_OK;
}

Microsoft::WRL::ComPtr<ID3D11Texture2D>& TextureElement::GetTexture()
{
	return renderTargetTexture;
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& TextureElement::GetTargetView()
{
	return textureTargetView;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& TextureElement::GetShaderResourceView()
{
	return shaderResourceView;
}

HRESULT TextureElement::SetShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& source)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	ID3D11Texture2D* pRes;
	source.Get()->GetResource((ID3D11Resource**)&pRes);
	pRes->GetDesc(&textureDesc);
	textureHeight = textureDesc.Height;
	textureWidth = textureDesc.Width;

	shaderResourceView.Swap(source);
	return S_OK;
}