#include "Effects.h"
#include "SimpleMath.h"
#include "AlignedNew.h"

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

enum PSShaderType
{
	BloomExtract = 2,
	BloomCombine = 1,
	GauussBlur = 0,
};

/*
template<UINT TNameLength>
inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char (&name)[TNameLength])
{
    #if defined(_DEBUG) || defined(PROFILE)
        resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
    #else
        UNREFERENCED_PARAMETER(resource);
        UNREFERENCED_PARAMETER(name);
    #endif
}
*/
class BlurEffect :
	public DirectX::IEffect, public DirectX::IEffectMatrices, public DirectX::AlignedNew<BlurEffect>
{
public:
	explicit BlurEffect(_In_ ID3D11Device* device);
	~BlurEffect(void);
     BlurEffect(BlurEffect&& moveFrom);
     BlurEffect& operator= (BlurEffect&& moveFrom);

	 void SetData(float blurAmount, DirectX::XMFLOAT2 delta, float BaseIntensity,	float BloomSaturation,	float BaseSaturation,	float BloomThreshold);

	void Apply(_In_ ID3D11DeviceContext* deviceContext);
	void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);
	void SetTexture(_In_ ID3D11ShaderResourceView* texture, _In_ ID3D11ShaderResourceView* texture2 = nullptr);
	void SetPSShader(PSShaderType ShaderType) {currentPS = ShaderType;}
	float ComputeGauss(float factor, int Amount);

	void SetWorld(DirectX::CXMMATRIX value);
    void SetView(DirectX::CXMMATRIX value);
    void SetProjection(DirectX::CXMMATRIX value);

	void SetMatrices( DirectX::CXMMATRIX world,  DirectX::CXMMATRIX view,  DirectX::CXMMATRIX proj);
	// The underlying D3D object.
	PSShaderType currentPS;
	DirectX::XMMATRIX lworld, lview, lproject;
private:
	class Impl;
	std::unique_ptr<Impl> pImpl;

	        // Prevent copying.
        BlurEffect(BlurEffect const&);
        BlurEffect& operator= (BlurEffect const&);
};