//--------------------------------------------------------------------------------------
// File: MeshFromOBJ10.cpp
//
// This sample shows how an ID3DXMesh object can be created from mesh data stored in an 
// .obj file. It's convenient to use .x files when working with ID3DXMesh objects since 
// D3DX can create and fill an ID3DXMesh object directly from an .x file; however, it’s 
// also easy to initialize an ID3DXMesh object with data gathered from any file format 
// or memory resource.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#pragma warning(disable: 4995)
#include "meshloader10.h"
#pragma warning(default: 4995)
#include "SDKmisc.h"
#include "GlobalType.h"


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3DX10Font*                        g_pFont10 = NULL;
ID3DX10Sprite*                      g_pSprite10 = NULL;

ID3D10Effect*                       g_pEffect10 = NULL;
ID3D10InputLayout*                  g_pVertexLayout = NULL;
ID3D10EffectTechnique*              g_pTechnique = NULL;

ID3D10EffectVectorVariable*         g_pCameraPosition = NULL;

ID3D10EffectMatrixVariable*         g_pWorldViewProjection = NULL;
ID3D10EffectScalarVariable*			g_pNearPlane		= NULL;
ID3D10EffectScalarVariable*			g_pFarPlane		= NULL;

//bool                        g_bShowHelp = true;      // If true, it renders the UI control text
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;           // Device settings dialog
CDXUTDialog                 g_HUD;                   // dialog for standard controls
CDXUTDialog                 g_SampleUI;              // dialog for sample specific controls

CMeshLoader10                       g_MeshLoader;            // Loads a mesh from an .obj file
CModelViewerCamera                  g_Camera;                // A model viewing camera

WCHAR g_strFileSaveMessage[MAX_PATH] = {0}; // Text indicating file write success/failure

//-- Render the quad image --
ID3D10EffectTechnique*              g_pRenderVerticiesQuad      = NULL;

ID3D10Buffer*                       g_pFullScreenVertexBuffer   = NULL;

ID3D10Texture2D*                    g_pColorTexture             = NULL;
ID3D10RenderTargetView*             g_pColorRTView              = NULL;
ID3D10ShaderResourceView*           g_pColorSRView              = NULL;

ID3D10Texture2D*                    g_pDepthStencilTexture      = NULL;
ID3D10DepthStencilView*             g_pDepthStencilDSView       = NULL;
ID3D10ShaderResourceView*           g_pDepthStencilSRView       = NULL;

ID3D10EffectShaderResourceVariable* g_pDepthTex                 = NULL;

int									g_width                     = 0;
int									g_height                    = 0;

bool								g_bSaveImage                = false;

float								g_nearPlane					= 0.1f;
float								g_farPlane					= 200.0f;

typedef struct _FSVertex {

	D3DXVECTOR3         Pos;
	D3DXVECTOR3         Norm;  //  un-used
	D3DXVECTOR2         Tex;

} FSVertex;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_STATIC              -1
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_SUBSET              5
#define IDC_TOGGLEWARP          6

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------

HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBufferSurfaceDesc,
									  void* pUserContext );
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain* pSwapChain,
										  const DXGI_SURFACE_DESC* pBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D10DestroyDevice( void* pUserContext );
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void InitApp();
void RenderSubset( UINT iSubset );
void SaveImage(ID3D10Device* pd3dDevice, ID3D10RenderTargetView* pRTView);
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// DXUT will create and use the best device (either D3D9 or D3D10) 
	// that is available on the system depending on which D3D callbacks are set below

	DXUTSetCallbackD3D10DeviceCreated( OnD3D10CreateDevice );
	DXUTSetCallbackD3D10SwapChainResized( OnD3D10ResizedSwapChain );
	DXUTSetCallbackD3D10SwapChainReleasing( OnD3D10ReleasingSwapChain );
	DXUTSetCallbackD3D10DeviceDestroyed( OnD3D10DestroyDevice );
	DXUTSetCallbackD3D10FrameRender( OnD3D10FrameRender );
	DXUTSetCallbackKeyboard( OnKeyboard );

	InitApp();
	DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
	DXUTCreateWindow( L"MeshFromOBJ10" );
	DXUTCreateDevice( true, 1000, 1000 );
	DXUTMainLoop(); // Enter into the DXUT render loop

	return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	// Initialize dialogs
	g_SettingsDlg.Init( &g_DialogResourceManager );
	g_HUD.Init( &g_DialogResourceManager );
	g_SampleUI.Init( &g_DialogResourceManager );
	// Title font for comboboxes
	g_SampleUI.SetFont( 1, L"Arial", 14, FW_BOLD );
	CDXUTElement* pElement = g_SampleUI.GetDefaultElement( DXUT_CONTROL_STATIC, 0 );
	if( pElement )
	{
		pElement->iFont = 1;
		pElement->dwTextFormat = DT_LEFT | DT_BOTTOM;
	}

	g_SampleUI.AddComboBox( IDC_SUBSET, 20, 25, 140, 24, 'S' );
}


//--------------------------------------------------------------------------------------
//  Create a vertex buffer which contains a full-screen quad in view space.
//  The buffer contains a texture coordinate ranging [0,1] accross the quad.
//  tThe normal is unused but is there to re-use the normal vertex declaration.
//--------------------------------------------------------------------------------------
HRESULT
	SetupFullScreenQuadVertexBuffer(
	ID3D10Device* pd3dDevice )
{

	D3D10_BUFFER_DESC       BufDesc;
	D3D10_SUBRESOURCE_DATA  SRData;
	FSVertex                Verticies[ 4 ];
	HRESULT                 hr;

	//  
	//  setup full-screen quad vertex buffer
	//
	Verticies[ 0 ].Pos.x        = -1;
	Verticies[ 0 ].Pos.y        = -1;
	Verticies[ 0 ].Pos.z        =  1;

	Verticies[ 0 ].Tex.x        =  0;
	Verticies[ 0 ].Tex.y        =  1;

	Verticies[ 1 ].Pos.x        = -1;
	Verticies[ 1 ].Pos.y        =  1;
	Verticies[ 1 ].Pos.z        =  1;

	Verticies[ 1 ].Tex.x        =  0;
	Verticies[ 1 ].Tex.y        =  0;

	Verticies[ 2 ].Pos.x        =  1;
	Verticies[ 2 ].Pos.y        = -1;
	Verticies[ 2 ].Pos.z        =  1;

	Verticies[ 2 ].Tex.x        =  1;
	Verticies[ 2 ].Tex.y        =  1;

	Verticies[ 3 ].Pos.x        =  1;
	Verticies[ 3 ].Pos.y        =  1;
	Verticies[ 3 ].Pos.z        =  1;

	Verticies[ 3 ].Tex.x        =  1;
	Verticies[ 3 ].Tex.y        =  0;


	BufDesc.ByteWidth           = sizeof( FSVertex ) * 4;
	BufDesc.Usage               = D3D10_USAGE_DEFAULT;
	BufDesc.BindFlags           = D3D10_BIND_VERTEX_BUFFER;
	BufDesc.CPUAccessFlags      = 0;
	BufDesc.MiscFlags           = 0;

	SRData.pSysMem              = Verticies;
	SRData.SysMemPitch          = 0;
	SRData.SysMemSlicePitch     = 0;

	hr = pd3dDevice->CreateBuffer(
		&BufDesc,
		&SRData,
		&g_pFullScreenVertexBuffer );

	return hr;
}

//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBufferSurfaceDesc,
									 void* pUserContext )
{
	HRESULT hr;

	V_RETURN( g_DialogResourceManager.OnD3D10CreateDevice( pd3dDevice ) );
	V_RETURN( g_SettingsDlg.OnD3D10CreateDevice( pd3dDevice ) );
	V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
								OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
								L"Arial", &g_pFont10 ) );
	V_RETURN( D3DX10CreateSprite( pd3dDevice, 512, &g_pSprite10 ) );
	//g_pTxtHelper = new CDXUTTextHelper( NULL, NULL, g_pFont10, g_pSprite10, 15 );


	// Read the D3DX effect file
	WCHAR str[MAX_PATH];
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"MeshFromOBJ10.fx" ) );
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows
	// the shaders to be optimized and to run exactly the way they will run in
	// the release configuration of this program.
	dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif
	V_RETURN( D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0", dwShaderFlags, 0, pd3dDevice, NULL,
											  NULL, &g_pEffect10, NULL, NULL ) );

	// Obtain the technique
	g_pTechnique = g_pEffect10->GetTechniqueByName( "NoSpecular" );
	g_pCameraPosition = g_pEffect10->GetVariableByName( "g_vCameraPosition" )->AsVector();

	g_pWorldViewProjection = g_pEffect10->GetVariableByName( "g_mWorldViewProjection" )->AsMatrix();
	g_pNearPlane = g_pEffect10->GetVariableByName("g_nearPlane")->AsScalar();
	g_pFarPlane = g_pEffect10->GetVariableByName("g_farPlane")->AsScalar();

	g_pRenderVerticiesQuad      = g_pEffect10->GetTechniqueByName( "RenderQuad" );
	g_pDepthTex                 = g_pEffect10->GetVariableByName( "g_txDepth" )->AsShaderResource();
	// Define the input layout
	const D3D10_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	} ;
	UINT numElements = sizeof( layout ) / sizeof( layout[0] );

	// Create the input layout
	D3D10_PASS_DESC PassDesc;
	g_pTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	V_RETURN( pd3dDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
											 PassDesc.IAInputSignatureSize, &g_pVertexLayout ) );

	pd3dDevice->IASetInputLayout( g_pVertexLayout );

	//
	//  Create the full-screen vertex buffer
	//
	hr = SetupFullScreenQuadVertexBuffer( pd3dDevice );
	V_RETURN( hr );

	// Load the mesh
	V_RETURN( g_MeshLoader.Create( pd3dDevice, L"media\\flowers.obj" ) );

	// Add the identified subsets to the UI
	CDXUTComboBox* pComboBox = g_SampleUI.GetComboBox( IDC_SUBSET );
	pComboBox->RemoveAllItems();
	pComboBox->AddItem( L"All", ( void* )( INT_PTR )-1 );
	
	
	// Setup the camera's view parameters
	D3DXVECTOR3 vecEye( 0.0f, 0.0f, -3.5f );
	D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
	//D3DXVECTOR3 vecEye( 65.0f, 50.0f, 147.5f );
	//D3DXVECTOR3 vecAt ( 65.0f, 50.0f, 0.0f );
	g_Camera.SetViewParams( &vecEye, &vecAt );

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain* pSwapChain,
										  const DXGI_SURFACE_DESC* pBufferSurfaceDesc, void* pUserContext )
{
	D3D10_TEXTURE2D_DESC                TexDesc;
	D3D10_RENDER_TARGET_VIEW_DESC       RTDesc;
	D3D10_SHADER_RESOURCE_VIEW_DESC     SRDesc;
	D3D10_DEPTH_STENCIL_VIEW_DESC       DSDesc;

	HRESULT hr;

	V_RETURN( g_DialogResourceManager.OnD3D10ResizedSwapChain( pd3dDevice, pBufferSurfaceDesc ) );
	V_RETURN( g_SettingsDlg.OnD3D10ResizedSwapChain( pd3dDevice, pBufferSurfaceDesc ) );
	
	// Setup the camera's projection parameters
	float fAspectRatio = pBufferSurfaceDesc->Width / ( FLOAT )pBufferSurfaceDesc->Height;
	g_Camera.SetProjParams( 92.794f * D3DX_PI / 180.0f, fAspectRatio, g_nearPlane, g_farPlane);
	g_Camera.SetWindow( pBufferSurfaceDesc->Width, pBufferSurfaceDesc->Height );

	g_HUD.SetLocation( pBufferSurfaceDesc->Width - 170, 0 );
	g_HUD.SetSize( 170, 170 );
	g_SampleUI.SetLocation( pBufferSurfaceDesc->Width - 170, pBufferSurfaceDesc->Height - 350 );
	g_SampleUI.SetSize( 170, 300 );

	g_width                   = pBufferSurfaceDesc->Width;
	g_height                  = pBufferSurfaceDesc->Height;					
	//
	//  Create full-screen render target and its views
	//

	TexDesc.Width                   = pBufferSurfaceDesc->Width;
	TexDesc.Height                  = pBufferSurfaceDesc->Height;
	TexDesc.MipLevels               = 0;
	TexDesc.ArraySize               = 1;
	TexDesc.Format                  = pBufferSurfaceDesc->Format;
	TexDesc.SampleDesc.Count        = 1;
	TexDesc.SampleDesc.Quality      = 0;
	TexDesc.Usage                   = D3D10_USAGE_DEFAULT;
	TexDesc.BindFlags               = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags          = 0;
	TexDesc.MiscFlags               = D3D10_RESOURCE_MISC_GENERATE_MIPS;

	hr = pd3dDevice->CreateTexture2D(
		&TexDesc,
		NULL,
		&g_pColorTexture );

	V_RETURN( hr );

	RTDesc.Format = pBufferSurfaceDesc->Format;
	RTDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
	RTDesc.Texture2D.MipSlice = 0;

	hr = pd3dDevice->CreateRenderTargetView(
		g_pColorTexture,
		&RTDesc,
		&g_pColorRTView );

	V_RETURN( hr );

	//
	//  Create depth/stencil view or depth color buffer.
	//    
	TexDesc.Width               = pBufferSurfaceDesc->Width;
	TexDesc.Height              = pBufferSurfaceDesc->Height;
	TexDesc.MipLevels           = 1;
	TexDesc.ArraySize           = 1;
	TexDesc.Format              = DXGI_FORMAT_R16_TYPELESS;
	TexDesc.SampleDesc.Count    = 1;
	TexDesc.SampleDesc.Quality  = 0;
	TexDesc.Usage               = D3D10_USAGE_DEFAULT;
	TexDesc.CPUAccessFlags      = 0;
	TexDesc.MiscFlags           = 0;
	TexDesc.BindFlags       = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;

	hr = pd3dDevice->CreateTexture2D(
		&TexDesc,
		NULL,
		&g_pDepthStencilTexture );

	V_RETURN( hr );

	//-- Create depth stencil view --
	DSDesc.Format = DXGI_FORMAT_D16_UNORM;
	DSDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	DSDesc.Texture2D.MipSlice = 0;

	hr = pd3dDevice->CreateDepthStencilView(
		g_pDepthStencilTexture,
		&DSDesc,
		&g_pDepthStencilDSView );

	V_RETURN( hr );

	SRDesc.Format = DXGI_FORMAT_R16_UNORM;
	SRDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	SRDesc.Texture2D.MostDetailedMip = 0;
	SRDesc.Texture2D.MipLevels = 1;

	hr = pd3dDevice->CreateShaderResourceView(
		g_pDepthStencilTexture,
		&SRDesc,
		&g_pDepthStencilSRView );

	V_RETURN( hr );

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
	// If the settings dialog is being shown, then
	// render it instead of rendering the app's scene
	if( g_SettingsDlg.IsActive() )
	{
		g_SettingsDlg.OnRender( fElapsedTime );
		return;
	}
	
	const UINT                  uOffset             = 0;
	const UINT                  uStride             = sizeof( FSVertex );

	//
	// Clear the back buffer
	//
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f } ; // red, green, blue, alpha

	//
	// get the back-buffer target views
	ID3D10RenderTargetView*     pRTView             = DXUTGetD3D10RenderTargetView();
	ID3D10DepthStencilView*     pDSView             = DXUTGetD3D10DepthStencilView();


	pd3dDevice->ClearRenderTargetView( 
		g_pColorRTView, 
		ClearColor );


	pd3dDevice->ClearDepthStencilView( 
		g_pDepthStencilDSView, 
		D3D10_CLEAR_DEPTH, 
		1.0, 
		0 );

	pd3dDevice->OMSetRenderTargets(
		1,
		&g_pColorRTView,
		g_pDepthStencilDSView );

	HRESULT hr;
	D3DXMATRIXA16 mWorld;
	D3DXMATRIXA16 mView;
	D3DXMATRIXA16 mProj;
	D3DXMATRIXA16 mWorldViewProjection;

	// Get the projection & view matrix from the camera class
	mWorld = *g_Camera.GetWorldMatrix();
	mView = *g_Camera.GetViewMatrix();
	mProj = *g_Camera.GetProjMatrix();

	mWorldViewProjection = mWorld * mView * mProj;

	// Update the effect's variables. 
	V( g_pWorldViewProjection->SetMatrix( (float*)&mWorldViewProjection ) );

	//
	// Set the Vertex Layout
	//
	pd3dDevice->IASetInputLayout( g_pVertexLayout );

	UINT iCurSubset = ( UINT )( INT_PTR )g_SampleUI.GetComboBox( IDC_SUBSET )->GetSelectedData();

	//
	// Render the mesh
	//

		for ( UINT iSubset = 0; iSubset < g_MeshLoader.GetNumSubsets(); ++iSubset )
		{

			g_pTechnique->GetPassByIndex( 0 )->Apply( 0 );
			g_MeshLoader.GetMesh()->DrawSubset(iSubset);
		}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//
	//  Set swap chain render targets to draw to the screen
	//
	pd3dDevice->ClearRenderTargetView( 
		pRTView, 
		ClearColor );

	pd3dDevice->ClearDepthStencilView( 
		pDSView, 
		D3D10_CLEAR_DEPTH, 
		1.0, 
		0 );
		
	pd3dDevice->OMSetRenderTargets(
		1,
		&pRTView,
		pDSView );

	pd3dDevice->IASetVertexBuffers(
		0,
		1,
		&g_pFullScreenVertexBuffer,
		&uStride,
		&uOffset );

	g_pDepthTex->SetResource( g_pDepthStencilSRView );
	g_pNearPlane->SetFloat( g_nearPlane);
	g_pFarPlane->SetFloat( g_farPlane);


	g_pRenderVerticiesQuad->GetPassByIndex( 0 )->Apply( 0 );

	pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	pd3dDevice->Draw(
		4,
		0 );
	if (g_bSaveImage)
	{
		SaveImage(pd3dDevice, pRTView);
		g_bSaveImage = false;
	}
	

	g_HUD.OnRender( fElapsedTime );
	g_SampleUI.OnRender( fElapsedTime );    
	
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
/*	switch( nControlID )
	{
	case IDC_TOGGLEFULLSCREEN:
		DXUTToggleFullScreen(); break;
	case IDC_TOGGLEWARP:
		DXUTToggleWARP(); break;
	case IDC_TOGGLEREF:
		DXUTToggleREF(); break;
	case IDC_CHANGEDEVICE:
		g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;    
	}
	*/
}

//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext )
{
	g_DialogResourceManager.OnD3D10ReleasingSwapChain();
	SAFE_RELEASE( g_pColorRTView );
	SAFE_RELEASE( g_pColorSRView );

	SAFE_RELEASE( g_pColorRTView );
	SAFE_RELEASE( g_pColorSRView );
	SAFE_RELEASE( g_pColorTexture );

	SAFE_RELEASE( g_pDepthStencilTexture );
	SAFE_RELEASE( g_pDepthStencilDSView );
	SAFE_RELEASE( g_pDepthStencilSRView );
}

//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice( void* pUserContext )
{
	g_DialogResourceManager.OnD3D10DestroyDevice();
	g_SettingsDlg.OnD3D10DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
	//SAFE_DELETE( g_pTxtHelper );
	SAFE_RELEASE( g_pVertexLayout );
	SAFE_RELEASE( g_pFont10 );
	SAFE_RELEASE( g_pSprite10 );
	SAFE_RELEASE( g_pEffect10 );
	SAFE_RELEASE(g_pFullScreenVertexBuffer);
	g_MeshLoader.Destroy();
}

void SaveImage(ID3D10Device* pd3dDevice, ID3D10RenderTargetView* pRTView)
{
	HRESULT hr;

	ID3D10Resource *backbufferRes;
	pRTView->GetResource(&backbufferRes);

	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width = g_width;  // must be same as backbuffer
	texDesc.Height = g_height; // must be same as backbuffer
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DEFAULT;

	ID3D10Texture2D *texture;

	V( pd3dDevice->CreateTexture2D(&texDesc, 0, &texture) );
	pd3dDevice->CopyResource(texture, backbufferRes);

	V( D3DX10SaveTextureToFile(texture, D3DX10_IFF_BMP, L"test.bmp") );
	texture->Release();
	backbufferRes->Release();
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	if( bKeyDown )
	{
		switch( nChar )
		{
		case KEY_F:
			g_bSaveImage = true;
		}
	}
}
