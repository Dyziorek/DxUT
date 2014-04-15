#pragma once

#include "stdafx.h"
#include  "VertexTypes.h"
#include "AlignedNew.h"


template <typename Vtx> class DXTxQuad : public DirectX::AlignedNew< DXTxQuad<Vtx> >
{
public:
	DXTxQuad(void);
	~DXTxQuad(void);

	
	DirectX::XMVECTOR Origin;
    DirectX::XMVECTOR UpperLeft;
    DirectX::XMVECTOR LowerLeft;
    DirectX::XMVECTOR UpperRight;
    DirectX::XMVECTOR LowerRight;
    DirectX::XMVECTOR Normal;
    DirectX::XMVECTOR Up;
    DirectX::XMVECTOR Left;
    DirectX::XMVECTOR upCenter;
    DirectX::XMVECTOR originTile;

    float Width;
    float Height;

    Vtx* Vertices;
    uint16_t* Indexes;
    DirectX::XMMATRIX* Transforms;
    int numberX;
    int numberY;
    int tick;
    int QuadCount;
	DXTxQuad(DirectX::XMVECTOR origin, DirectX::XMVECTOR normal, DirectX::XMVECTOR up, double width, double height, int numberX, int numberY);
	std::pair<std::vector<Vtx>, std::vector<uint16_t>> CreateCube(float size = 1, bool rhcoords = true);
	std::pair<std::vector<Vtx>, std::vector<uint16_t>> Create12Cube(float size = 1, bool rhcoords = true);
	std::pair<std::vector<Vtx>, std::vector<uint16_t>> CreateCubeAdjacent(float size = 1, bool rhcoords = true);
	
	//DirectX::XMVECTOR* CreatePolyOnBezier(DirectX::XMVECTOR* points, int count);
	//void Render(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	//HRESULT PrepareVSS(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void FillVertices();
};

