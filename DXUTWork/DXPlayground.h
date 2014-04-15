#pragma once
class DXPlayground
{
public:
	DXPlayground(ID3D11Device* graph);
	~DXPlayground(void);

	SRWLOCK dataTransfer;
	DWORD m_dwThreadID;
	HANDLE physicsEvent;
	HANDLE drawEvent;
	b2World* m_World;
	b2Body* m_bomb;
	b2Body* m_ground;
	b2PolygonShape shapeBox;
	b2CircleShape shapeCircle;
	b2Vec2 viewSize;
	b2Vec2 pixelDensity;
	int rows,cols;
	TextureElement* currTexture;
	void SetView(float32 x, float32 y,ID3D11DeviceContext* currCtx );
	DXGlyph* m_glyphRender;
	std::vector<b2Body*> m_Boxes;
	std::vector<b2Fixture*> m_Boxesinfo;
	ID3D11Device* m_graph;
	std::vector<DirectX::XMFLOAT2> blockPositions;
	std::vector<float> blockAngles;
	void AddGlyph(int GlyphID, TextureElement* GlyphResource, uint16 rows, uint16 cols, uint16 w, uint16 h);
	bool InitializeGlyph(float32 x, float32 y, int number);
	int CreateBlock(int blockRows,  int blockColumns, TextureElement* resource);
	DirectX::XMFLOAT2 GetScreenPosBlock(int blockNum);
	DirectX::XMFLOAT2 GetScreenPosBlockEx(int blockNum){
		AcquireSRWLockShared(&dataTransfer);
		if (blockPositions.size() > blockNum)
		{
			ReleaseSRWLockShared(&dataTransfer);
			return blockPositions[blockNum];
		}
		ReleaseSRWLockShared(&dataTransfer);
	}
	DirectX::XMFLOAT2 GetPosBlock(int blockNum);
	void AdvanceStep();
	void GetStep() {SetEvent(physicsEvent);}
	float GetScreenAngleBlock(int blockNum);
	float GetScreenAngleBlockEx(int blockNum)
	{	
		AcquireSRWLockShared(&dataTransfer);
		if (blockAngles.size() > blockNum)
		{
			ReleaseSRWLockShared(&dataTransfer);
			return blockAngles[blockNum];
		}
		ReleaseSRWLockShared(&dataTransfer);
	}
	DXGlyph* GetGlyph(int glyphNum);
	int GetCount() {  AcquireSRWLockShared(&dataTransfer); 
			int dataLocal = blockPositions.size();
			ReleaseSRWLockShared(&dataTransfer);
			return dataLocal;
			}
	void Bombarding();
	void Reset();
	bool IsMoving();
	bool triggered;
};

