#include "Effects.h"
#include "SimpleMath.h"
#include "AlignedNew.h"
#include <d3d11.h>


struct VertexPositionNormalTextureStride
    {
        VertexPositionNormalTextureStride()
        { }

        VertexPositionNormalTextureStride(DirectX::XMFLOAT3 const& position, DirectX::XMFLOAT3 const& normal, DirectX::XMFLOAT2 const& textureCoordinate)
          : position(position),
            normal(normal),
            textureCoordinate(textureCoordinate)
        { }

        VertexPositionNormalTextureStride(DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        }

        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 textureCoordinate;

        static const int InputElementCount = 3;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
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
class GeometryEffect :
	public DirectX::IEffect, public DirectX::IEffectMatrices, public DirectX::AlignedNew<GeometryEffect>
{
public:
	explicit GeometryEffect(_In_ ID3D11Device* device);
	~GeometryEffect(void);
     GeometryEffect(GeometryEffect&& moveFrom);
     GeometryEffect& operator= (GeometryEffect&& moveFrom);

	void Apply(_In_ ID3D11DeviceContext* deviceContext);
	void GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength);
	void SetTexture(_In_ ID3D11ShaderResourceView* texture);

	void SetWorld(DirectX::CXMMATRIX value);
    void SetView(DirectX::CXMMATRIX value);
    void SetProjection(DirectX::CXMMATRIX value);
	void DisableGeometry(bool bEnable);
	void SetMatrices( DirectX::CXMMATRIX world,  DirectX::CXMMATRIX view,  DirectX::CXMMATRIX proj);
	// The underlying D3D object.
	
	DirectX::XMMATRIX lworld, lview, lproject;
private:
	class Impl;
	std::unique_ptr<Impl> pImpl;

	        // Prevent copying.
        GeometryEffect(GeometryEffect const&);
        GeometryEffect& operator= (GeometryEffect const&);
};