#pragma once
class TextureElement
{
public:
	TextureElement(void);
	virtual ~TextureElement(void);
	HRESULT Initialize(ID3D11Device* device, int width, int height, float screenNear, float screenDepth, bool enabledStencil);
	HRESULT SetPrivateName(const char* name);
	HRESULT RenderTexture(ID3D11DeviceContext* deviceCtx, DirectX::IEffect *pShader, DirectX::IEffectMatrices* matrices, DirectX::GeometricPrimitive* mesh);
	HRESULT BeginRenderTexture(ID3D11DeviceContext* deviceCtx, DirectX::IEffect *pShader, DirectX::IEffectMatrices* matrices);
	HRESULT EndRenderTexture(ID3D11DeviceContext* deviceCtx);
	HRESULT ApplyShaders(ID3D11DeviceContext* deviceCtx, DirectX::IEffect *pShader, DirectX::IEffectMatrices* matrices);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetShaderResourceView();
	HRESULT GetTextureData(D3D11_TEXTURE2D_DESC* pData, ID3D11Texture2D** texture);
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& GetTargetView();
	HRESULT SetShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& source);
	HRESULT SetMatrices(DirectX::CXMMATRIX, DirectX::CXMMATRIX, DirectX::CXMMATRIX, DirectX::CXMMATRIX);
	Microsoft::WRL::ComPtr<ID3D11Texture2D>& GetTexture();
	int textureWidth, textureHeight;

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTargetTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> textureTargetView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> offDepthStencilState;
	D3D11_VIEWPORT viewport;
	DirectX::XMMATRIX projectionMatrix;
	DirectX::XMMATRIX orthoMatrix, txworld, txview;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> oldTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> oldStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> olddepthStencilState;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> Layout;
	ID3D11Device* pdevice;

	D3D11_VIEWPORT oldViewPort;

    // Prevent copying.
    TextureElement(TextureElement const&);
    TextureElement& operator= (TextureElement const&);
};


    inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char* name)
    {
		
        #if defined(_DEBUG) || defined(PROFILE)
		if (nullptr != resource)
		{
            resource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
		}
        #else
            UNREFERENCED_PARAMETER(resource);
            UNREFERENCED_PARAMETER(name);
        #endif
    }