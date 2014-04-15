// DXUTWork.cpp : Defines the entry point for the application.
//
#include "DXUTWork.h"
#include "DXGIDebug.h"

#include "WicTextureLoader.h"
#include "Wincodec.h"
//DirectXTK library helpers
#include "GeometricPrimitive.h"
#include "Effects.h"
#include "ScreenGrab.h"
#include "SpriteFont.h"

#include <vector>
#include <wrl.h>

#include "scrnsave.h"
#include "BlurEffect.h"



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
HINSTANCE hInst;								// current instance
HWND g_hWnd;
D3D_DRIVER_TYPE g_driverType;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
LARGE_INTEGER Freq;
LARGE_INTEGER cbTimeStart;
std::vector <Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;
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

void InitApp();
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ID3D11Device* g_pd3dDevice;
ID3D11DeviceContext* g_pd3dCtx;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
IDXGISwapChain*                     g_pSwapChain = nullptr;




XMMATRIX g_World;
XMMATRIX g_View;
XMMATRIX g_Proj;

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
    RECT rc = { 0, 0 , 1024, 768 };
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
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
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

    hr = g_pd3dDevice->CreateRenderTargetView( swapBuffer.Get(), nullptr, &renderTargetView );
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

	

	g_pd3dCtx->OMSetRenderTargets( 1, &renderTargetView, renderstencilView.Get() );

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
	XMVECTOR Eye = XMVectorSet( 0.0f, 3.0f, -6.0f, 0.0f );
    XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    g_View = XMMatrixLookAtLH( Eye, At, Up );

     // Initialize the projection matrix
    g_Proj = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );


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

	

	float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };
	float ClearColor1[4] = { 0.976f, 0.196f, 0.167f, 0.0f };
	float ClearColor2[4] = { 0.176f, 0.996f, 0.167f, 0.0f };
	float ClearColor3[4] = { 0.176f, 0.196f, 0.967f, 0.0f };

    // Clear the depth stencil

    // Get the projection & view matrix from the camera class
    XMMATRIX mWorld = g_World;
    XMMATRIX mView = g_View;
    XMMATRIX mProj = g_Proj;
    XMMATRIX mWorldViewProjection = mWorld * mView * mProj;

	
    // Set the constant buffers
    //HRESULT hr;
    //D3D11_MAPPED_SUBRESOURCE MappedResource;
    //V( g_pd3dCtx->Map( g_pcbVSPerFrame11, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    //CB_VS_PER_FRAME* pVSPerFrame = ( CB_VS_PER_FRAME* )MappedResource.pData;
    //pVSPerFrame->m_vLightDir = D3DXVECTOR3( 0,0.707f,-0.707f );
    //pVSPerFrame->m_fTime = (float)fTime;
    //pVSPerFrame->m_LightDiffuse = D3DXVECTOR4( 1.f, 1.f, 1.f, 1.f );
    //g_pd3dCtx->Unmap( g_pcbVSPerFrame11, 0 );
    //g_pd3dCtx->VSSetConstantBuffers( 1, 1, &g_pcbVSPerFrame11 );

    //V( g_pd3dCtx->Map( g_pcbVSPerObject11, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    //CB_VS_PER_OBJECT* pVSPerObject = ( CB_VS_PER_OBJECT* )MappedResource.pData;
    //D3DXMatrixTranspose( &pVSPerObject->m_mWorldViewProjection, &mWorldViewProjection );
    //D3DXMatrixTranspose( &pVSPerObject->m_mWorld, &mWorld );
    //pVSPerObject->m_MaterialAmbientColor = D3DXVECTOR4( 0.3f, 0.3f, 0.3f, 1.0f );
    //pVSPerObject->m_MaterialDiffuseColor = D3DXVECTOR4( 0.7f, 0.7f, 0.7f, 1.0f );
    //g_pd3dCtx->Unmap( g_pcbVSPerObject11, 0 );
    //g_pd3dCtx->VSSetConstantBuffers( 0, 1, &g_pcbVSPerObject11 );

    //// Set render resources
    //g_pd3dCtx->IASetInputLayout( g_pLayout11 );
    //g_pd3dCtx->VSSetShader( g_pVertexShader11, NULL, 0 );
    //g_pd3dCtx->PSSetShader( g_pPixelShader11, NULL, 0 );
    //g_pd3dCtx->PSSetSamplers( 0, 1, &g_pSamLinear );

	g_pd3dCtx->CopyResource(swapMap.Get(), swapBuffer.Get());

	g_pd3dCtx->ClearRenderTargetView(renderTargetView.Get(), ClearColor);
	g_pd3dCtx->ClearDepthStencilView(stencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_pd3dCtx->ClearDepthStencilView(renderstencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_pd3dCtx->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), renderstencilView.Get()); 

	XMMATRIX xmWorld(mWorld) , xmView(mView) ,xmProj(mProj);
	
	batch.get()->Begin();
	fontTable[0]->DrawString(batch.get(), L"Tests fo DirectX", XMVectorZero(), Colors::Black, 0.0f, XMVectorZero(), XMVectorSet(1.0f, 0.0f,0.0f,0.0f));
	batch.get()->End();
	Microsoft::WRL::ComPtr<ID3D11InputLayout> pLayout;
	for (int idx = 0; idx < objects.size(); idx++)
	{
		BasicEffect* pEffect = (BasicEffect*)effects[0];
		pEffect->SetWorld(xmWorld);
		pEffect->SetView(xmView);
		pEffect->SetLightEnabled(0, false);
		pEffect->SetLightEnabled(1, false);
		pEffect->SetLightEnabled(2, false);
		pEffect->SetPerPixelLighting(false);
		pEffect->SetVertexColorEnabled(false);
		pEffect->SetProjection(xmProj);
		pEffect->SetTextureEnabled(false);
		pEffect->SetAlpha(1.1f);
		pEffect->SetFogEnabled(false);
		pEffect->SetFogColor(Colors::Azure);
		pEffect->SetFogStart(0.0f);
		pEffect->SetFogEnd(100.0f);
		pEffect->SetLightingEnabled(false);
		
		

		objects[idx]->CreateInputLayout(pEffect, &pLayout);

		pEffect->SetTexture(textures[0].Get());
		objects[idx]->Draw(pEffect, pLayout.Get());
		pLayout.Reset();
	}

	

	g_pd3dCtx->OMSetRenderTargets(1, sceneTargetView.GetAddressOf(), nullptr);
	g_pd3dCtx->ClearRenderTargetView(sceneTargetView.Get(), ClearColor1);
	BlurEffect* blur = (BlurEffect*)effects[1];
	blur->SetPSShader(PSShaderType::BloomExtract);
	blur->SetMatrices(xmWorld * DirectX::XMMatrixRotationX(XM_PI/16), xmView, xmProj);
	blur->SetData(0.0f, XMFLOAT2(0.0f, 0.0f), 0,0,0,0.25f);
	blur->SetTexture(textures[1].Get());
	
	objects[0]->CreateInputLayout(blur, pLayout.GetAddressOf());
	objects[0]->Draw(blur, pLayout.Get());
	blur->SetMatrices( xmWorld * DirectX::XMMatrixRotationX(XM_PI/16), xmView, xmProj);
	blur->SetPSShader(PSShaderType::GauussBlur);
	D3D11_TEXTURE2D_DESC texDesc;
	if (g_grabScreen)
	{
		D3D11_RENDER_TARGET_VIEW_DESC desc;
		Microsoft::WRL::ComPtr<ID3D11Resource> cResource;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texLocal;
		sceneTargetView->GetResource(cResource.GetAddressOf());
		cResource.As(&texLocal);
		texLocal->GetDesc(&texDesc);
		SaveWICTextureToFile(g_pd3dCtx, texLocal.Get() , GUID_ContainerFormatJpeg, L"TextureOutLoc.jpg", nullptr);
	}
	blur->SetMatrices( xmWorld * DirectX::XMMatrixRotationX(XM_PI/8), xmView, xmProj);
	blur->SetData(4.0f, XMFLOAT2(1.0f/texDesc.Width, 0.0f), 1.0f,1.0f,1.0f,0.25f);

	g_pd3dCtx->OMSetRenderTargets(1, bloomTargetView.GetAddressOf(), nullptr);
	g_pd3dCtx->ClearRenderTargetView(bloomTargetView.Get(), ClearColor2);
	blur->SetTexture(textures[3].Get());
	objects[0]->Draw(blur, pLayout.Get());


	g_pd3dCtx->OMSetRenderTargets(1, sceneTargetView.GetAddressOf(), nullptr);
	blur->SetMatrices( xmWorld * DirectX::XMMatrixRotationX(XM_PI/4), xmView, xmProj);
    blur->SetData(4.0f, XMFLOAT2(0.0f,1.0f/texDesc.Width), 1.0f,1.0f,1.0f,0.25f);
	blur->SetTexture(textures[2].Get());
	objects[0]->Draw(blur, pLayout.Get());
	g_pd3dCtx->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), renderstencilView.Get());
	
	blur->SetPSShader(PSShaderType::BloomCombine);
	blur->SetData(4.0f, XMFLOAT2(0.0f,1.0f/texDesc.Width), 1.0f,1.0f,1.0f,0.25f);
	blur->SetTexture(textures[3].Get());
	objects[0]->Draw(blur, pLayout.Get());
	pLayout.Reset();

	if (g_grabScreen)
	{
		
		SaveWICTextureToFile(g_pd3dCtx, swapMap.Get() , GUID_ContainerFormatJpeg, L"TextureOut.jpg", nullptr);
		SaveWICTextureToFile(g_pd3dCtx, texturemap1.Get() , GUID_ContainerFormatJpeg, L"TextureOut1.jpg", nullptr);
		SaveWICTextureToFile(g_pd3dCtx, texturemap2.Get() , GUID_ContainerFormatJpeg, L"TextureOut2.jpg", nullptr);
		SaveWICTextureToFile(g_pd3dCtx, swapBuffer.Get() , GUID_ContainerFormatJpeg, L"TextureOut3.jpg", nullptr);
		SaveWICTextureToFile(g_pd3dCtx, stencilmap.Get() , GUID_ContainerFormatJpeg, L"TextureOut4.jpg", nullptr);
		g_grabScreen = false;
	}

	
	//g_pd3dCtx->OMSetRenderTargets(1, sceneTargetView.GetAddressOf(), stencilView.Get());

	
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
	QueryPerformanceCounter(&Freq);
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
			if (step.QuadPart-cbTimeStart.QuadPart/Freq.QuadPart > 1)
			{
				Render();
				cbTimeStart=step;
			}
        }
    }

	return 0;
}

LPVOID GetDesktopImage(PUINT cSize)
{
	HDC hWndDC = GetDC(GetDesktopWindow());
	HDC hMemDC = CreateCompatibleDC(hWndDC);
	int dx = GetSystemMetrics(SM_CXSCREEN);
	int dy = GetSystemMetrics(SM_CYSCREEN);
	HBITMAP hbm = CreateCompatibleBitmap(hWndDC, dx, dy);

	HGDIOBJ hOldBmp = SelectObject(hMemDC, hbm);
	BitBlt(hMemDC, 0, 0, dx, dy, hWndDC, 0,0,SRCCOPY);

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

	GetDIBits(hWndDC, hbm, 0,
        (UINT)bmpScreen.bmHeight,
        bitmapData,
        (BITMAPINFO *)&bi, DIB_RGB_COLORS);

	*cSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	SelectObject(hMemDC, hOldBmp);
	DeleteDC(hMemDC);
	ReleaseDC(GetDesktopWindow(), hWndDC);

	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmfHeader.bfSize = *cSize;
	bmfHeader.bfType = 0x4D42;
	memcpy_s(lpbitmap, *cSize, &bmfHeader, sizeof(BITMAPFILEHEADER));
	memcpy_s(lpbitmap + sizeof(BITMAPFILEHEADER), *cSize, &bi, sizeof(BITMAPINFOHEADER));

	return lpbitmap;

}

void InitApp()
{
	UINT cbSize;
	void* pImage = GetDesktopImage(&cbSize);
	HRESULT hr = E_FAIL;
	
	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureData;
	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	hr = CreateWICTextureFromMemory(g_pd3dDevice, g_pd3dCtx, (byte*)pImage, cbSize, nullptr,&textureData);
	if (SUCCEEDED(hr))
	{
		textureData->GetDesc(&Desc);
		textures.push_back(textureData);
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
				textures.push_back(textureData);
			}
		}
	}

	D3D11_TEXTURE2D_DESC desc;



	hr = g_pSwapChain->GetBuffer(0,  __uuidof( ID3D11Texture2D ), &swapBuffer);
	swapBuffer->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&renderTargetViewDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	renderTargetViewDesc.Format = desc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	g_pd3dDevice->CreateTexture2D((const D3D11_TEXTURE2D_DESC*) &desc, nullptr, swapMap.GetAddressOf());

	g_pd3dDevice->CreateRenderTargetView(swapMap.Get() , &renderTargetViewDesc, &renderTargetView);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC rshaderResourceViewDesc;
    rshaderResourceViewDesc.Format = desc.Format;
	rshaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rshaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	rshaderResourceViewDesc.Texture2D.MipLevels = 1;
	g_pd3dDevice->CreateShaderResourceView(swapMap.Get(), &rshaderResourceViewDesc, &textureData);
	textures.push_back(textureData);
	
	ZeroMemory(&renderTargetViewDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	renderTargetViewDesc.Format = desc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureData2;

	desc.Height = desc.Height / 2;
	desc.Width = desc.Width / 2;
	g_pd3dDevice->CreateTexture2D((const D3D11_TEXTURE2D_DESC*) &desc, nullptr, &texturemap1);
	g_pd3dDevice->CreateTexture2D((const D3D11_TEXTURE2D_DESC*) &desc, nullptr, &texturemap2);
	g_pd3dDevice->CreateRenderTargetView(texturemap1.Get() , &renderTargetViewDesc, &sceneTargetView);
	g_pd3dDevice->CreateRenderTargetView(texturemap2.Get() , &renderTargetViewDesc, &bloomTargetView);

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	g_pd3dDevice->CreateShaderResourceView(texturemap1.Get(), &shaderResourceViewDesc, &textureData);
	textures.push_back(textureData);
	g_pd3dDevice->CreateShaderResourceView(texturemap2.Get(), &shaderResourceViewDesc, &textureData);
	textures.push_back(textureData);

	//Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.Width     = desc.Width;
	depthStencilDesc.Height    = desc.Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count   = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	g_pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &stencilmap);
	g_pd3dDevice->CreateDepthStencilView(stencilmap.Get(), nullptr, &stencilView);

	depthStencilDesc.Width     = desc.Width *2;
	depthStencilDesc.Height    = desc.Height*2;
	g_pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &renderstencilmap);
	g_pd3dDevice->CreateDepthStencilView(renderstencilmap.Get(), nullptr, &renderstencilView);
	//g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &targetView);
	

	fontTable.push_back(std::unique_ptr<SpriteFont>(new SpriteFont(g_pd3dDevice, L"Arial.spf")));
	//fontTable.push_back(std::unique_ptr<SpriteFont>(new SpriteFont(g_pd3dDevice, L"Courier.sft")));
	//fontTable.push_back(std::unique_ptr<SpriteFont>(new SpriteFont(g_pd3dDevice, L"Times.sft")));

	objects.push_back(GeometricPrimitive::CreateCube(g_pd3dCtx, 3.0f, false));
	
	effects.push_back((IEffect*)new BasicEffect(g_pd3dDevice));
	batch.reset(new SpriteBatch(g_pd3dCtx));
	effects.push_back((IEffect*)new BlurEffect(g_pd3dDevice));

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
	//g_Camera.FrameMove( fElapsedTime );
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
		if (wParam == VK_SPACE)
		{
			g_grabScreen = true;
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

