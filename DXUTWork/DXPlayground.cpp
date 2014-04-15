#include "stdafx.h"
#include "SimpleMath.h"
#include "TextureElement.h"
#include "DXGlyph.h"
#include "Box2D\Box2D.h"

#include "DXPlayground.h"

DXPlayground::DXPlayground(ID3D11Device* graph) : m_World(new b2World(b2Vec2(0, -10.0f)))
{
	m_graph = graph;
	m_bomb = nullptr;
	m_glyphRender = nullptr;
	b2BodyDef bd;
	b2Body* ground = m_World->CreateBody(&bd);

	b2EdgeShape shape;
	shape.Set(b2Vec2(-10.0f, 0.0f), b2Vec2(50.0f, 0.0f));
	ground->CreateFixture(&shape, 0.0f);

	m_ground = ground;
	//float32 a = 1.5f;
	//shapeBox.SetAsBox(a, a);
	physicsEvent = CreateEvent(NULL, TRUE, FALSE, L"PhysicsInterception");
	drawEvent  = CreateEvent(NULL, TRUE, FALSE, L"DrawInterception");
	ResetEvent(physicsEvent);
	ResetEvent(drawEvent);
	InitializeSRWLock(&dataTransfer);
	triggered = false;
	m_dwThreadID = 0;
}


DXPlayground::~DXPlayground(void)
{
	CloseHandle(physicsEvent);
	CloseHandle(drawEvent);
}


void DXPlayground::AddGlyph(int GlyphID, TextureElement* GlyphResource, uint16 cols, uint16 rows, uint16 w, uint16 h)
{
	if (m_glyphRender == nullptr)
		m_glyphRender = new DXGlyph(m_graph);
	m_glyphRender->Initialize(GlyphResource, GlyphID, cols,rows,w,h);
}


bool DXPlayground::InitializeGlyph(float32 x, float32 y, int number)
{

	b2BodyDef bd;
	bd.type = b2_dynamicBody;
	bd.position = b2Vec2(x,y);
	m_glyphRender->SwitchGlyph(number);
	m_Boxes.push_back(m_World->CreateBody(&bd));
	if (number % 2 == 2)
	{
		m_Boxesinfo.push_back(m_Boxes.back()->CreateFixture(&shapeBox, 0.50f));
	}
	else
	{
		m_Boxesinfo.push_back(m_Boxes.back()->CreateFixture(&shapeCircle, 0.50f));
		m_glyphRender->circle(shapeCircle.m_radius);
	}

	return true;
}

void DXPlayground::SetView(float32 x, float32 y, ID3D11DeviceContext* currCtx)
{
	// viewbox is like 0, 40 width and 0, 30 height
	viewSize.y = y;
	viewSize.x = x;

	pixelDensity.x = x / 40;
	pixelDensity.y = y / 30;


	if (m_glyphRender != nullptr)
	{
		m_glyphRender->UpdateViewportTransform(currCtx);
	}
}


DWORD __stdcall ThreadProcess(void* threadInfo)
{
	DXPlayground* pWork = (DXPlayground*)threadInfo;
	while(1)
	{
		if (WaitForSingleObject(pWork->physicsEvent, INFINITE) == WAIT_OBJECT_0)
		{
			ResetEvent(pWork->physicsEvent);
			pWork->Bombarding();
			pWork->AdvanceStep();
			SetEvent(pWork->drawEvent);
		}
	}
}

int DXPlayground::CreateBlock(int blockRows, int blockColumns, TextureElement* resource)
{


	uint16 txWidth = resource->textureWidth;
	uint16 txHeight = resource->textureHeight;


	float32 boxW = viewSize.x/blockColumns;
	float32 boxH = viewSize.y/blockRows;
	

	int numCounter = 0;
	b2Vec2 pos(0.05f + boxW/pixelDensity.x/2, blockRows*boxH/pixelDensity.y+0.05f-boxH/pixelDensity.y/2);

	b2Vec2 deltaX(boxW/pixelDensity.x+0.01f, 0.0f);
	b2Vec2 deltaY(0.0f, boxH/pixelDensity.y + 0.01f);
	b2Vec2 posBox;

	shapeBox.SetAsBox(boxW/pixelDensity.x/2,boxH/pixelDensity.y/2);

	shapeCircle.m_radius = 0.95f * boxW/pixelDensity.x/2;

	for (int rX = 0; rX<blockRows; rX++)
	{
		posBox = pos;
		for(int rY = 0; rY<blockColumns; rY++)
		{
			AddGlyph(numCounter, resource, blockColumns, blockRows, boxW, boxH);
			InitializeGlyph(posBox.x, posBox.y, numCounter);
			posBox+=deltaX;
			numCounter++;
		}
		pos -= deltaY;
	}
	rows = blockRows;
	cols = blockColumns;
	currTexture = resource;
	m_glyphRender->ScreenWidth(viewSize.y, viewSize.y);

	if (m_dwThreadID == 0)
	{
		CreateThread(NULL, 0, ThreadProcess, this, 0, &m_dwThreadID);
	}

	return 0;
}

void DXPlayground::AdvanceStep()
{
	if (m_World != nullptr)
	{
		AcquireSRWLockExclusive(&dataTransfer);
		m_World->Step(1/30.0f, 6, 2);
		ReleaseSRWLockExclusive(&dataTransfer);
		AcquireSRWLockExclusive(&dataTransfer);
		blockPositions.clear();
		blockAngles.clear();
		for (int blockID = 0; blockID< m_Boxes.size(); blockID++)
		{
			blockPositions.push_back(GetScreenPosBlock(blockID));
			blockAngles.push_back(GetScreenAngleBlock(blockID));
		}
		ReleaseSRWLockExclusive(&dataTransfer);
	}
	
}

DirectX::XMFLOAT2 DXPlayground::GetPosBlock(int blockNum)
{
	b2Vec2 pos = m_Boxes.at(blockNum)->GetPosition();
	
	DirectX::XMFLOAT2 vecPos(0,0);
	vecPos.x = pos.x;
	vecPos.y = pos.y;
	return vecPos;
}


DirectX::XMFLOAT2 DXPlayground::GetScreenPosBlock(int blockNum)
{
	float32 hy = (shapeBox.GetVertex(2) - shapeBox.GetVertex(0)).y;
	float32 hx = (shapeBox.GetVertex(2) - shapeBox.GetVertex(0)).x;
	
	b2Fixture* bodyInfo = m_Boxesinfo.at(blockNum);
	if (bodyInfo->GetType() == b2Shape::e_circle)
	{
		b2Vec2 pos = m_Boxes.at(blockNum)->GetPosition();
		DirectX::XMFLOAT2 vecPos(0,0);
		
		vecPos.x = pos.x * pixelDensity.x;
		vecPos.y = viewSize.y - (pos.y * pixelDensity.y);
		if (vecPos.x < -(hx* pixelDensity.x) || vecPos.x > viewSize.x+(hx* pixelDensity.x) || vecPos.y > viewSize.y+(hy* pixelDensity.y))
		{
			m_Boxes.at(blockNum)->SetActive(false);
		}
		return vecPos;
	}
	else
	{
		b2Vec2 pos = m_Boxes.at(blockNum)->GetPosition();
		DirectX::XMFLOAT2 vecPos(0,0);
		
		vecPos.x = pos.x * pixelDensity.x;
		vecPos.y = viewSize.y - (pos.y * pixelDensity.y);
		if (vecPos.x < -(hx* pixelDensity.x) || vecPos.x > viewSize.x+(hx* pixelDensity.x) || vecPos.y > viewSize.y+(hy* pixelDensity.y))
		{
			m_Boxes.at(blockNum)->SetActive(false);
		}
		return vecPos;
	}
}

float DXPlayground::GetScreenAngleBlock(int blockNum)
{


	
	b2Fixture* bodyInfo = m_Boxesinfo.at(blockNum);
	if (bodyInfo->GetType() == b2Shape::e_circle)
	{
		b2CircleShape* circle = (b2CircleShape*)bodyInfo->GetShape();
		b2Transform transFmt = m_Boxes.at(blockNum)->GetTransform();
		b2Vec2 posAngle = b2Mul(transFmt.q, b2Vec2(1.0f, 0.0f));
		DirectX::XMFLOAT2 vecPos(1,0);
		DirectX::XMFLOAT2 vecPosAng(1,0);
		vecPosAng.x = posAngle.x;
		vecPosAng.y = posAngle.y;
		float32 angle = m_Boxes.at(blockNum)->GetAngle();
		
		float32 angDX = DirectX::XMVector2AngleBetweenVectors(DirectX::XMLoadFloat2(&vecPosAng), DirectX::XMLoadFloat2(&vecPos)).m128_f32[0];
		return angDX;
	}
	else
	{

	float32 angle = m_Boxes.at(blockNum)->GetAngle();
	
	return angle;
	}
}

DXGlyph* DXPlayground::GetGlyph(int glyphNum)
{
	m_glyphRender->SwitchGlyph(glyphNum);
	return m_glyphRender;
}

#define	RAND_LIMIT	32767

inline float32 RandomFloat(float32 lo, float32 hi)
{
	float32 r = (float32)(std::rand() & (RAND_LIMIT));
	r /= RAND_LIMIT;
	r = (hi - lo) * r + lo;
	return r;
}

void DXPlayground::Reset()
{
	if (!IsMoving())
	{
		AcquireSRWLockExclusive(&dataTransfer);
		delete m_World;
		m_World = new b2World(b2Vec2(0,-10.f));
		b2BodyDef bd;
		b2Body* ground = m_World->CreateBody(&bd);
		m_ground = ground;
		b2EdgeShape shape;
		shape.Set(b2Vec2(-10.0f, 0.0f), b2Vec2(50.0f, 0.0f));
		ground->CreateFixture(&shape, 0.0f);
		m_bomb = nullptr;
		DirectX::XMMATRIX clone = m_glyphRender->localTransform;
		delete m_glyphRender;
		m_Boxes.clear();
		m_Boxesinfo.clear();
		m_glyphRender = new DXGlyph(m_graph);
		m_glyphRender->localTransform = clone;
		CreateBlock(rows,cols, currTexture);
		ReleaseSRWLockExclusive(&dataTransfer);
		
	}
}

bool DXPlayground::IsMoving()
{
	
	if (m_Boxes.size() > 0)
	{
		int awakeNum = 0;
		std::for_each(m_Boxes.begin(), m_Boxes.end(), [&awakeNum](b2Body* x) 
		{
			if ((x->GetLinearVelocity().Length() > 1.0) && x->IsActive()) 
			{
					awakeNum ++;
			}
		});
		
		return awakeNum > 0;
	}

	return false;
}

void DXPlayground::Bombarding()
{
	if (triggered)
	{
		triggered = false;
	//if (m_ground != nullptr)
	//{
	//	m_World->DestroyBody(m_ground);
	//	m_ground = nullptr;
	//}

	if (m_bomb == nullptr)
	{
  		b2Vec2 p(RandomFloat(0.0f, 40.0f), 0.0f  );
 		b2Vec2 v = -5.0f * p;
		b2BodyDef bd;
		bd.type = b2_dynamicBody;
		bd.position = p;
		bd.bullet = true;
		m_bomb = m_World->CreateBody(&bd);
		m_bomb->SetLinearVelocity(b2Vec2(0,500));
		b2FixtureDef fd;
		fd.shape = &shapeCircle;
		fd.density = 20.0f;
		fd.restitution = 0.0f;
	
		b2Vec2 minV = v - b2Vec2(0.3f,0.3f);
		b2Vec2 maxV = v + b2Vec2(0.3f,0.3f);
	
		b2AABB aabb;
		aabb.lowerBound = minV;
		aabb.upperBound = maxV;

		m_bomb->CreateFixture(&fd);
	}
	else
	{
		m_World->DestroyBody(m_bomb);
		m_bomb = nullptr;
	}
	
	}
}
