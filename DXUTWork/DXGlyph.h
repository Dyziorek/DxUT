#pragma once
#include <map>
#include "AlignedNew.h"
#include "ConstantBuffer.h"
class DXGlyphElement
{
public:
	int glyphChar;
	float circle;
	int8_t glyphTexID;
	uint16_t txh, txw;
	uint8_t txu, txv;
	DirectX::XMINT2 texPosition;
	RECT glyphRectResult;
	std::vector<DirectX::VertexPositionTexture> polyVertices;
	std::vector<uint16_t> polyIndices;
	DXGlyphElement()
	{ glyphChar = 0; circle = 0.0f;}
	DXGlyphElement(int charID, int8_t texID, uint16_t textPosNum, uint8_t texU = 8,uint8_t texV = 8, uint16_t textureW = 256, uint16_t textureH = 256 )
	{
		circle = 0.0f;
		glyphChar = charID;
		glyphTexID = texID;
		int relativeTexturePosition = textPosNum;
		int16_t tx = (relativeTexturePosition) % texU * textureW;
        int16_t ty = (float)floor((float)relativeTexturePosition / texU) * textureH;
		txh = textureH;
		txw = textureW;
		txu = texU;
		txv = texV;
		texPosition = DirectX::XMINT2(tx, ty);
		glyphRectResult = glyphRect();
	}
	RECT glyphRect()
	{
		RECT rect;
		int16_t top = texPosition.y;
		int16_t left = texPosition.x;
		int16_t bottom = (texPosition.y + txh);
		int16_t right = (texPosition.x + txw );
		RECT pos;
		pos.bottom = bottom;
		pos.top = top;
		pos.left = left;
		pos.right = right;
		return pos;
	}
	void prepareCircle(float radius);
};
struct PSConstant
	{
		DirectX::XMFLOAT4A texKeyColor;
		DirectX::XMFLOAT4A pixelUnit;
		float circle;
	};

struct KeyFrame
{
	DirectX::SimpleMath::Vector2 position;
	DirectX::SimpleMath::Vector2 orientation;
	float timePart;

	KeyFrame(float x, float y, float scale, float rotation, float time)
	{
		position = DirectX::SimpleMath::Vector2(x, y);
		orientation = DirectX::SimpleMath::Vector2(scale, rotation);
		timePart = time;
	}
};

class DXGlyph : public DirectX::AlignedNew<DXGlyph>
{
public:
	DXGlyph(ID3D11Device* pDevice);
	~DXGlyph(void);
	void Initialize(TextureElement* textureResource, std::vector<KeyFrame>);
	void Initialize(TextureElement* textureResource, int GlyphChar, std::vector<KeyFrame> kl) {Initialize(textureResource, kl); m_glyph = GlyphChar;}
	void Initialize(TextureElement* textureResource, int GlyphChar, uint16_t txU, uint16_t txV, uint16_t txWidth, uint16_t txHeight);
	void DrawGlyph(ID3D11DeviceContext* ctx, DirectX::SpriteBatch& renderer, DirectX::XMFLOAT2 position, int glyphChar, float scaleFactor, float rotation = 0, float layer = 0, bool customShader = true);
	void DrawGlyph(ID3D11DeviceContext* ctx, DirectX::SpriteBatch& renderer, int glyphChar, float timeFrame, float layer = 0);
	void DrawGlyph(ID3D11DeviceContext* ctx, DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>& renderer,  DirectX::XMFLOAT2 position, int glyphChar, float scaleFactor, float rotation = 0, float layer = 0, bool customShader = false);
	void SwitchGlyph(int glyphNum){m_glyph = glyphNum;}
	void ScreenWidth(float x, float y);
	TextureElement* GetTexture() { return this->m_vecGlyphsTex[0];}
	std::vector<TextureElement*> m_vecGlyphsTex;
	std::vector<KeyFrame> m_vecKeyFrames;
	DirectX::XMMATRIX localTransform;
	char m_glyph;
	void circle(float radius);
	float get_circle();
	Microsoft::WRL::ComPtr<ID3D11InputLayout> LocLayout;
	void UpdateViewportTransform(_In_ ID3D11DeviceContext* deviceContext);
private:
	class DXGlyphImpl
	{
		
		std::map<int, DXGlyphElement> glyphs;

	public:
		DXGlyphImpl(void);
		DXGlyphImpl(DXGlyph*);
		~DXGlyphImpl(void);
		void InitializeGlyphs();
		void InitGlyph(uint16_t glyphNum, uint16_t txu,uint16_t txv, uint16_t txwidth, uint16_t txh)
		{
			glyphs[glyphNum] = DXGlyphElement(glyphNum, 0, glyphNum, txu, txv, txwidth, txh);
		}
		DXGlyphElement& getGlyphData(int glyphChar) 
		{
			if  (glyphs.find(glyphChar) == glyphs.end()) 
				return glyphs.begin()->second;
			return glyphs[glyphChar]; 
		}
	};
	DirectX::ConstantBuffer<PSConstant> constants;

	DirectX::XMMATRIX localInverseTransform;
	DirectX::BasicEffect localEffect;
	PSConstant psData;
	void customPSShader();
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pCtx;
	static std::shared_ptr<Microsoft::WRL::ComPtr<ID3D11PixelShader>> shaderPS;
	DXGlyphImpl impl;
	
};

