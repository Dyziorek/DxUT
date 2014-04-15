// DXUTWork.cpp : Defines the entry point for the application.
//


#include "stdafx.h"
#include "resource.h"
#include "DXGIDebug.h"


#include "scrnsave.h"
#include "BlurEffect.h"
#include "GeometryEffect.h"
#include "TextureElement.h"

#include <algorithm>
#include <sstream>
#ifndef _M_X64 
#include  "vsgcapture.h"
#endif
#include "DXUTcamera.h"
#include "DXUTQuad.h"
#include "DXGlyph.h"

#include "Box2d/Box2D.h"
#include "DXPlayground.h"

#ifdef __cplusplus
extern "C"{
#endif 
//DEFINE_GUID(GUID_ContainerFormatJpeg, 0x19e4a5aa, 0x5662, 0x4fc5, 0xa0, 0xc0, 0x17, 0x58, 0x02, 0x8e, 0x10, 0x57);

extern "C" const GUID DECLSPEC_SELECTANY SDXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, { 0x87, 0xe6,  0x43,  0xe9,  0xa9,  0xcf,  0xda,  0x8 } };

#ifdef __cplusplus
}
#endif 

#define MAX_LOADSTRING 100


using namespace DirectX;

// Global Variables:
bool g_grabScreen = false;
double timePart = 0.01f;
int capture = 10;
HINSTANCE hInst;								// current instance
HWND g_hWnd;
D3D_DRIVER_TYPE g_driverType;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
LARGE_INTEGER Freq;
LARGE_INTEGER cbTimeStart;
std::vector <std::unique_ptr<TextureElement>> textures;
std::vector <std::unique_ptr<GeometricPrimitive>> objects;
std::vector <std::unique_ptr<SpriteFont>> fontTable;
std::vector <IEffect*> effects;
std::unique_ptr<SpriteBatch> batch;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneTargetView;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> bloomTargetView;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
Microsoft::WRL::ComPtr<ID3D11Texture2D> texturemap1;
Microsoft::WRL::ComPtr<ID3D11Texture2D> texturemap2;
Microsoft::WRL::ComPtr<ID3D11Texture2D> stencilmap;
Microsoft::WRL::ComPtr<ID3D11Texture2D> renderstencilmap;
Microsoft::WRL::ComPtr<ID3D11Texture2D> swapBuffer;
Microsoft::WRL::ComPtr<ID3D11Texture2D> swapMap;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> stencilView;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> renderstencilView;
std::vector<VertexPositionTexture> list;
DXTxQuad<VertexPositionNormalTexture>* quad;
std::pair<std::vector<VertexPositionNormalTexture>, std::vector<uint16_t>> quadInfo;
void InitApp();
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ID3D11Device* g_pd3dDevice;
ID3D11DeviceContext* g_pd3dCtx;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
IDXGISwapChain*                     g_pSwapChain = nullptr;

DXPlayground* playGround;

CModelViewerCamera g_camera;
XMMATRIX g_World;
XMMATRIX g_View;
XMMATRIX g_Proj;

CRITICAL_SECTION physicalInterceptor;

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_DXUTWORK );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TexturePlay";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_SMALL );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    hInst = hInstance;
    RECT rc = { 0, 0 , 1680, 1050 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TexturePlay", L"DirectXTK Texture Play", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
	{
		DWORD Err = GetLastError();
        return E_FAIL;
	}

	ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_11_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pd3dCtx );

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    
	hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&swapBuffer );
    if( FAILED( hr ) )
        return hr;
	
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.ArraySize = 1;
	swapBuffer->GetDesc(&textureDesc);
	D3D11_RENDER_TARGET_VIEW_DESC tgtDesc;
	ZeroMemory(&tgtDesc, sizeof(tgtDesc));
	tgtDesc.Format = textureDesc.Format;
	tgtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	tgtDesc.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateRenderTargetView( swapBuffer.Get(), &tgtDesc, renderTargetView.GetAddressOf() );
	swapBuffer.Reset();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &stencilmap );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView( stencilmap.Get(), &descDSV, &stencilView );
    if( FAILED( hr ) )
        return hr;

	

	//g_pd3dCtx->OMSetRenderTargets( 1, renderTargetView.GetAddressOf(), renderstencilView.Get() );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3dCtx->RSSetViewports( 1, &vp );


	

	g_World = XMMatrixIdentity();
	XMVECTOR Eye = XMVectorSet( 2.0f, 5.0f, -4.0f, 0.0f );
    XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    g_View = XMMatrixLookAtLH( Eye, At, Up );
	XMFLOAT3 v3Eye, v3At;
	XMStoreFloat3(&v3Eye, Eye);
	XMStoreFloat3(&v3At, At);

     // Initialize the projection matrix
    g_Proj = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 10.0f );
	g_Proj = XMMatrixOrthographicLH(width, height, 0.1f, 10.0f);

	g_camera.Reset();
	g_camera.SetWnd(g_hWnd);
	g_camera.SetWindow(width, height);
	g_camera.SetViewParams(&v3Eye, &v3At);

	//g_camera.SetProjParams(XM_PIDIV4,  width / (FLOAT)height, 0.001f, 1000.0f);
	
	quad = new DXTxQuad<VertexPositionNormalTexture>(XMVectorZero(), At, Up, 10, 10, 10, 10);

	quadInfo = quad->CreateCube(2.0f, false);

	

	playGround = new DXPlayground(g_pd3dDevice);

	//g_camera.SetProjParams( XM_PIDIV4 / 4, width / (FLOAT)height, 2.0f, 4000.0f );
    //g_camera.SetWindow( width, height );
    //g_camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON );
    return S_OK;
}

void CleanupDevice()
{
    if( g_pd3dCtx ) g_pd3dCtx->ClearState();

	if( renderstencilView ) renderstencilView.Reset();
	if( stencilmap ) stencilmap.Reset();
	if( renderTargetView ) renderTargetView.Reset();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pd3dCtx ) g_pd3dCtx->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}

void  Render(  )
{
	PrimitiveBatch<VertexPositionTexture> lquad(g_pd3dCtx);
	

	float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };
	float ClearColor1[4] = { 0.976f, 0.196f, 0.167f, 0.0f };
	float ClearColor2[4] = { 0.176f, 0.996f, 0.167f, 0.0f };
	float ClearColor3[4] = { 0.176f, 0.196f, 0.967f, 0.0f };

    // Clear the depth stencil

    // Get the projection & view matrix from the camera class
	const XMMATRIX* mWorld = g_camera.GetWorldMatrix();
    const XMMATRIX* mView = g_camera.GetViewMatrix();
	const XMMATRIX* mProj = g_camera.GetProjMatrix();
    //const XMMATRIX mWorldViewProjection = *mWorld * *mView * *mProj;

	
	
    //g_pd3dCtx->CopyResource(swapMap.Get(), swapBuffer.Get());
	if(capture == 10 || capture == 9 || capture == 4)
	{
		#ifndef _M_X64 || _DEBUG
	 //g_pVsgDbg->CaptureCurrentFrame();
		#endif
	}
	capture--;
	if (g_grabScreen)
	{
		#ifndef _M_X64 
		g_pVsgDbg->CaptureCurrentFrame();
		#endif
	}

	XMMATRIX xmWorld = *mWorld , xmView = *mView ,xmProj = *mProj;
	
	g_pd3dCtx->ClearRenderTargetView(renderTargetView.Get(), Colors::Black);
	
//	g_pd3dCtx->ClearRenderTargetView(textures[1]->GetTargetView().Get(), Colors::Yellow);
	//g_pd3dCtx->ClearRenderTargetView(textures[2]->GetTargetView().Get(), Colors::AliceBlue);
	//g_pd3dCtx->ClearRenderTargetView(textures[3]->GetTargetView().Get(), Colors::LightCoral);
	//g_pd3dCtx->ClearRenderTargetView(textures[4]->GetTargetView().Get(), Colors::LightSkyBlue);


	ID3D11ShaderResourceView* nullSrv = NULL;
	g_pd3dCtx->PSSetShaderResources(0, 1, &nullSrv);
	// Render To texture - normal scene
	//Microsoft::WRL::ComPtr<ID3D11InputLayout> pLayout;
	//for (int idx = 0; idx < objects.size(); idx++)
	//{
	//	BasicEffect* pEffect = (BasicEffect*)effects[0];
	//	pEffect->SetWorld(xmWorld);
	//	pEffect->SetView(xmView);
	//	pEffect->SetProjection(xmProj);
	//	pEffect->SetTextureEnabled(true);
	//	pEffect->SetAlpha(1.1f);
	//	pEffect->SetFogEnabled(true);
	//	pEffect->SetFogColor(Colors::Azure);
	//	pEffect->SetFogStart(0.0f);
	//	pEffect->SetFogEnd(100.0f);
	//	pEffect->SetLightingEnabled(true);
	//	pEffect->EnableDefaultLighting();
	//	

	//	objects[idx]->CreateInputLayout(pEffect, &pLayout);

	//	pEffect->SetTexture(textures[0]->GetShaderResourceView().Get());
	//	//objects[idx]->Draw(pEffect, pLayout.Get());
	//	textures[1]->SetMatrices(xmWorld, xmView, xmProj, xmProj);
	//	textures[1]->BeginRenderTexture(g_pd3dCtx, pEffect, pEffect);
	//	// draw to texture[1] (normal)
	//	Microsoft::WRL::ComPtr<ID3D11InputLayout> Layout;
	//	objects[idx].get()->CreateInputLayout(pEffect, &Layout);
	//	objects[idx].get()->Draw(pEffect, Layout.Get());
	//	textures[1]->EndRenderTexture(g_pd3dCtx);
	//	//textures[1]->RenderTexture(g_pd3dCtx, pEffect, pEffect, objects[idx].get());

	//	pEffect->SetTexture(nullptr);
	//	pLayout.Reset();
	//}

	g_pd3dCtx->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), renderstencilView.Get());
	playGround->GetStep();


	GeometryEffect* pGeometry = (GeometryEffect*)effects[2];
	pGeometry->SetWorld(xmWorld);
	pGeometry->SetView(xmView);
	pGeometry->SetProjection(xmProj);
	pGeometry->SetTexture(textures[0]->GetShaderResourceView().Get());
	PrimitiveBatch<VertexPositionNormalTexture> batchEntry(g_pd3dCtx);
	batchEntry.Begin();
	Microsoft::WRL::ComPtr<ID3D11InputLayout> Layout;
	byte* shaderBytes;
	size_t counter;
	pGeometry->GetVertexShaderBytecode((const void**)&shaderBytes, &counter);
	/*objects[0].get()->CreateInputLayout(effects[2], &Layout);
	
	objects[0].get()->Draw(effects[2], Layout.Get());*/
	g_pd3dDevice->CreateInputLayout(VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount, shaderBytes, counter, Layout.GetAddressOf());
	g_pd3dCtx->IASetInputLayout(Layout.Get());
	pGeometry->DisableGeometry(false);
	pGeometry->Apply(g_pd3dCtx);
	batchEntry.DrawIndexed(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ, &quadInfo.second.front(), quadInfo.second.size(), &quadInfo.first.front(), quadInfo.first.size());
	//pGeometry->DisableGeometry(true);
	batchEntry.End();
	//g_pd3dCtx->GSSetShader(nullptr, nullptr, 0);
	Layout.Reset();

	g_pd3dCtx->GSSetShader(nullptr, nullptr, 0);
	std::wstringstream str;
	XMMATRIX wvp = XMMatrixMultiply(xmWorld, xmView);
	wvp = XMMatrixMultiply(wvp, xmProj);
	std::for_each(quadInfo.first.begin(), quadInfo.first.begin()+8, [&](VertexPositionNormalTexture vtx)
	{
		XMVECTOR vr = XMVector3Transform(XMLoadFloat3(&vtx.position), wvp);
		XMVECTOR vrLoc = XMVector3Transform(XMLoadFloat3(&vtx.position), xmWorld);
		vrLoc = XMVector3Transform(vrLoc, xmView);
		vrLoc = XMVector3Transform(vrLoc, xmProj);
		str << "ox:" << vtx.position.x << " oy:" << vtx.position.y << " oz:"<< vtx.position.z;
		str << "   x:" << vr.m128_f32[0] << " y:" << vr.m128_f32[1] << " z:"<< vr.m128_f32[2];
		str << "   xt:" << vrLoc.m128_f32[0] << " yt:" << vrLoc.m128_f32[1] << " zt:"<< vrLoc.m128_f32[2] << std::endl;
	});

	str.str(std::wstring());
	if (WaitForSingleObject(playGround->drawEvent, 0) == WAIT_OBJECT_0)
	{
	//str<<"b1 x" << playGround->GetPosBlock(playGround->GetCount() - 2).x << " b1 y" << playGround->GetPosBlock(playGround->GetCount() - 2).y << std::endl;
	str<<"b1 posx" << playGround->GetScreenPosBlockEx(playGround->GetCount() - 2).x << " b1 posy" << playGround->GetScreenPosBlockEx(playGround->GetCount() - 2).y << std::endl;
	}
		batch.get()->Begin();
		XMVECTOR position = XMVectorSplatOne()*10.f;
		fontTable[0]->DrawString(batch.get(),str.str().c_str(), position, Colors::DimGray, 0.0f, XMVectorZero(), XMVectorSplatOne()*0.5f);
		XMVECTOR moveDown = XMVectorSet(0,20,0,0);
		//fontTable[0]->DrawString(batch.get(),, position+=moveDown, Colors::Yellow, 0.0f, XMVectorZero(), XMVectorSplatOne()*0.7f);
		//fontTable[0]->DrawString(batch.get(),, position+=moveDown, Colors::Yellow, 0.0f, XMVectorZero(), XMVectorSplatOne()*0.7f);
		batch.get()->End();

		std::vector<KeyFrame> vecKeyFrames;
	
			vecKeyFrames.push_back(KeyFrame(500,350, 0.001, 0, 0));
	//vecKeyFrames.push_back(KeyFrame(500,350, 0.1, 0, 1));
	vecKeyFrames.push_back(KeyFrame(500,350, 2.0, 0, 2));
	//vecKeyFrames.push_back(KeyFrame(600,250, 0.8, 0, 3));
	vecKeyFrames.push_back(KeyFrame(700,150, 0.4, 0, 4));
	vecKeyFrames.push_back(KeyFrame(800,50, 0.1, 0, 5));

	vecKeyFrames.push_back(KeyFrame(100,350, 0.001, 0, 5));
	//vecKeyFrames.push_back(KeyFrame(500,350, 0.1, 0, 1));
	vecKeyFrames.push_back(KeyFrame(100,350, 2.0, 0, 7));
	//vecKeyFrames.push_back(KeyFrame(600,250, 0.8, 0, 3));
	vecKeyFrames.push_back(KeyFrame(700,150, 0.4, 0, 9));
	vecKeyFrames.push_back(KeyFrame(800,50, 0.1, 0, 10));

	vecKeyFrames.push_back(KeyFrame(200,350, 0.001, 0, 10));
	//vecKeyFrames.push_back(KeyFrame(500,350, 0.1, 0, 1));
	vecKeyFrames.push_back(KeyFrame(200,350, 2.0, 0, 12));
	//vecKeyFrames.push_back(KeyFrame(600,250, 0.8, 0, 3));
	vecKeyFrames.push_back(KeyFrame(700,150, 0.4, 0, 14));
	vecKeyFrames.push_back(KeyFrame(800,50, 0.1, 0, 15));

	vecKeyFrames.push_back(KeyFrame(300,350, 0.001, 0, 15));
	//vecKeyFrames.push_back(KeyFrame(500,350, 0.1, 0, 1));
	vecKeyFrames.push_back(KeyFrame(300,350, 2.0, 0, 17));
	//vecKeyFrames.push_back(KeyFrame(600,250, 0.8, 0, 3));
	vecKeyFrames.push_back(KeyFrame(700,150, 0.4, 0, 19));
	vecKeyFrames.push_back(KeyFrame(800,50, 0.1, 0, 20));

	

	SpriteBatch batchWork(g_pd3dCtx);
	PrimitiveBatch<VertexPositionTexture> primitiveBatch(g_pd3dCtx, 65536, 32768);
	if (WaitForSingleObject(playGround->drawEvent, INFINITE) == WAIT_OBJECT_0)
	{
		int loadConunter = playGround->GetCount();
		//_ASSERT(!(playGround->rows * playGround->cols > counter + 5));
		for (int glyphNum = 0; glyphNum < loadConunter; glyphNum++)
		{
			DXGlyph* drawGlyph = playGround->GetGlyph(glyphNum);
			if (drawGlyph->get_circle() != 0.0f)
			{
				drawGlyph->DrawGlyph(g_pd3dCtx, primitiveBatch, playGround->GetScreenPosBlockEx(glyphNum), playGround->GetGlyph(glyphNum)->m_glyph, 1.0f, playGround->GetScreenAngleBlockEx(glyphNum), 0.0f, false);
			}
			else
			{
				drawGlyph->DrawGlyph(g_pd3dCtx, batchWork, playGround->GetScreenPosBlockEx(glyphNum), playGround->GetGlyph(glyphNum)->m_glyph, 1.0f, playGround->GetScreenAngleBlockEx(glyphNum),0.0f, false);
			}
		}
		ResetEvent(playGround->drawEvent);
		SetEvent(playGround->physicsEvent);		
	}

		/*DXGlyph glyper0(g_pd3dDevice);
		DXGlyph glyperA(g_pd3dDevice);
		DXGlyph glyperB(g_pd3dDevice);
		DXGlyph glyperC(g_pd3dDevice);
		glyper0.Initialize( textures[1].get(),std::vector<KeyFrame>(vecKeyFrames.begin(), vecKeyFrames.begin()+5));
		glyperA.Initialize( textures[1].get(),std::vector<KeyFrame>(vecKeyFrames.begin()+5, vecKeyFrames.begin()+10));
		glyperB.Initialize( textures[1].get(),std::vector<KeyFrame>(vecKeyFrames.begin()+10, vecKeyFrames.begin()+15));
		glyperC.Initialize( textures[1].get(),std::vector<KeyFrame>(vecKeyFrames.begin()+15, vecKeyFrames.end()));

		glyper0.DrawGlyph(g_pd3dCtx,batchWork, '0', ((int)(timePart) % 20) + (timePart - floor(timePart)) );
		glyperA.DrawGlyph(g_pd3dCtx,batchWork, 'A', ((int)(timePart) % 20) + (timePart - floor(timePart)) );
		glyperB.DrawGlyph(g_pd3dCtx,batchWork, 'B', ((int)(timePart) % 20) + (timePart - floor(timePart)) );
		glyperC.DrawGlyph(g_pd3dCtx,batchWork, 'C', ((int)(timePart) % 20) + (timePart - floor(timePart)) );*/

		//DXGlyph glyper(g_pd3dDevice);
		//glyper.Initialize( textures[1].get(),std::vector<KeyFrame>(vecKeyFrames.begin(), vecKeyFrames.begin()+5));
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(180.1f, 100.5f + (int)(timePart * 80) % 700), ((int)(timePart) % 35) + '0', 0.5f, DirectX::XM_PIDIV4 * timePart, 0.1 );
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(260.1f, 100.5f + (int)(timePart * 50) % 700),  ((int)(timePart) % 35) +'0', 0.5f, DirectX::XM_PIDIV2 * timePart, 0);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(340.1f, 100.5f + (int)(timePart * 70) % 700),  ((int)(timePart*1.3) % 35) +'0', 0.8f,DirectX::XM_PIDIV4 * timePart, 0.4);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(420.1f, 100.5f + (int)(timePart * 10) % 700),  ((int)(timePart*1.4) % 35) +'0', 0.8f, DirectX::XM_PIDIV2 * timePart);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(500.1f, 100.5f + (int)(timePart * 90) % 700),  ((int)(timePart*1.5) % 35) +'0', 0.8f, DirectX::XM_PIDIV2 * timePart);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(580.1f, 100.5f + (int)(timePart * 76) % 700),  ((int)(timePart*1.6) % 35) +'0', 0.4f, DirectX::XM_PIDIV2 * timePart);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(660.1f, 100.5f + (int)(timePart * 25) % 700),  ((int)(timePart*1.7) % 35) +'0', 0.4f, DirectX::XM_PIDIV2 * timePart);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(740.1f, 100.5f + (int)(timePart * 55) % 700),  ((int)(timePart*1.8) % 35) +'0', 0.4f, DirectX::XM_PIDIV2 * timePart);
		//glyper.DrawGlyph(g_pd3dCtx,batchWork, XMFLOAT2(820.1f, 100.5f + (int)(timePart * 66) % 700),  ((int)(timePart*1.9) % 35) +'0', 0.4f, DirectX::XM_PIDIV2 * timePart);
	//BlurEffect* blur = (BlurEffect*)effects[1];
	//blur->SetPSShader(PSShaderType::BloomExtract);
	////textures[2]->SetMatrices(XMMatrixIdentity(), XMMatrixIdentity(), XMMatrixIdentity(), XMMatrixIdentity());
	//blur->SetMatrices(XMMatrixIdentity(), XMMatrixIdentity(), XMMatrixIdentity());
	//blur->SetData(4.0f, XMFLOAT2(0.0f, 0.0f), 0,0,0,0.0f);
	//blur->SetTexture(textures[1]->GetShaderResourceView().Get());

	//if (g_grabScreen)
	//{
	//	D3D11_TEXTURE2D_DESC texDesc;
	//	Microsoft::WRL::ComPtr<ID3D11Resource> cResource;
	//	Microsoft::WRL::ComPtr<ID3D11Texture2D> texLocal;
	//	textures[1]->GetTargetView().Get()->GetResource(cResource.GetAddressOf());
	//	cResource.As(&texLocal);
	//	texLocal->GetDesc(&texDesc);
	//	SaveWICTextureToFile(g_pd3dCtx, texLocal.Get() , GUID_ContainerFormatJpeg, L"TextureOutpLoc.jpg", nullptr);
	//}

	////objects[0]->CreateInputLayout(blur, pLayout.GetAddressOf());
	////objects[0]->Draw(blur, pLayout.Get());
	//textures[2]->SetMatrices(XMMatrixIdentity(), XMMatrixIdentity(), XMMatrixIdentity(), XMMatrixIdentity());
	//// extract data from tex[1] to tex[2]
	//textures[2]->BeginRenderTexture(g_pd3dCtx, blur, blur);
 //   lquad.Begin();
	//textures[2]->ApplyShaders(g_pd3dCtx, blur, blur);
	//lquad.DrawQuad(list[0],list[1],list[2], list[3]);
	//lquad.End();
	//textures[2]->EndRenderTexture(g_pd3dCtx);
	//if (g_grabScreen)
	//{
	//	D3D11_TEXTURE2D_DESC texDesc;
	//	Microsoft::WRL::ComPtr<ID3D11Resource> cResource;
	//	Microsoft::WRL::ComPtr<ID3D11Texture2D> texLocal;
	//	textures[2]->GetTargetView().Get()->GetResource(cResource.GetAddressOf());
	//	cResource.As(&texLocal);
	//	texLocal->GetDesc(&texDesc);
	//	SaveWICTextureToFile(g_pd3dCtx, texLocal.Get() , GUID_ContainerFormatJpeg, L"TextureOutp2Loc.jpg", nullptr);
	//}
	//blur->SetPSShader(PSShaderType::GauussBlur);
	//D3D11_TEXTURE2D_DESC texDesc;

	//if (g_grabScreen && sceneTargetView.Get() != nullptr)
	//{
	//	D3D11_RENDER_TARGET_VIEW_DESC desc;
	//	Microsoft::WRL::ComPtr<ID3D11Resource> cResource;
	//	Microsoft::WRL::ComPtr<ID3D11Texture2D> texLocal;
	//	sceneTargetView->GetResource(cResource.GetAddressOf());
	//	cResource.As(&texLocal);
	//	texLocal->GetDesc(&texDesc);
	//	SaveWICTextureToFile(g_pd3dCtx, texLocal.Get() , GUID_ContainerFormatJpeg, L"TextureOutLoc.jpg", nullptr);
	//}
	//ID3D11Texture2D* textInfo = nullptr;
	//textures[3]->GetTextureData(&texDesc, &textInfo);
	//blur->SetData(4.1f, XMFLOAT2(0.0f,1.0f/texDesc.Width), 2.0f,2.0f,2.0f,0.25f);
	//blur->SetTexture(textures[2]->GetShaderResourceView().Get());
	//// blur on tex[2] to tex[3]
	//textures[3]->BeginRenderTexture(g_pd3dCtx, blur, blur);
 //   lquad.Begin();
	//textures[3]->ApplyShaders(g_pd3dCtx, blur, blur);
	//lquad.DrawQuad(list[0],list[1],list[2], list[3]);
	//lquad.End();
	//textures[3]->EndRenderTexture(g_pd3dCtx);
	//blur->SetData(4.1f, XMFLOAT2(1.0f/texDesc.Width,0.0f), 2.0f,2.0f,2.0f,0.25f);

	//if (g_grabScreen)
	//{
	//	SaveWICTextureToFile(g_pd3dCtx, textInfo , GUID_ContainerFormatJpeg, L"TextureOutLocBlur.jpg", nullptr);
	//}
	//
	//blur->SetTexture(textures[3]->GetShaderResourceView().Get(), textures[3]->GetShaderResourceView().Get());
	//// combine on tex[2] to tex[3]
	//textures[4]->BeginRenderTexture(g_pd3dCtx, blur, blur);
 //   lquad.Begin();
	//textures[4]->ApplyShaders(g_pd3dCtx, blur, blur);
	//lquad.DrawQuad(list[0],list[1],list[2], list[3]);
	//lquad.End();
	//textures[4]->EndRenderTexture(g_pd3dCtx);

	//blur->SetPSShader(PSShaderType::BloomCombine);
	//blur->SetTexture(textures[1]->GetShaderResourceView().Get(), textures[3]->GetShaderResourceView().Get());

	//g_pd3dCtx->OMSetRenderTargets( 1, renderTargetView.GetAddressOf(), renderstencilView.Get() );
	//
	//lquad.Begin();
	//textures[4]->ApplyShaders(g_pd3dCtx, blur, blur);
	//lquad.DrawQuad(list[0],list[1],list[2], list[3]);
	//lquad.End();
	////blur->SetTexture(textures[2]->GetShaderResourceView().Get());
	////objects[0]->Draw(blur, pLayout.Get());


	////g_pd3dCtx->OMSetRenderTargets(1, sceneTargetView.GetAddressOf(), stencilView.Get());
	////
 //   
	////blur->SetTexture(textures[1]->GetShaderResourceView().Get());
	////objects[0]->Draw(blur, pLayout.Get());
	////g_pd3dCtx->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), renderstencilView.Get());
	////
	////blur->SetPSShader(PSShaderType::BloomCombine);
	////blur->SetData(4.0f, XMFLOAT2(0.0f,1.0f/texDesc.Width), 1.0f,1.0f,1.0f,0.25f);
	////blur->SetTexture(textures[2]->GetShaderResourceView().Get());
	////objects[0]->Draw(blur, pLayout.Get());
	////pLayout.Reset();

	if (g_grabScreen)
	{
		
		SaveWICTextureToFile(g_pd3dCtx, swapMap.Get() , GUID_ContainerFormatJpeg, L"TextureOut.jpg", nullptr);
		SaveWICTextureToFile(g_pd3dCtx, textures[1]->GetTexture().Get() , GUID_ContainerFormatJpeg, L"TextureOutGL.jpg", nullptr);
		//SaveWICTextureToFile(g_pd3dCtx, textures[4]->GetTexture().Get() , GUID_ContainerFormatJpeg, L"TextureOut4.jpg", nullptr);
		//SaveWICTextureToFile(g_pd3dCtx, texturemap2.Get() , GUID_ContainerFormatJpeg, L"TextureOut2.jpg", nullptr);
		//SaveWICTextureToFile(g_pd3dCtx, swapBuffer.Get() , GUID_ContainerFormatJpeg, L"TextureOut3.jpg", nullptr);
		//SaveWICTextureToFile(g_pd3dCtx, stencilmap.Get() , GUID_ContainerFormatJpeg, L"TextureOut4.jpg", nullptr);
		g_grabScreen = false;
	}


	
	//g_pd3dCtx->OMSetRenderTargets(1, sceneTargetView.GetAddressOf(), stencilView.Get());

	g_pSwapChain->Present(0,0);
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	//UNREFERENCED_PARAMETER(lpCmdLine);
	HRESULT hr;
	    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	
    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

	LARGE_INTEGER Freq;
	bool resetSwitch = false;
	QueryPerformanceFrequency(&Freq);
	QueryPerformanceCounter(&cbTimeStart);
	
    InitApp();
	// Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
			LARGE_INTEGER step;
			QueryPerformanceCounter(&step);
			LARGE_INTEGER period;
			period.QuadPart = step.QuadPart-cbTimeStart.QuadPart;
			double periodPart = period.QuadPart;
			double freqPart = Freq.QuadPart;
			if (periodPart/freqPart > 0.033)
			{
				g_camera.FrameMove( 0.033f );
				timePart+= 0.016f;
				Render();
				if ((int)timePart % 10 == 9)
				{
					if (!resetSwitch)
					{
						playGround->Reset();
						resetSwitch = !resetSwitch;
					}
				}
				if ((int)timePart % 10 == 0)
				{
					if (resetSwitch)
					{
						resetSwitch = !resetSwitch;
					}
				}
				cbTimeStart=step;
			}
        }
    }

	return 0;
}

BOOL CALLBACK EnumChildProc(
  _In_  HWND hwnd,
  _In_  LPARAM lParam
)
{
	wchar_t StrStore[100];

	GetWindowText(hwnd, StrStore, 100);

	if (wcsncmp(StrStore, L"FolderView", sizeof(L"FolderView")) == 0)
	{
		*(HWND*)lParam = hwnd;
		return false;
	}
	
	return true;
}


LPVOID GetDesktopImage(PUINT cSize)
{

	HWND hDesktop = GetDesktopWindow();

	HWND hChild = NULL;

	if (*cSize == 1)
	{

		EnumChildWindows(hDesktop, EnumChildProc, (LPARAM)&hChild);

		if (hChild == NULL)
		{
			return nullptr;
		}
	}
	else
	{
		hChild = hDesktop;
	}

	HDC hWndDC = GetDC(hChild);
	HDC hMemDC = CreateCompatibleDC(hWndDC);
	int dx = GetSystemMetrics(SM_CXSCREEN);
	int dy = GetSystemMetrics(SM_CYSCREEN);
	HBITMAP hbm = CreateCompatibleBitmap(hWndDC, dx, dy);

	HGDIOBJ hOldBmp = SelectObject(hMemDC, hbm);
	BOOL bitOp = true;
	bitOp = BitBlt(hMemDC, 0, 0, dx, dy, hWndDC, 0,0,SRCCOPY);

	if (bitOp == false)
	{
		DWORD err = GetLastError();
		OutputDebugString(L"errr");
	}

	BITMAP bmpScreen;
	GetObject(hbm, sizeof(BITMAP),&bmpScreen);
	BITMAPFILEHEADER   bmfHeader;    
    BITMAPINFOHEADER   bi;
     

    bi.biSize = sizeof(BITMAPINFOHEADER);    
    bi.biWidth = bmpScreen.bmWidth;    
    bi.biHeight = bmpScreen.bmHeight;  
    bi.biPlanes = 1;    
    bi.biBitCount = 32;    
    bi.biCompression = BI_RGB;    
    bi.biSizeImage = 0;  
    bi.biXPelsPerMeter = 0;    
    bi.biYPelsPerMeter = 0;    
    bi.biClrUsed = 0;    
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
	

	byte* lpbitmap  = (byte*)HeapAlloc(GetProcessHeap(), 0, dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

	void* bitmapData = lpbitmap +  sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	int dibScans = GetDIBits(hWndDC, hbm, 0,
        (UINT)bmpScreen.bmHeight,
        bitmapData,
        (BITMAPINFO *)&bi, DIB_RGB_COLORS);

	*cSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	SelectObject(hMemDC, hOldBmp);
	DeleteDC(hMemDC);
	ReleaseDC(hChild, hWndDC);

	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmfHeader.bfSize = *cSize;
	bmfHeader.bfType = 0x4D42;
	memcpy_s(lpbitmap, *cSize, &bmfHeader, sizeof(BITMAPFILEHEADER));
	memcpy_s(lpbitmap + sizeof(BITMAPFILEHEADER), *cSize, &bi, sizeof(BITMAPINFOHEADER));

	return lpbitmap;

}

void InitApp()
{
	UINT cbSize = 1;
	void* pImage = GetDesktopImage(&cbSize);
	HRESULT hr = E_FAIL;
	
	if (pImage == nullptr)
	{
		cbSize = 0;
		void* pImage = GetDesktopImage(&cbSize);
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureData;
	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	hr = CreateWICTextureFromMemory(g_pd3dDevice, g_pd3dCtx, (byte*)pImage, cbSize, nullptr,&textureData);
	if (SUCCEEDED(hr))
	{
		textureData->GetDesc(&Desc);
		textures.push_back(std::unique_ptr<TextureElement>(new TextureElement()));
		textures.back()->SetShaderResourceView(textureData);
		textures.back()->SetPrivateName("DesktopTexture");
	}
	else
	{
		HMODULE hMod = (HMODULE)GetModuleHandle(NULL);
		HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(IDR_BINARY1), L"BINARY");
		HGLOBAL hResData = LoadResource(hMod, hRes);
		DWORD dwSize = SizeofResource(hMod,hRes);
		if (dwSize > 0)
		{
			void* pResBin = LockResource(hResData);
			hr = CreateWICTextureFromMemory(g_pd3dDevice, g_pd3dCtx, (byte*)pResBin, dwSize, nullptr,&textureData);
			if (SUCCEEDED(hr))
			{
				textures.push_back(std::unique_ptr<TextureElement>(new TextureElement()));
				textures.back()->SetShaderResourceView(textureData);
			}
		}
	}

	D3D11_TEXTURE2D_DESC desc;

	HKEY Soft;


	TextureElement* pTextureElemWIC = new TextureElement();
	CreateWICTextureFromFile(g_pd3dDevice, g_pd3dCtx, L"C:\\Windows\\Web\\Wallpaper\\Windows\\img0.jpg", (ID3D11Resource**)pTextureElemWIC->GetTexture().GetAddressOf(), pTextureElemWIC->GetShaderResourceView().GetAddressOf());
	textures.push_back(std::unique_ptr<TextureElement>(pTextureElemWIC));
	textures.back()->SetShaderResourceView(textures.back()->GetShaderResourceView());
	textures.back()->SetPrivateName("WallPaper"); 
	TextureElement* pTextureElem = new TextureElement();
	CreateDDSTextureFromFile(g_pd3dDevice, L"Output.dds", (ID3D11Resource**)pTextureElem->GetTexture().GetAddressOf(), pTextureElem->GetShaderResourceView().GetAddressOf());
	textures.push_back(std::unique_ptr<TextureElement>(pTextureElem));
	textures.back()->SetShaderResourceView(textures.back()->GetShaderResourceView());
	textures.back()->SetPrivateName("SWChevrons"); 
	//pTextureElem = new TextureElement();
	//CreateDDSTextureFromFile(g_pd3dDevice, L"1Output.dds", (ID3D11Resource**)pTextureElem->GetTexture().GetAddressOf(), pTextureElem->GetShaderResourceView().GetAddressOf());

	//textures.push_back(std::unique_ptr<TextureElement>(pTextureElem));
	//textures.back()->SetPrivateName("SWChevronTwo"); 
	//pTextureElem = new TextureElement();
	//CreateDDSTextureFromFile(g_pd3dDevice, L"2Output.dds", (ID3D11Resource**)pTextureElem->GetTexture().GetAddressOf(), pTextureElem->GetShaderResourceView().GetAddressOf());

	//textures.push_back(std::unique_ptr<TextureElement>(pTextureElem));
	//textures.back()->SetPrivateName("SWChevronThree"); 
	//textures.back()->Initialize(g_pd3dDevice, 512, 324, 0.1f, 1000.0f, true);	
	//textures.back()->SetPrivateName("BlurBase"); // 1
	//textures.push_back(std::unique_ptr<TextureElement>(new TextureElement()));
	//textures.back()->Initialize(g_pd3dDevice, 512, 324, 0.1f, 1000.0f, false);	
	//textures.back()->SetPrivateName("BlurExtract"); // 2
	//textures.push_back(std::unique_ptr<TextureElement>(new TextureElement()));
	//textures.back()->Initialize(g_pd3dDevice, 512, 324, 0.1f, 1000.0f, false);	
	//textures.back()->SetPrivateName("BlurGauss"); // 3
	//textures.push_back(std::unique_ptr<TextureElement>(new TextureElement()));
	//textures.back()->Initialize(g_pd3dDevice, 1024, 768, 0.1f, 1000.0f, false);	
	//textures.back()->SetPrivateName("BlurCombine"); //4


	

	fontTable.push_back(std::unique_ptr<SpriteFont>(new SpriteFont(g_pd3dDevice, L"Arial.spf")));
	//fontTable.push_back(std::unique_ptr<SpriteFont>(new SpriteFont(g_pd3dDevice, L"Courier.sft")));
	//fontTable.push_back(std::unique_ptr<SpriteFont>(new SpriteFont(g_pd3dDevice, L"Times.sft")));

	objects.push_back(GeometricPrimitive::CreateCube(g_pd3dCtx, 3.0f, false));
	
	effects.push_back((IEffect*)new BasicEffect(g_pd3dDevice));
	batch.reset(new SpriteBatch(g_pd3dCtx));
	effects.push_back((IEffect*)new BlurEffect(g_pd3dDevice));
	effects.push_back((IEffect*)new GeometryEffect(g_pd3dDevice));
	
	list.push_back(VertexPositionTexture(XMVectorSet(-1, -1, 0,0), XMVectorSet(0,1,0,0)));
	list.push_back(VertexPositionTexture(XMVectorSet(-1, 1, 0,0), XMVectorSet(0,0,0,0)));
	list.push_back(VertexPositionTexture(XMVectorSet(1, 1, 0,0), XMVectorSet(1,0,0,0)));
	list.push_back(VertexPositionTexture(XMVectorSet(1, -1, 0,0), XMVectorSet(1,1,0,0)));

	    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    playGround->SetView(width, height, g_pd3dCtx);
	playGround->CreateBlock(10, 10,  textures[1].get());
	playGround->SetView(width, height, g_pd3dCtx);

	

}



HRESULT CALLBACK OnDeviceCreated( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr;

	g_pd3dDevice = pd3dDevice;

	ID3D11DeviceContext* pContext = g_pd3dCtx;
	
	
	 //V_RETURN( g_pResMan->OnD3D11CreateDevice( pd3dDevice, pContext ) );
    //V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
	//V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
 //                               OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
 //                               L"Arial", &g_pFont10 ) );
	//V_RETURN( D3DX10CreateSprite( pd3dDevice, 512, &g_pSprite10 ) );
	
	
//    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
//#if defined( DEBUG ) || defined( _DEBUG )
//    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
//    // Setting this flag improves the shader debugging experience, but still allows 
//    // the shaders to be optimized and to run exactly the way they will run in 
//    // the release configuration of this program.
//    dwShaderFlags |= D3DCOMPILE_DEBUG;
//#endif
//	WCHAR str[MAX_PATH]; 
//	DWORD res = GetEnvironmentVariable(L"DXSDK_DIR", str, MAX_PATH);
//	wcscat_s(str, L"Samples\\Media");
//	DXUTSetMediaSearchPath(str);

	//V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"SimpleSample.hlsl" ) );

	//ID3DBlob* pVertexShaderBuffer = NULL;
	//ID3DBlob* pPixelShaderBuffer = NULL;
	//V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "RenderSceneVS", "vs_4_1", dwShaderFlags, 0, NULL,
 //                                    &pVertexShaderBuffer, NULL, NULL ) );

 //  // ID3DBlob* pPixelShaderBuffer = NULL;
 //   V_RETURN( D3DX11CompileFromFile( str, NULL, NULL, "RenderScenePS", "ps_4_1", dwShaderFlags, 0, NULL,
 //                                    &pPixelShaderBuffer, NULL, NULL ) );

	//V_RETURN( pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(),
 //                                             pVertexShaderBuffer->GetBufferSize(), NULL, &g_pVertexShader11 ) );
 //   DXUT_SetDebugName( g_pVertexShader11, "RenderSceneVS" );

 //   V_RETURN( pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
 //                                            pPixelShaderBuffer->GetBufferSize(), NULL, &g_pPixelShader11 ) );
 //   DXUT_SetDebugName( g_pPixelShader11, "RenderScenePS" );


	//const D3D11_INPUT_ELEMENT_DESC layout[] =
 //   {
 //       { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
 //       { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
 //       { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
 //   };
	//V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVertexShaderBuffer->GetBufferPointer(),
 //                                            pVertexShaderBuffer->GetBufferSize(), &g_pLayout11 ) );
 //   DXUT_SetDebugName( g_pLayout11, "Primary" );

 //   // No longer need the shader blobs
 //   SAFE_RELEASE( pVertexShaderBuffer );
 //   SAFE_RELEASE( pPixelShaderBuffer );
	//// Create state objects
 //   D3D11_SAMPLER_DESC samDesc;
 //   ZeroMemory( &samDesc, sizeof(samDesc) );
 //   samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
 //   samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
 //   samDesc.MaxAnisotropy = 1;
 //   samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
 //   samDesc.MaxLOD = D3D11_FLOAT32_MAX;
 //   V_RETURN( pd3dDevice->CreateSamplerState( &samDesc, &g_pSamLinear ) );
 //   DXUT_SetDebugName( g_pSamLinear, "Linear" );

 //   // Create constant buffers
 //   D3D11_BUFFER_DESC cbDesc;
 //   ZeroMemory( &cbDesc, sizeof(cbDesc) );
 //   cbDesc.Usage = D3D11_USAGE_DYNAMIC;
 //   cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
 //   cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

 //   cbDesc.ByteWidth = sizeof( CB_VS_PER_OBJECT );
 //   V_RETURN( pd3dDevice->CreateBuffer( &cbDesc, NULL, &g_pcbVSPerObject11 ) );
 //   DXUT_SetDebugName( g_pcbVSPerObject11, "CB_VS_PER_OBJECT" );



 //   cbDesc.ByteWidth = sizeof( CB_VS_PER_FRAME );
 //   V_RETURN( pd3dDevice->CreateBuffer( &cbDesc, NULL, &g_pcbVSPerFrame11 ) );
 //   DXUT_SetDebugName( g_pcbVSPerFrame11, "CB_VS_PER_FRAME" );
	
	//XMVECTOR vecEye = XMVectorSet( 0.0f, 0.0f, -10.0f );
 //   XMVECTOR vecAt = XMVectorSet( 0.0f, 0.0f, -0.0f );
	//XMVECTOR vecUP = XMVectorSet( 0.0f, 1.0f, 0.0f);
	//XMMATRIX mRotation = XMMatrixLookAtLH(vecEye, vecAt, vecUP);
	//
	//XMVECTOR mQuat = XMQuaternionRotationMatrix(mRotation);
 //   g_Camera.SetViewParams( &vecEye, &vecAt );
	//g_Camera.SetProjParams( XM_PI/4, 1.0f, 1.0f, 100.0f);
	return S_OK;
}

void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	//g_pTxtHelper->DrawTextLine(L"Created in DX10 Devices");
	g_camera.FrameMove( fElapsedTime );
}


LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    //// Pass messages to dialog resource manager calls so GUI state is updated correctly
	/**pbNoFurtherProcessing = g_pResMan->MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;*/

    //// Pass messages to settings dialog if its active
    //if( g_D3DSettingsDlg.IsActive() )
    //{
    //    g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
    //    return 0;
    //}

    //// Give the dialogs a chance to handle the message first
    //*pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    //if( *pbNoFurtherProcessing )
    //    return 0;
    //*pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    //if( *pbNoFurtherProcessing )
    //    return 0;

    //g_LightControl[g_nActiveLight].HandleMessages( hWnd, uMsg, wParam, lParam );

    //// Pass all remaining windows messages to camera so it can respond to user input
    //g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

	if (uMsg == WM_KEYDOWN && wParam == VK_SPACE)
	{
		g_grabScreen = true;
	}
	
    return 0;
}



typedef HRESULT  (WINAPI *dxGiDebugProc)(  REFIID riid,  void **ppDebug);

void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	 HMODULE hModule;
   void* procFun=NULL;
	dxGiDebugProc dxGiDebugProcessor = NULL;
	IDXGIDebug* pDxGIDebugger;
    
    // Delete additional render resources here...
	textures.clear();
	objects.pop_back();
	fontTable.clear();
	batch.reset(nullptr);
	effects.clear();

	hModule = GetModuleHandle(L"dxgidebug.dll");
		 procFun = GetProcAddress(hModule, "DXGIGetDebugInterface");
		 dxGiDebugProcessor = (dxGiDebugProc)procFun;
		
		dxGiDebugProcessor(__uuidof(IDXGIDebug), (void**)&pDxGIDebugger);
		pDxGIDebugger->ReportLiveObjects(SDXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL); 

}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
   HMODULE hModule;
   void* procFun=NULL;
	dxGiDebugProc dxGiDebugProcessor = NULL;
	IDXGIDebug* pDxGIDebugger;

	g_camera.HandleMessages( hWnd, message, wParam, lParam );

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			g_grabScreen = true;
		}
		if (wParam == VK_SPACE)
		{
			playGround->triggered = true;
		}
		break;
	case WM_DESTROY:
		hModule = GetModuleHandle(L"dxgidebug.dll");
		 procFun = GetProcAddress(hModule, "DXGIGetDebugInterface");
		 dxGiDebugProcessor = (dxGiDebugProc)procFun;
		
		dxGiDebugProcessor(__uuidof(IDXGIDebug), (void**)&pDxGIDebugger);
		pDxGIDebugger->ReportLiveObjects(SDXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL); 

		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

