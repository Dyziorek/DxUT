#include "stdafx.h"
#include <map>
#include <algorithm>
#include "TextureElement.h"
#include "ConstantBuffer.h"
#include "PrimitiveBatch.h"
#include "SpriteBatch.h"
#include "CommonStates.h"
#include "DXGlyph.h"
#include "SpritePS.inc"
#include "SimpleMath.h"


std::shared_ptr<Microsoft::WRL::ComPtr<ID3D11PixelShader>> DXGlyph::shaderPS;

DXGlyph::DXGlyph(ID3D11Device* pDevice) : impl(), constants(pDevice), localEffect(pDevice)
{
	m_pDevice = pDevice;
	if (shaderPS.get() == nullptr)
	{
		shaderPS.reset(new Microsoft::WRL::ComPtr<ID3D11PixelShader>());
		pDevice->CreatePixelShader(g_main, sizeof(g_main), nullptr, shaderPS.get()->GetAddressOf());
	}
	
}


DXGlyph::~DXGlyph(void)
{
}

void DXGlyph::ScreenWidth(float x, float y)
{
	psData.pixelUnit.z = 1/((GetTexture()->textureHeight/x)*(GetTexture()->textureHeight/x)*impl.getGlyphData(0).txu);
	psData.pixelUnit.w = 1/((GetTexture()->textureWidth/y)*(GetTexture()->textureWidth/y)*impl.getGlyphData(0).txv);
}

void DXGlyph::Initialize(TextureElement* textureResource, std::vector<KeyFrame> keys)
{
	m_vecGlyphsTex.push_back(textureResource);
	m_vecKeyFrames = keys;

}

void DXGlyph::Initialize(TextureElement* textureResource, int GlyphChar, uint16_t txU, uint16_t txV, uint16_t txWidth, uint16_t txHeight)
{
	if (m_vecGlyphsTex.size() == 0)
	{
		m_vecGlyphsTex.push_back(textureResource);
	}
	impl.InitGlyph(GlyphChar, txU, txV, txWidth, txHeight);
}

void DXGlyph::UpdateViewportTransform(_In_ ID3D11DeviceContext* deviceContext)
{
    // Look up the current viewport.
    D3D11_VIEWPORT viewport;
    UINT viewportCount = 1;

    deviceContext->RSGetViewports(&viewportCount, &viewport);

    if (viewportCount != 1)
        throw std::exception("No viewport is set");

    // Compute the matrix.
    float xScale = (viewport.Width  > 0) ? 2.0f / viewport.Width  : 0.0f;
    float yScale = (viewport.Height > 0) ? 2.0f / viewport.Height : 0.0f;

	localTransform = DirectX::XMMATRIX
    (
         xScale,  0,       0,  0,
         0,      -yScale,  0,  0,
         0,       0,       1,  0,
        -1,       1,       0,  1
    );

	 localInverseTransform = DirectX::XMMatrixInverse(nullptr, localTransform);
}


void DXGlyph::DrawGlyph(ID3D11DeviceContext* pCtx, DirectX::SpriteBatch& renderer, DirectX::XMFLOAT2 position, int glyphChar, float scaleFactor, float rotation, float layer, bool customShader)
{
	m_pCtx = pCtx;
	DirectX::CommonStates coom(m_pDevice);
	DXGlyphElement& element = impl.getGlyphData(glyphChar);
	if (element.glyphChar >= 0)
	{
		if (customShader)
		{
		renderer.Begin(DirectX::SpriteSortMode_Deferred, nullptr, nullptr, nullptr,nullptr,std::bind(&DXGlyph::customPSShader, this));
		}
		else
		{
			renderer.Begin(DirectX::SpriteSortMode_Deferred);
		}
		float x = (element.glyphRectResult.right - element.glyphRectResult.left)/2 * scaleFactor;
		float y = (element.glyphRectResult.bottom - element.glyphRectResult.top)/2 * scaleFactor;
		DirectX::XMFLOAT2 xx(x,y);
		renderer.Draw(m_vecGlyphsTex[element.glyphTexID]->GetShaderResourceView().Get(), position, &element.glyphRectResult, DirectX::Colors::White,  rotation, xx , scaleFactor, DirectX::SpriteEffects_None, layer ); 
		renderer.End();
	}
}

void DXGlyph::DrawGlyph(ID3D11DeviceContext* ctx, DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>& renderer,  DirectX::XMFLOAT2 position, int glyphChar, float scaleFactor, float rotation, float layer, bool customShader)
{
	DXGlyphElement& elem = impl.getGlyphData(glyphChar);
	DirectX::XMVECTOR  vec =  DirectX::XMVector2Transform(DirectX::XMLoadFloat3(&elem.polyVertices.at(0).position), DirectX::XMMatrixTranslation(position.x, position.y, 0));
	DirectX::XMMATRIX result = DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(position.x, position.y, 0);
	
	
	localEffect.SetProjection(localTransform);
	localEffect.SetTextureEnabled(true);
	localEffect.SetTexture(m_vecGlyphsTex.at(0)->GetShaderResourceView().Get());

	for(int idx = 0; idx < elem.polyVertices.size(); idx++)
	{
		DirectX::XMStoreFloat3(&elem.polyVertices[idx].position, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&elem.polyVertices[idx].position), result));
	}

	//Microsoft::WRL::ComPtr<ID3D11InputLayout> LocLayout;
	byte* shaderBytes;
	size_t counter;
	localEffect.GetVertexShaderBytecode((const void**)&shaderBytes, &counter);
	if (LocLayout.Get() == nullptr)
	{
		m_pDevice->CreateInputLayout(DirectX::VertexPositionTexture::InputElements, DirectX::VertexPositionTexture::InputElementCount, shaderBytes, counter, LocLayout.GetAddressOf());
	}
	ctx->IASetInputLayout(LocLayout.Get());
	renderer.Begin();
	localEffect.Apply(ctx);
	renderer.DrawIndexed(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, elem.polyIndices.data(), elem.polyIndices.size(), elem.polyVertices.data(),elem.polyVertices.size());
	renderer.End();
}

void DXGlyph::DrawGlyph(ID3D11DeviceContext* ctx, DirectX::SpriteBatch& renderer, int glyphChar, float timeFrame, float layer)
{
	m_pCtx = ctx;
	
	DirectX::CommonStates coom(m_pDevice);
	DXGlyphElement element = impl.getGlyphData(glyphChar);
	if (element.glyphChar != 0)
	{
		DirectX::SimpleMath::Vector2 FramePosition;
		DirectX::SimpleMath::Vector2 FrameOrientation;
		KeyFrame prevFrame = m_vecKeyFrames.front();
		KeyFrame currFrame = m_vecKeyFrames.front();
		for (std::vector<KeyFrame>::iterator vecIt = m_vecKeyFrames.begin(); vecIt!= m_vecKeyFrames.end(); vecIt++)
		{
			if ((*vecIt).timePart <= timeFrame)
			{
				prevFrame = *vecIt;
				if (vecIt+1 != m_vecKeyFrames.end())
				{
					currFrame = *(vecIt+1);
				}
				else
				{
					currFrame = m_vecKeyFrames.front();
				}
			}
		}
		if (currFrame.timePart != m_vecKeyFrames.front().timePart)
		{
			// beginFrame
			float frameTimeSpan = currFrame.timePart - prevFrame.timePart;
			if (frameTimeSpan > 0)
			{
				// normal frame advance interpolate
				// it must be in range 0...1
				float timeFrameRange = (timeFrame - prevFrame.timePart)/frameTimeSpan;
				FramePosition = DirectX::SimpleMath::Vector2::SmoothStep(prevFrame.position, currFrame.position, timeFrameRange);
				FrameOrientation.x = prevFrame.orientation.x + (currFrame.orientation.x - prevFrame.orientation.x) * timeFrameRange;
				FrameOrientation.y = prevFrame.orientation.y + (currFrame.orientation.y - prevFrame.orientation.y) * timeFrameRange;
			}
			{
				// end of loop frame - no intepolation
			}
		}
		renderer.Begin(DirectX::SpriteSortMode_Deferred, nullptr, nullptr, nullptr,nullptr,std::bind(&DXGlyph::customPSShader, this));
		renderer.Draw(m_vecGlyphsTex[element.glyphTexID]->GetShaderResourceView().Get(), FramePosition, &element.glyphRect(), DirectX::Colors::White,  FrameOrientation.y,  DirectX::XMFLOAT2(128,128),  FrameOrientation.x, DirectX::SpriteEffects_None, layer ); 
		renderer.End();
	}
}



DXGlyph::DXGlyphImpl::DXGlyphImpl(void)
{
}

DXGlyph::DXGlyphImpl::DXGlyphImpl(DXGlyph* parent)
{
	InitializeGlyphs();
}

DXGlyph::DXGlyphImpl::~DXGlyphImpl(void)
{

}


void DXGlyph::DXGlyphImpl::InitializeGlyphs()
{
	DXGlyphElement milkyway('0', 0, 15);
	glyphs['0'] = milkyway;
	glyphs['1'] = DXGlyphElement('1', 0, 0);
	glyphs['2'] = DXGlyphElement('2', 0, 1);
	glyphs['3'] = DXGlyphElement('3', 0, 2);
	glyphs['4'] = DXGlyphElement('4', 0, 3);
	glyphs['5'] = DXGlyphElement('5', 0, 4);
	glyphs['6'] = DXGlyphElement('6', 0, 5);
	glyphs['7'] = DXGlyphElement('7', 0, 6);
	glyphs['8'] = DXGlyphElement('8', 0, 7);
	glyphs['9'] = DXGlyphElement('9', 0, 8);
	glyphs['A'] = DXGlyphElement('A', 0, 9);
	glyphs['B'] = DXGlyphElement('B', 0, 10);
	glyphs['C'] = DXGlyphElement('C', 0, 11);
	glyphs['D'] = DXGlyphElement('D', 0, 12);
	glyphs['E'] = DXGlyphElement('E', 0, 13);
	glyphs['F'] = DXGlyphElement('F', 0, 14);
	glyphs['G'] = DXGlyphElement('G', 0, 15);
	glyphs['H'] = DXGlyphElement('H', 0, 16);
	glyphs['I'] = DXGlyphElement('I', 0, 17);
	glyphs['J'] = DXGlyphElement('J', 0, 18);
	glyphs['K'] = DXGlyphElement('K', 0, 19);
	glyphs['L'] = DXGlyphElement('L', 0, 20);
	glyphs['M'] = DXGlyphElement('M', 0, 21);
	glyphs['N'] = DXGlyphElement('N', 0, 22);
	glyphs['O'] = DXGlyphElement('O', 0, 23);
	glyphs['P'] = DXGlyphElement('P', 0, 24);
	glyphs['Q'] = DXGlyphElement('Q', 0, 25);
	glyphs['R'] = DXGlyphElement('R', 0, 26);
	glyphs['S'] = DXGlyphElement('S', 0, 27);
	glyphs['T'] = DXGlyphElement('T', 0, 28);
	glyphs['U'] = DXGlyphElement('U', 0, 29);
	glyphs['V'] = DXGlyphElement('V', 0, 30);
	glyphs['X'] = DXGlyphElement('X', 0, 31);
	glyphs['Y'] = DXGlyphElement('Y', 0, 32);
	glyphs['Z'] = DXGlyphElement('Z', 0, 33);
	glyphs['['] = DXGlyphElement('[', 0, 34);
	glyphs[']'] = DXGlyphElement(']', 0, 35);
	glyphs['-'] = DXGlyphElement('-', 0, 36);
	glyphs['='] = DXGlyphElement('=', 0, 37);
	glyphs[';'] = DXGlyphElement(';', 0, 38);

	
}

static DirectX::XMVECTOR GetCircleVector(size_t i, size_t tessellation)
{
	float angle = i * DirectX::XM_2PI / tessellation + DirectX::XM_PI;
    float dx, dz;

    DirectX::XMScalarSinCos(&dx, &dz, angle);

    return DirectX::XMVectorPermute<0, 1, 4, 5>(DirectX::XMLoadFloat(&dx), DirectX::XMLoadFloat(&dz));
}



// Helper creates a triangle fan to close the end of a cylinder.
void DXGlyphElement::prepareCircle(float radius)
{
	int tessellation = 20;
	radius*=5.1f;
    // Create cap indices.
    for (size_t i = 0; i < tessellation - 2; i++)
    {
        size_t i1 = (i + 1) % tessellation;
        size_t i2 = (i + 2) % tessellation;

		std::swap(i1, i2);

		size_t vbase = polyVertices.size();
		polyIndices.push_back(vbase);
		polyIndices.push_back(vbase + i1);
		polyIndices.push_back(vbase + i2);
    }

    // Which end of the cylinder is this?
    DirectX::XMVECTOR normal = DirectX::g_XMIdentityR1;
	DirectX::XMVECTOR textureScale = DirectX::g_XMNegativeOneHalf;

	DirectX::XMMATRIX result = DirectX::XMMatrixRotationZ(DirectX::XM_PI);
	
	DirectX::XMVECTOR vecScale = DirectX::XMLoadFloat2(&DirectX::XMFLOAT2(0.766f/txu, 0.766f/txv));
	DirectX::XMVECTOR vecPos = DirectX::XMLoadFloat2(&DirectX::XMFLOAT2((float)texPosition.x/txw, (float)texPosition.y/txh ));
	DirectX::operator*=(vecPos, vecScale);

    // Create cap vertices.
    for (size_t i = 0; i < tessellation; i++)
    {
        DirectX::XMVECTOR circleVector = GetCircleVector(i, tessellation);

		DirectX::XMVECTOR position = DirectX::XMVectorSwizzle<0, 2, 1, 1>(DirectX::operator*(circleVector , radius));
		position = DirectX::XMVector3Transform(position, result);
		DirectX::XMVECTOR textureCoordinate = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSwizzle<0, 2, 3, 3>(circleVector), textureScale, DirectX::g_XMOneHalf);

		textureCoordinate = DirectX::XMVectorMultiplyAdd(textureCoordinate,vecScale, vecPos );
		//DirectX::operator*=(textureCoordinate, DirectX::XMLoadFloat2(&DirectX::XMFLOAT2(0.766f/txu, 0.766f/txv)));

		polyVertices.push_back(DirectX::VertexPositionTexture(position,  textureCoordinate));
    }
}


void DXGlyph::circle(float radius)
{
	impl.getGlyphData(m_glyph).circle = radius;
	impl.getGlyphData(m_glyph).prepareCircle(radius*5);
}

float DXGlyph::get_circle()
{
	return impl.getGlyphData(m_glyph).circle;
}

void DXGlyph::customPSShader()
{
	psData.texKeyColor  = DirectX::XMFLOAT4A(0,0,0,1);
	psData.circle = get_circle();
	psData.pixelUnit.x = impl.getGlyphData(m_glyph).txu;
	psData.pixelUnit.y = impl.getGlyphData(m_glyph).txv;
//	psData.pixelUnit.z = (GetTexture()->textureHeight/impl.getGlyphData(m_glyph).txh);
//	psData.pixelUnit.w = (GetTexture()->textureWidth/impl.getGlyphData(m_glyph).txw);


	constants.SetData(m_pCtx, psData);
	ID3D11Buffer* buff = constants.GetBuffer();
	m_pCtx->PSSetConstantBuffers(0, 1, &buff);

	m_pCtx->PSSetShader(shaderPS.get()->Get(), nullptr, 0);
	//m_pCtx->PSSetShader();

}