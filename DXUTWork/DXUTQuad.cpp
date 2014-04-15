#include "stdafx.h"
#include "DXUTQuad.h"


#include <algorithm>
#include <set>
#include <functional>
#include <map>

using namespace DirectX;

template <typename Vtx>
DXTxQuad<Vtx>::DXTxQuad(void)
{
	
}

template <typename Vtx>
DXTxQuad<Vtx>::~DXTxQuad(void)
{
}

struct Face
{
	Face(uint16_t one, uint16_t two, uint16_t three)
	{
		faceIndex[0] = one;
		faceIndex[1] = two;
		faceIndex[2] = three;
	}
	uint16_t faceIndex[3];
};

// Stores an edge by its vertices and force an order between them
struct Edge
{
    Edge(uint16_t _a, uint16_t _b)
    {
        assert(_a != _b);
        
        if (_a < _b)
        {
            a = _a;
            b = _b;                   
        }
        else
        {
            a = _b;
            b = _a;
        }
    }
    
    uint16_t a;
    uint16_t b;
};

struct Neighbors
{
    uint16_t n1;
    uint16_t n2;
    
    Neighbors()
    {
        n1 = n2 = 65535;
    }
    
    void AddNeigbor(uint16_t n)
    {
        if (n1 == 65535) {
            n1 = n;
        }
        else if (n2 == 65535) {
            n2 = n;
        }
        else {
            assert(0);
        }
    }
    
    uint16_t GetOther(uint16_t me) const
    {
        if (n1 == me) {
            return n2;
        }
        else if (n2 == me) {
            return n1;
        }
        else {
            assert(0);
        }

        return 0;
    }
};

struct CompareEdges
{
    bool operator()(const Edge& Edge1, const Edge& Edge2)
    {
        if (Edge1.a < Edge2.a) {
            return true;
        }
        else if (Edge1.a == Edge2.a) {
            return (Edge1.b < Edge2.b);
        }        
        else {
            return false;
        }            
    }
};


class vecComp
{
public:
	 bool operator()(const VertexPositionNormalTexture& arg1, const VertexPositionNormalTexture& arg2)
	 {
		 float hash1 = 31 * arg1.position.x + 37 * arg1.position.y + 17 * arg1.position.z;
		 float hash2 = 31 * arg2.position.x + 37 * arg2.position.y + 17 * arg2.position.z;
		 return hash1 < hash2;
	 }
};


 // Helper for flipping winding of geometric primitives for LH vs. RH coords
static void ReverseWinding( std::vector<uint16_t>& indices, std::vector<VertexPositionNormalTexture>& vertices )
    {
        assert( (indices.size() % 3) == 0 );
        for( auto it = indices.begin(); it != indices.end(); it += 3 )
        {
            std::swap( *it, *(it+2) );
        }

        for( auto it = vertices.begin(); it != vertices.end(); ++it )
        {
            it->textureCoordinate.x = ( 1.f - it->textureCoordinate.x );
        }
    }

template <typename Vtx>
DXTxQuad<Vtx>::DXTxQuad(XMVECTOR origin, XMVECTOR normal, XMVECTOR up, double width, double height, int numberX, int numberY)
{

	tick = 0;
    this->numberX = numberX;
    this->numberY = numberY;
    QuadCount = numberX * numberY;
    Vertices = new Vtx[4 * numberX * numberY];
	//Transforms = new XMMATRIX[numberX * numberY];
    Indexes = new uint16_t[6 * numberX * numberY];
    Origin = origin;
    Normal = normal;
    Up = up;
    Width = width;
    Height = height;
            // Calculate the quad corners
	Left = XMVector3Cross(  Normal, Up);
    upCenter = (Up * height / 2) + origin;
    UpperLeft = upCenter + (Left * width / 2);
    UpperRight = upCenter - (Left * width / 2);
    LowerLeft = UpperLeft - (Up * height);
    LowerRight = UpperRight - (Up * height);
    originTile = UpperLeft;
            //D3DXVECTOR2* plVectors = new D3DXVECTOR2[6] ;
            //plVectors[0] = D3DXVECTOR2(0.0f,0.0f));
            //plVectors[1] = D3DXVECTOR2(10.1f,10.1f));
            //plVectors[2] = D3DXVECTOR2(20.2f,10.2f));
            //plVectors[3] = D3DXVECTOR2(30.3f,17.3f));
            //plVectors[4] = D3DXVECTOR2(35.4f,17.4f));
            //plVectors[5] = D3DXVECTOR2(40.5f,30.5f));
            //D3DXVECTOR2* verRes = CreatePolyOnBezier(plVectors);
            //System.Diagnostics.Debug.Write(verRes.Count);
            FillVertices();
}

class vecLess
{
	VertexPositionNormalTexture locCheck;
public:
	vecLess(const VertexPositionNormalTexture arg2)
	{
		locCheck = arg2;
	}
	bool operator()(const std::pair<VertexPositionNormalTexture, std::vector<int>> arg1)
	{
		{bool bRet = XMVector3Equal( XMLoadFloat3(&arg1.first.position) , XMLoadFloat3(&locCheck.position)); 
		   return bRet;};
	};


};

struct triangle
{
	VertexPositionNormalTexture vertices[3];
	uint16_t indexes[3];
};

template <typename Vtx>
std::pair<std::vector<Vtx>, std::vector<uint16_t>> DXTxQuad<Vtx>::CreateCube(float size = 1, bool rhcoords = true)
{
	// A cube has six faces, each one pointing in a different direction.
    const int FaceCount = 6;

    static const XMVECTORF32 faceNormals[FaceCount] =
    {
        {  0,  0,  1 },
        {  0,  0, -1 },
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
    };

    static const XMVECTORF32 textureCoordinates[4] =
    {
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
    };

	std::vector<Vtx> vertices;
	std::vector<uint16_t> indices;
	std::vector<triangle> triangles;
	
	std::vector<std::pair<VertexPositionNormalTexture, std::vector<int>>> uniqueVerts;
    size /= 2;

	std::set<VertexPositionNormalTexture> vv;


	std::vector<Face> faces;
	std::map<VertexPositionNormalTexture, uint16_t, vecComp> uniqueVertices;

    // Create each face in turn.
    for (int i = 0; i < FaceCount; i++)
    {
        XMVECTOR normal = faceNormals[i];

        // Get two vectors perpendicular both to the face normal and to each other.
        XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        // Six indices (two triangles) per face.
        size_t vbase = vertices.size();
        indices.push_back(vbase + 0);
        indices.push_back(vbase + 1);
        indices.push_back(vbase + 2);

        indices.push_back(vbase + 0);
        indices.push_back(vbase + 2);
        indices.push_back(vbase + 3);

		faces.push_back(Face(vbase + 2, vbase + 1, vbase + 0));
		faces.push_back(Face(vbase + 3, vbase + 2, vbase + 0));

		triangle trg;
		trg.vertices[0] = VertexPositionNormalTexture((normal - side1 - side2) * size, normal, textureCoordinates[0]);
		trg.vertices[1] = VertexPositionNormalTexture((normal - side1 + side2) * size, normal, textureCoordinates[1]);
		trg.vertices[2] = VertexPositionNormalTexture((normal + side1 + side2) * size, normal, textureCoordinates[2]);
		trg.indexes[0] = vbase + 0;
		trg.indexes[1] = vbase + 1;
		trg.indexes[2] = vbase + 2;
		triangles.push_back(trg);
		trg.vertices[0] = VertexPositionNormalTexture((normal - side1 - side2) * size, normal, textureCoordinates[0]);
		trg.vertices[1] = VertexPositionNormalTexture((normal + side1 + side2) * size, normal, textureCoordinates[2]);
		trg.vertices[2] = VertexPositionNormalTexture((normal + side1 - side2) * size, normal, textureCoordinates[3]);
		trg.indexes[0] = vbase + 0;
		trg.indexes[1] = vbase + 2;
		trg.indexes[2] = vbase + 3;
		triangles.push_back(trg);



        // Four vertices per face.
        vertices.push_back(VertexPositionNormalTexture((normal - side1 - side2) * size, normal, textureCoordinates[0]));
        vertices.push_back(VertexPositionNormalTexture((normal - side1 + side2) * size, normal, textureCoordinates[1]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 + side2) * size, normal, textureCoordinates[2]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 - side2) * size, normal, textureCoordinates[3]));
    }

	//	//step 1: build a list of unique vertices
	//for each triangle
	//   for each vertex in triangle
	//      if not vertex in listOfVertices
	//         Add vertex to listOfVertices
	//
	////step 2: build a list of faces 
	//for each triangle
	//   for each vertex in triangle
	//      Get Vertex Index From listOfvertices
	//      AddToMap(vertex Index, triangle)

	//for(std::vector<triangle>::iterator trIt = triangles.begin(); trIt != triangles.end(); trIt++)
	//{
	//	bool bRet = XMVector3Equal( XMLoadFloat3((*trIt).vertices[0]) , XMLoadFloat3(&locCheck.position)
	//}

	int index = 0;
	for(std::vector<Vtx>::iterator itval = vertices.begin(); itval != vertices.end(); itval++)
	{

		if ( std::find_if(uniqueVerts.begin(), uniqueVerts.end(), vecLess(*itval)) == uniqueVerts.end())
		{   
			std::pair<VertexPositionNormalTexture, std::vector<int>> px = std::make_pair(*itval, std::vector<int>());
			px.second.push_back(index);
			uniqueVerts.push_back(px);
		}
		else
		{
			std::vector<std::pair<VertexPositionNormalTexture, std::vector<int>>>::iterator itUq;
			itUq = std::find_if(uniqueVerts.begin(), uniqueVerts.end(), vecLess(*itval));
			(*itUq).second.push_back(index);
		}
		index++;
	}

	for(std::vector<triangle>::iterator trIt = triangles.begin(); trIt != triangles.end(); trIt++)
	{
		
	}


	if (rhcoords == false)
	{
		ReverseWinding(indices, vertices);
	}


		std::map<Edge, Neighbors, CompareEdges> indexMap;

	Face Unique(-1,-1,-1);
	std::vector<Face> Ufaces;
	for(int idx = 0; idx < faces.size(); idx++)
	{
		Face face = faces[idx];
        for (uint16_t j = 0 ; j < 3 ; j++) { 
			uint16_t Index = face.faceIndex[j];
            VertexPositionNormalTexture& v = vertices[Index];

            if (uniqueVertices.find(v) == uniqueVertices.end()) {
                uniqueVertices[v] = Index;
            }
            else {
                Index = uniqueVertices[v];
            } 

            Unique.faceIndex[j] = Index;
        }

        Ufaces.push_back(Unique);

		Edge e1(Ufaces[idx].faceIndex[0], Ufaces[idx].faceIndex[1]);
        Edge e2(Ufaces[idx].faceIndex[1], Ufaces[idx].faceIndex[2]);
        Edge e3(Ufaces[idx].faceIndex[2], Ufaces[idx].faceIndex[0]);
        
        indexMap[e1].AddNeigbor(idx);
        indexMap[e2].AddNeigbor(idx);
        indexMap[e3].AddNeigbor(idx);
	}

	std::for_each(faces.begin(), faces.end(), [&](Face faceIt)
	{
		VertexPositionNormalTexture& v0 = vertices[faceIt.faceIndex[0]];
		VertexPositionNormalTexture& v1 = vertices[faceIt.faceIndex[1]];
		VertexPositionNormalTexture& v2 = vertices[faceIt.faceIndex[2]];
		XMVECTOR normal = XMVector3Normalize( XMVector3Cross( XMLoadFloat3(&v1.position) - XMLoadFloat3(&v0.position), 
			 XMLoadFloat3(&v2.position) - XMLoadFloat3(&v0.position)));
		auto ress = std::find_if(std::begin(faceNormals), std::end(faceNormals), [&](XMVECTOR locNorm)
		{
			return XMVector3Equal(normal, locNorm);
		});
		XMVector3Equal(*ress, normal);
	});
	

	std::vector<uint16_t> Calcindices;

	for (uint16_t i = 0 ; i < Ufaces.size() ; i++) {        
        
        
        for (uint16_t j = 0 ; j < 3 ; j++) {            
            Edge e(Ufaces[i].faceIndex[j], Ufaces[i].faceIndex[(j + 1) % 3]);
            assert(indexMap.find(e) != indexMap.end());
            Neighbors n = indexMap[e];
            uint16_t OtherTri = n.GetOther(i);
            
            if (OtherTri == 65535)
                OtherTri = 0;
            
            
			uint16_t OppositeIndex = 0;

			for (uint16_t loci = 0 ; loci < 3 ; loci++) {
				uint16_t Index = Ufaces[OtherTri].faceIndex[loci];

			if (Index != e.a && Index != e.b) {
					OppositeIndex = Index;
				}
			}

         
            Calcindices.push_back(faces[i].faceIndex[j]);
            Calcindices.push_back(OppositeIndex);            
        }
    }    

	std::vector<uint16_t> adjacent;
	adjacent.resize(indices.size() * 2);


	std::vector<uint16_t> uniq;
	uniq.resize(indices.size());
	// unificate indices as copy from original.
	std::copy(indices.begin(), indices.end(), uniq.begin());

	for (int i = 0; i < indices.size() ; i++)
	{
		for(std::vector<std::pair<VertexPositionNormalTexture, std::vector<int>>>::iterator uq = uniqueVerts.begin();
			uq != uniqueVerts.end(); uq++)
		{
			for (int px = 1; px < (*uq).second.size(); px++)
			{
				if ((*uq).second[px] == indices[i])
				{
					uniq[i] = (*uq).second[0];
				}
			}

		}

	}


	// generate adjacency information
	// adjacent in closed cube is always somewhere other index

	for (int i = 0; i < indices.size()/3 ; i++)
	{
		uint16_t i0 = uniq[i*3+0];
		uint16_t i1 = uniq[i*3+1];
		uint16_t i2 = uniq[i*3+2];
		
		adjacent[i*6 + 0] = uniq[i*3+0];
		adjacent[i*6 + 1] = -1;
		adjacent[i*6 + 2] = uniq[i*3+1];
		adjacent[i*6 + 3] = -1;
		adjacent[i*6 + 4] = uniq[i*3+2];
		adjacent[i*6 + 5] = -1;
		
		for(uint16_t j=0; j<indices.size()/3; j++)	
		{		
			if( j != i ) // don't check a triangle against itself		
			{	
				uint16_t j0 = uniq[j*3+0];
				uint16_t j1 = uniq[j*3+1];
				uint16_t j2 = uniq[j*3+2];
				// check for i0 and i1	
				if( j0 == i0 )	
				{
					if(j1==i1) 
						adjacent[i*6+1] = j2;
					else if(j2==i1) 
						adjacent[i*6+1] = j1;
				} else if( j1==i0 )	
				{
					if(j0==i1)
						adjacent[i*6+1] = j2;
					else if(j2==i1)
						adjacent[i*6+1] = j0;
				} else if( j2==i0 )
				{
					if(j0==i1) 
						adjacent[i*6+1] = j1;
					else if(j1==i1)
						adjacent[i*6+1] = j0;
				}
				// check for i1 and i2		
				if( j0 == i1 )	
				{
					if(j1==i2)
						adjacent[i*6+3] = j2;	
					else if(j2==i2)
						adjacent[i*6+3] = j1;	
				} 
				else if( j1==i1 )	
				{
					if(j0==i2)
						adjacent[i*6+3] = j2;
					else if(j2==i2)
						adjacent[i*6+3] = j0;	
				}
				else if( j2==i1 )
				{	
					if(j0==i2)
						adjacent[i*6+3] = j1;
					else if(j1==i2)
						adjacent[i*6+3] = j0;	
				}
				// check for i2 and i0		
				if( j0 == i2 )		
				{
					if(j1==i0)
						adjacent[i*6+5] = j2;
					else if(j2==i0)
						adjacent[i*6+5] = j1;			
				}
				else if( j1==i2 )
				{
					if(j0==i0)
						adjacent[i*6+5] = j2;
					else if(j2==i0)
						adjacent[i*6+5] = j0;	
				}
				else if( j2==i2 )	
				{
					if(j0==i0)
						adjacent[i*6+5] = j1;	
					else if(j1==i0)
						adjacent[i*6+5] = j0;
				}
			}
		}	
	}
	std::vector<Vtx> qvertices;
	qvertices.resize(8);
	std::copy(vertices.begin(), vertices.begin()+8, qvertices.begin());

	return std::pair<std::vector<Vtx>, std::vector<uint16_t>>(vertices, Calcindices);
}


template <typename Vtx>
std::pair<std::vector<Vtx>, std::vector<uint16_t>> DXTxQuad<Vtx>::Create12Cube(float size = 1, bool rhcoords = true)
{
            // Calculate constants that will be used to generate vertices
            auto phi = (float) (sqrt(5) - 1) / 2; // The golden ratio
            auto R = (float) (size / sqrt(3));

            auto a = (R * 1);
            auto b = R / phi;
            auto c = R * phi;

            // Generate each vertex
            auto vertices = std::vector<VertexPositionNormalTexture>();
            /*for (auto i = -1; i < 2; i += 2)
            {
                for (auto j = -1; j < 2; j += 2)
                {
					XMFLOAT3 posLoc1(0, i * c * R, j * b * R);
					XMFLOAT3 posLoc2(i * c * R, j * b * R, 0);
					XMFLOAT3 posLoc3(i * b * R, 0, j * c * R);
					
					vertices.push_back(new VertexPositionNormalTexture );
                    vertices.push_back(new VertexPositionNormalTexture );
                    vertices.push_back(new VertexPositionNormalTexture );

                    for (auto k = -1; k < 2; k += 2)
					{
						XMFLOAT3 posLocSide( i * a * R ,j * a * R ,k * a * R);
                        vertices.push_back(new VertexPositionNormalTexture();
					}
                }
            }*/
			std::vector<uint16_t> pss;
			return std::pair<std::vector<Vtx>, std::vector<uint16_t>>(vertices, pss);
}

template <typename Vtx>
std::pair<std::vector<Vtx>, std::vector<uint16_t>> DXTxQuad<Vtx>::CreateCubeAdjacent(float size = 1, bool rhcoords = true)
{
	// A full cube has eight vectors which 4 is forward and 4 backward
    const int FaceCount = 6;

    static const XMVECTORF32 faceNormals[FaceCount] =
    {
        {  0,  0, -1 },
        {  0,  0,  1 },
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
    };

    static const XMVECTORF32 textureCoordinates[4] =
    {
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
    };

	std::vector<Vtx> vertices;
	std::vector<uint16_t> indices;

	
    size /= 2;


    // Create each face in turn.
    for (int i = 0; i < FaceCount / 3; i++)
    {
        XMVECTOR normal = faceNormals[i];

        // Get two vectors perpendicular both to the face normal and to each other.
        XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        // Six indices (two triangles) per face.
        size_t vbase = vertices.size();
        indices.push_back(vbase + 0);
        indices.push_back(vbase + 1);
        indices.push_back(vbase + 2);

        indices.push_back(vbase + 0);
        indices.push_back(vbase + 2);
        indices.push_back(vbase + 3);

        // Four vertices per face.
        vertices.push_back(VertexPositionNormalTexture((normal - side1 - side2) * size, normal, textureCoordinates[0]));
        vertices.push_back(VertexPositionNormalTexture((normal - side1 + side2) * size, normal, textureCoordinates[1]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 + side2) * size, normal, textureCoordinates[2]));
        vertices.push_back(VertexPositionNormalTexture((normal + side1 - side2) * size, normal, textureCoordinates[3]));
    }


	

	indices.clear();


	// triangle list

	//// front
	//	indices.push_back(0);
	//	indices.push_back(1);
	//	indices.push_back(2);

	//	indices.push_back(0);
	//	indices.push_back(2);
	//	indices.push_back(3);
	////top
 //   indices.push_back(3);
 //   indices.push_back(7);
 //   indices.push_back(0);

	//indices.push_back(3);
 //   indices.push_back(4);
 //   indices.push_back(7);

	////right
	//	indices.push_back(4);
	//	indices.push_back(3);
	//	indices.push_back(2);

	//    indices.push_back(4);
	//	indices.push_back(2);
	//	indices.push_back(5);

	////// back
 //   indices.push_back(5);
 //   indices.push_back(7);
 //   indices.push_back(4);

	//indices.push_back(5);
 //   indices.push_back(6);
 //   indices.push_back(7);
	//
	//// now we have front and back indices add additional indices for top, left, bottom and right
	////// bottom
 //   indices.push_back(6);
 //   indices.push_back(5);
 //   indices.push_back(2);

	//indices.push_back(6);
 //   indices.push_back(2);
 //   indices.push_back(1);
	//
	//////left
 //   indices.push_back(1);
 //   indices.push_back(7);
 //   indices.push_back(6);

	//indices.push_back(1);
 //   indices.push_back(0);
 //   indices.push_back(7);
	//

	//if (rhcoords == false)
	//{
	//	ReverseWinding(indices, vertices);
	//}

	// triangle strip


	int index = 0;
	



	std::vector<uint16_t> adjacent;
	adjacent.resize(indices.size() * 2);

	


	// generate adjacency information
	// adjacent in closed cube is always somewhere other index

	for (int i = 0; i < indices.size()/3 ; i++)
	{
		uint16_t i0 = indices[i*3+0];
		uint16_t i1 = indices[i*3+1];
		uint16_t i2 = indices[i*3+2];
		
		adjacent[i*6 + 0] = indices[i*3+0];
		adjacent[i*6 + 1] = -1;
		adjacent[i*6 + 2] = indices[i*3+1];
		adjacent[i*6 + 3] = -1;
		adjacent[i*6 + 4] = indices[i*3+2];
		adjacent[i*6 + 5] = -1;
		
		for(uint16_t j=0; j<indices.size()/3; j++)	
		{		
			if( j != i ) // don't check a triangle against itself		
			{	
				uint16_t j0 = indices[j*3+0];
				uint16_t j1 = indices[j*3+1];
				uint16_t j2 = indices[j*3+2];
				// check for i0 and i1	
				if( j0 == i0 )	
				{
					if(j1==i1) 
						adjacent[i*6+1] = j2;
					else if(j2==i1) 
						adjacent[i*6+1] = j1;
				} else if( j1==i0 )	
				{
					if(j0==i1)
						adjacent[i*6+1] = j2;
					else if(j2==i1)
						adjacent[i*6+1] = j0;
				} else if( j2==i0 )
				{
					if(j0==i1) 
						adjacent[i*6+1] = j1;
					else if(j1==i1)
						adjacent[i*6+1] = j0;
				}
				// check for i1 and i2		
				if( j0 == i1 )	
				{
					if(j1==i2)
						adjacent[i*6+3] = j2;	
					else if(j2==i2)
						adjacent[i*6+3] = j1;	
				} 
				else if( j1==i1 )	
				{
					if(j0==i2)
						adjacent[i*6+3] = j2;
					else if(j2==i2)
						adjacent[i*6+3] = j0;	
				}
				else if( j2==i1 )
				{	
					if(j0==i2)
						adjacent[i*6+3] = j1;
					else if(j1==i2)
						adjacent[i*6+3] = j0;	
				}
				// check for i2 and i0		
				if( j0 == i2 )		
				{
					if(j1==i0)
						adjacent[i*6+5] = j2;
					else if(j2==i0)
						adjacent[i*6+5] = j1;			
				}
				else if( j1==i2 )
				{
					if(j0==i0)
						adjacent[i*6+5] = j2;
					else if(j2==i0)
						adjacent[i*6+5] = j0;	
				}
				else if( j2==i2 )	
				{
					if(j0==i0)
						adjacent[i*6+5] = j1;	
					else if(j1==i0)
						adjacent[i*6+5] = j0;
				}
			}
		}	
	}
	


	return std::pair<std::vector<Vtx>, std::vector<uint16_t>>(vertices, indices);
}



        /*float fact(int i)
        {
            float res = 1;
            for (int n = 1; n <= i; n++)
            {
                res = res * n;
            }

            return  res;
        }*/

//D3DXVECTOR2* DXTxQuad::CreatePolyOnBezier(D3DXVECTOR2* points, int count)
//        {
//
//            // created list of points which bezier will be traced
//            if (points == NULL)
//                return NULL;
//
//
//			D3DXVECTOR2* phPoints = new D3DXVECTOR2[count] ;
//            for (int i = 0; i < count; i++)
//            {
//                phPoints[i] = points[i];
//            }
//
//
//            double* cfx = new double[count];
//            double* cfy = new double[count];
//
//            int n = count - 1;
//            for (int i = 0; i <= n; i++)
//            {
//				cfx[i] = phPoints[i].x * fact(n) / (fact(i) * fact(n - i)));
//                cfy[i] = phPoints[i].y * fact(n) / (fact(i) * fact(n - i)));
//            }
//
//            double xl = phPoints[0].x - 1;
//            double yl = phPoints[0].y - 1;
//            double xp, yp;
//            double t = 0;
//            double f = 1;
//            xp = phPoints[0].x;
//            yp = phPoints[0].y;
//            bool yStart = false;
//            bool xStart = false;
//            double divCount = 0.0f;
//            double res;
//            double fct = 0.0f;
//            double x;
//            double y;
//            double j;
//            double k = 1.1f;
//            //Array to hold all points on the bezier curve
//			D3DXVECTOR2* curvePoints = new D3DXVECTOR2[count] ;
//
//            double y1, y2, x1, x2;
//            y1 = yp;
//            y2 = yp;
//            x1 = xp;
//            x2 = xp;
//
//            while (t <= 1)
//            {
//                x = 0;
//                y = 0;
//
//                for (int i = 0; i <= n; i++)
//                {
//                    fct = .Pow(1 - t, n - i) * Math.Pow(t, i);
//                    x = x + cfx[i] * fct;
//                    y = y + cfy[i] * fct;
//                }
//                x = Math.Round(x);
//                y = Math.Round(y);
//
//                if (x != xl || y != yl)
//                {
//                    if (x - xl > 1 || y - yl > 1 || xl - x > 1 || yl - y > 1)
//                    {
//                        t -= f;
//                        f = f / k;
//                    }
//                    else
//                    {
//                        curvePoints.Add(new Vector2((float)x, (float)y));
//                        xl = x;
//                        yl = y;
//                    }
//                }
//                else
//                {
//                    t -= f;
//                    f = f * k;
//                }
//                t += f;
//            }
//
//            return curvePoints;
//        }

		template <typename Vtx> void DXTxQuad<Vtx>::FillVertices()
        {
            // Fill in texture coordinates to display full texture
            // on quad

            // Provide a normal for each vertex
            for (int i = 0; i < 4 * numberX * numberY; i++)
            {
				XMStoreFloat3(&Vertices[i].normal,Normal);
            }

            short VertexInt = 0;
            short IndexInt = 0;
            

            for (int xAxis = 1; xAxis <= numberX; xAxis++)
            {
                for (int yAxis = 1; yAxis <= numberY; yAxis++)
                {
                    float xAxisNumber = Width / numberX;
                    float yAxisNumber = Height / numberY;

                    float textXNumber = 1.0f / numberX;
                    float textYNumber = 1.0f / numberY;

                    float xDistance = (xAxis - 1)* xAxisNumber;
                    float yDistance = (yAxis - 1) * yAxisNumber;
                    float xDistance1 = xAxis * xAxisNumber;
                    float yDistance1 = yAxis * yAxisNumber;

					

					XMFLOAT2 textureUpperLeft = XMFLOAT2((xAxis - 1) * textXNumber, (yAxis - 1) * textYNumber);
                    XMFLOAT2 textureUpperRight = XMFLOAT2((xAxis) * textXNumber, (yAxis - 1) * textYNumber);
                    XMFLOAT2 textureLowerLeft = XMFLOAT2((xAxis - 1) * textXNumber, (yAxis) * textYNumber);
                    XMFLOAT2 textureLowerRight = XMFLOAT2((xAxis) * textXNumber, (yAxis) * textYNumber);


					XMVECTOR UpperLeftTile = originTile - (Left * xDistance) - (Up * yDistance);
                    XMVECTOR UpperRightTile = originTile - (Left * xDistance1) - (Up * yDistance);
                    XMVECTOR LowerLeftTile = originTile - (Left * xDistance) - (Up * yDistance1);
                    XMVECTOR LowerRightTile = originTile - (Left * xDistance1) - (Up * yDistance1);


                    // vertex
					XMStoreFloat3(&Vertices[VertexInt].position,LowerLeftTile);
					Vertices[VertexInt].textureCoordinate = textureLowerLeft;
                    XMStoreFloat3(&Vertices[VertexInt + 1].position,UpperLeftTile);
                    Vertices[VertexInt + 1].textureCoordinate = textureUpperLeft;
                    XMStoreFloat3(&Vertices[VertexInt + 2].position,LowerRightTile);
                    Vertices[VertexInt + 2].textureCoordinate = textureLowerRight;
                    XMStoreFloat3(&Vertices[VertexInt + 3].position,UpperRightTile);
                    Vertices[VertexInt + 3].textureCoordinate = textureUpperRight;

                    // Set the index buffer for each vertex, using
                    // clockwise winding
                    Indexes[IndexInt] = VertexInt;
                    Indexes[IndexInt + 1] = (short)(VertexInt + 1);
                    Indexes[IndexInt + 2] = (short)(VertexInt + 2);
                    Indexes[IndexInt + 3] = (short)(VertexInt + 2);
                    Indexes[IndexInt + 4] = (short)(VertexInt + 1);
                    Indexes[IndexInt + 5] = (short)(VertexInt + 3);

					//Transforms[xAxis * yAxis - 1] = XMMatrixIdentity();
                    VertexInt += 4;
                    IndexInt += 6;
                }
            }
        }




		template DXTxQuad<VertexPositionNormalTexture>;