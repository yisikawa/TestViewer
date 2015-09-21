
// INCLUDE
#define _CRT_SECURE_NO_WARNINGS

#include "WinMain.h"
#include "Render.h"
#include "Model.h"

// PROTOTYPE

// DEFINE
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }
#define SAFE_DELETES(p)		if ( (p) != NULL ) { delete [] (p); (p) = NULL; }
#define SAFE_DELETE(p)		if ( (p) != NULL ) { delete (p); (p) = NULL; }
#define PAI					(3.1415926535897932384626433832795f)
#define PAI2				(PAI*2.0f)

inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

// GLOBAL
static LPDIRECT3D9				g_pDirect3D=NULL;
static LPDIRECT3DDEVICE9		g_pD3DDevice=NULL;
static D3DPRESENT_PARAMETERS	g_md3dpp;
static LPD3DXEFFECT				g_pEffect=NULL;
static LPD3DXMESH				g_pMesh=NULL;
static D3DMATERIAL9*			g_pMeshMaterials = NULL;
static LPDIRECT3DTEXTURE9*		g_pMeshTextures  = NULL; 
static DWORD					g_dwNumMaterials = 0;
static LPD3DXFONT				g_pFont = NULL;
unsigned long					g_mVertexShaderVersion;
int								g_mMaxVertexShaderConst=0; // 頂点シェーダー　MAX　Matrix
BOOL							g_mIsUseSoftware = FALSE;
CGlobalSet	Glob;
		float		g_mTime				=	0.;
		CNPC		*pNPC				=	NULL;
extern	HWND		hDlg2;

static	float		fTime		= 0;

//		各種関数
LPDIRECT3DDEVICE9 GetDevice( void ) { return g_pD3DDevice; }
D3DPRESENT_PARAMETERS *GetAdapter( void ) { return &g_md3dpp; }
unsigned long GetVertexShaderVersion( void ) { return g_mVertexShaderVersion; }

// デバッグ用に表示するテクスチャ用の構造体
typedef struct {
    FLOAT       p[4];
    FLOAT       tu, tv;
} TVERTEX;

//		CGlobalSet
CGlobalSet::CGlobalSet()
{
//	mffdir			= "C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV\\client\\chara\\mon";
	mffdir = "E:\\Dropbox\\Projects\\FINAL FANTASY XIV\\client\\chara\\mon";
	mDispWire = false;
	mDispBone		= false;
	mTime			= 0.;
	mMotionSpeed	= 2800; // 2300
	mFov			= PAI / 4.f;		// FOV : 60度
	mAspect			= 1.330f;		// 画面のアスペクト比
	mNear_z			= 0.1f;			// 最近接距離
	mFar_z			= 1400.0f;		// 最遠方距離
	mEyeScale		= 1.f;
	mEyeAlph		= 0.f;
	mEyeBeta		= 0.f;
	mLightAlph		= 0.f;
	mLightBeta		= 0.f;
	mLightDist		= 3.f;
	mEyebase.x = 0.f;	mEyebase.y = 1.f;	mEyebase.z = 3.f; mEye = mEyebase; 
	mAt.x = 0.f;	mAt.y = 0.0f;	mAt.z = 0.f;
	mUp.x = 0.f;	mUp.y = 1.0f;	mUp.z = 0.f;
	mLightPosition.x = -3.f;	mLightPosition.y = 3.f;	mLightPosition.z = 3.f;
	D3DXMatrixIdentity(&matWorld);
}

CGlobalSet::~CGlobalSet()
{
}

//		DirectXGraphics初期化
bool InitD3D( void )
{
	HRESULT hr;
	D3DDISPLAYMODE d3ddm;

	// Direct3D オブジェクトを作成
	g_pDirect3D = Direct3DCreate9( D3D_SDK_VERSION );
	if ( g_pDirect3D == NULL ) {
		MessageBox( NULL, "Direct3Dの作成に失敗しました", "Error", MB_OK | MB_ICONSTOP );
		return false;
	}

	// 現在の画面モードを取得
	hr = g_pDirect3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	if FAILED( hr ) {
		MessageBox( NULL, "画面モードの取得に失敗しました", "Error", MB_OK | MB_ICONSTOP );
		return false;
	}

	// Direct3D 初期化パラメータの設定
	ZeroMemory( &g_md3dpp, sizeof(D3DPRESENT_PARAMETERS) );

	g_md3dpp.BackBufferCount	= 1;
	g_md3dpp.Windowed			= TRUE;
	g_md3dpp.BackBufferWidth	= GetScreenWidth();
	g_md3dpp.BackBufferHeight	= GetScreenHeight();

	// ウインドウ : 現在の画面モードを使用
	g_md3dpp.BackBufferFormat		= d3ddm.Format;
	g_md3dpp.MultiSampleType		= D3DMULTISAMPLE_NONE;
	g_md3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	g_md3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_DEFAULT;
	g_md3dpp.hDeviceWindow			= GetWindow();

	// Z バッファの自動作成
	g_md3dpp.EnableAutoDepthStencil	= TRUE;
	//g_md3dpp.AutoDepthStencilFormat	= D3DFMT_D16;
	g_md3dpp.AutoDepthStencilFormat	= D3DFMT_D24S8;
	g_md3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;//ダブルステンシル

	// シェーダーバージョン取得
	D3DCAPS9 caps;
	g_pDirect3D->GetDeviceCaps( 0, D3DDEVTYPE_HAL, &caps );
	g_mVertexShaderVersion = caps.VertexShaderVersion;
	g_mMaxVertexShaderConst = caps.MaxVertexShaderConst;

	// デバイスの生成

	// 頂点シェーダー2.0？
	if ( g_mVertexShaderVersion >= D3DVS_VERSION(2,0) ) {
		g_mIsUseSoftware = FALSE;	// HARDWARE T&L
		if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
			// SOFTWARE T&L
			if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
				// REFERENCE RASTERIZE
				if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {				
					MessageBox( NULL,"Direct3Dデバイスの生成に失敗しました", "Error" , MB_OK | MB_ICONSTOP );
					return false;
				}
			}
		}
	} else {
		g_mIsUseSoftware = TRUE;	// SOFTWARE T&L
		if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_MIXED_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
			// SOFTWARE T&L
			if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
				// REFERENCE RASTERIZE
				if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {				
					MessageBox( NULL,"Direct3Dデバイスの生成に失敗しました", "Error" , MB_OK | MB_ICONSTOP );
					return false;
				}
			}
		}
	}
	//文字列レンダリングの初期化
	if(FAILED(D3DXCreateFont(g_pD3DDevice,0,10,FW_REGULAR,	NULL,	FALSE,SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_MODERN, "tahoma", &g_pFont))) 
	{
		return false;
	}
	

	//シェーダーを読み込む

//	if( FAILED(  D3DXCreateEffectFromFile(g_pD3DDevice, "PhongShader.fx", NULL, NULL,
	if( FAILED(  D3DXCreateEffectFromFile(g_pD3DDevice, "Bump.fx", NULL, NULL,
		0, NULL, &g_pEffect, NULL)))
	{
		MessageBox( NULL, "シェーダーファイル読み込み失敗", "", MB_OK | MB_ICONSTOP ); 
		return false;
	}
	
	return true;
}

//		DirectXGraphics開放
void ReleaseD3D( void )
{
	if ( g_pD3DDevice != NULL ) g_pD3DDevice->Release();
	if ( g_pDirect3D != NULL ) g_pDirect3D->Release();
}

//		頂点バッファ生成
HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf )
{
	HRESULT hr = g_pD3DDevice->CreateVertexBuffer(
					size,
					Usage,
					fvf,
					D3DPOOL_MANAGED,
					lpVB,NULL );
	return hr;
}

//		インデックスバッファ生成
HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage )
{
	HRESULT hr = g_pD3DDevice->CreateIndexBuffer(
					size,
					Usage,
					D3DFMT_INDEX16,
					D3DPOOL_MANAGED,
					lpIB,NULL );
	return hr;
}

//		レンダリング
void Rendering( void )
{
	D3DXVECTOR3		Pos;
	static unsigned long OldTime = timeGetTime();
	unsigned long NowTime = timeGetTime();

	fTime = (float)(NowTime - OldTime) / 1000.0f;
	OldTime = NowTime;
	// アニメーション時間
	g_mTime += fTime*Glob.mMotionSpeed;

	// 変換適用（引数はアニメ時間
	// レンダリング
	unsigned long poly = 0;
	//	ライト位置の計算
	Glob.mLightPosition = Glob.mAt+Glob.mLightDist*-Glob.mLight;
	D3DXMatrixLookAtLH( &Glob.mViewLight,&Glob.mLightPosition,&Glob.mAt,&Glob.mUp);

	//ワールドトランスフォーム（絶対座標変換）		
	GetDevice()->SetTransform( D3DTS_WORLD, &Glob.matWorld );		
	// ビュートランスフォーム（視点座標変換）
	D3DXMatrixLookAtLH( &Glob.matView, &Glob.mEye, &Glob.mAt, &Glob.mUp );
	GetDevice()->SetTransform( D3DTS_VIEW, &Glob.matView );
	// プロジェクショントランスフォーム（射影変換）
	GetDevice()->SetTransform( D3DTS_PROJECTION, &Glob.matProj );
	GetDevice()->SetRenderState( D3DRS_STENCILENABLE,		FALSE );
	GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );
	GetDevice()->SetRenderState( D3DRS_ALPHAREF,			0x80 );
	GetDevice()->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_GREATER );
	GetDevice()->SetRenderState( D3DRS_CULLMODE,			D3DCULL_NONE );
	GetDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE,	FALSE);
	GetDevice()->SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCCOLOR );
	GetDevice()->SetRenderState(D3DRS_DESTBLEND,		D3DBLEND_INVSRCCOLOR );
	//GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE4X);
	//GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	//GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	g_pEffect->SetTechnique( "tecPhong" );		//ワールド・ビュー・プロジェクション行列を渡す
	g_pEffect->SetMatrix("WVP",(D3DXMATRIX*)&(Glob.matWorld*Glob.matView*Glob.matProj));	
	//ワールド行列を渡す				
	g_pEffect->SetMatrix("matWorld",&Glob.matWorld);
	//ライトの方向ベクトルを渡す
	D3DXVECTOR4 lightDir = -Glob.mLight; lightDir.w = 0.f;
	g_pEffect->SetVector("LightDir",&lightDir);
	//視点位置（カメラ位置）を渡す
	g_pEffect->SetVector("Eye",(D3DXVECTOR4*)&Glob.mEye);		
	//拡散反射率を渡す 赤にしてみた
	g_pEffect->SetVector("Diffuse",&D3DXVECTOR4(1.f,1.f,1.f,1.f));
	//環境光を0.3にしてみた
	g_pEffect->SetVector("Ambient",&D3DXVECTOR4(.15f,.15f,.15f,1.f));
	//入射光（ライト）の強度を渡す　目一杯明るい白色光にしてみる
	g_pEffect->SetVector("LightIntensity",&D3DXVECTOR4(1.f,1.f,1.f,1));
	//ワールド行列の逆行列の転置行列を渡す
	D3DXMATRIX matWIT;		
	g_pEffect->SetMatrix("WIT",D3DXMatrixTranspose(&matWIT,D3DXMatrixInverse(&matWIT,NULL,&Glob.matWorld)));
	// メッシュ描画　開始
	g_pEffect->SetTexture("texDecal",NULL);
	g_pEffect->SetTexture("NormalMap",NULL);
	g_pEffect->SetTexture("SpecularMap",NULL);
	for( int i=0 ;i<(int)pNPC->a_tex.size(); i++ ) {
		if( pNPC->a_tex[i].m_texName.find("_d")!=-1 ) {
			g_pEffect->SetTexture("texDecal",pNPC->a_tex[i].a_texData[0]);
			break;
		}
	}
	for( int i=0 ;i<(int)pNPC->a_tex.size(); i++ ) {
		if( pNPC->a_tex[i].m_texName.find("_n")!=-1 ) {
			g_pEffect->SetTexture("NormalMap",pNPC->a_tex[i].a_texData[0]);
			break;
		}
	}
	//for( int i=0 ;i<(int)pNPC->a_tex.size(); i++ ) {
	//	if( pNPC->a_tex[i].m_texName.find("_s")!=-1 ) {
	//		g_pEffect->SetTexture("SpecularMap",pNPC->a_tex[i].a_texData[0]);
	//		break;
	//	}
	//}
	g_pEffect->Begin( NULL, 0 );						 
	g_pEffect->BeginPass(0);
	for( int i=0 ; i<(int)pNPC->a_Meshes.size() ; i++ ) {		
		CMesh *pMesh = &pNPC->a_Meshes[i];
		// BOXオフセットの設定
		//D3DXVECTOR4 offset = pNPC->a_Meshes[i].m_box.m_offset;offset.w = 0.f;
		D3DXVECTOR4 offset = pNPC->m_box.m_offset;offset.w = 0.f;
		g_pEffect->SetVector("BoxOffSet",&offset);
		// BOXスケールの設定
		//D3DXVECTOR4 scale = pNPC->a_Meshes[i].m_box.m_scale;scale.w = 1.f;
		D3DXVECTOR4 scale = pNPC->m_box.m_scale;scale.w = 1.f;
		g_pEffect->SetVector("BoxScale",&scale);
		GetDevice()->SetVertexDeclaration( pMesh->p_VertexFormat );
// 頂点バッファ1をデバイスに設定
		GetDevice()->SetStreamSource( 0,pMesh->m_lpVB, 0, pMesh->m_VertexSize );
// インデックスバッファをデバイスに設定
		GetDevice()->SetIndices( pMesh->m_lpIB );
		GetDevice()->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST,0,0,
			//D3DPT_TRIANGLESTRIP,0,0,
			pMesh->m_NumVertices,0,pMesh->m_NumFaces );
	}
	//for( DWORD i=0; i<g_dwNumMaterials; i++ )        
	//{
	//	//g_pEffect->SetTexture("texDecal",g_pMeshTextures[i]);				
	//	g_pMesh->DrawSubset( i );
	//}
	g_pEffect->EndPass();
	g_pEffect->End();
	// メッシュ描画　終了
	RECT rect={100,100,0,0};	
	//文字列のサイズを計算	
	g_pFont->DrawText(NULL,"これはテスト１です",-1,&rect,DT_CALCRECT,NULL);	
	// そのサイズでレンダリング	
	g_pFont->DrawText(NULL,"これはテスト２です",-1,&rect,DT_LEFT | DT_BOTTOM,0xffff0000);
	AdDrawPolygons( poly );
}

//		3D空間の生成
bool Create3DSpace( void )
{
	HRESULT	hr;
	// バックバッファ取得
	hr = GetDevice()->GetRenderTarget( 0,&Glob.pBackBuffer );
	if FAILED( hr ) return false;

	// Zバッファ生成
	hr = GetDevice()->GetDepthStencilSurface( &Glob.pZBuffer );
	if FAILED( hr ) return false;

	// プロジェクション行列の設定
	// 行列生成
	D3DXMatrixPerspectiveFovLH( &Glob.matProj, Glob.mFov, Glob.mAspect, Glob.mNear_z, Glob.mFar_z );

	// デフォルトのカメラの設定
	D3DXMatrixLookAtLH( &Glob.matView, &Glob.mEye, &Glob.mAt, &Glob.mUp );

	// レンダリングステート
	float	start	 = 0.0f;
	float	end		 = 1.0f;
	GetDevice()->SetRenderState( D3DRS_DITHERENABLE,		TRUE );
	GetDevice()->SetRenderState( D3DRS_ZENABLE,				TRUE );
	GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE );
	GetDevice()->SetRenderState( D3DRS_FOGTABLEMODE,		D3DFOG_NONE );
	//GetDevice()->SetRenderState( D3DRS_FOGVERTEXMODE,		D3DFOG_LINEAR );
	//GetDevice()->SetRenderState( D3DRS_FOGCOLOR,			D3DCOLOR_XRGB(200,200,255) );
	//GetDevice()->SetRenderState( D3DRS_FOGSTART,			*(DWORD*)(&start) );
	//GetDevice()->SetRenderState( D3DRS_FOGEND,				*(DWORD*)(&end) );
	// ライト

	D3DXVec3Normalize( &Glob.mLightbase, &D3DXVECTOR3( 0.3f, -1.0f, -0.3f) );
	Glob.mLight = Glob.mLightbase;
//	GetDevice()->LightEnable( 0, TRUE );

	// ライト方向のカメラの設定
	Glob.mLightPosition = Glob.mAt+Glob.mLightDist*-Glob.mLight;
	D3DXMatrixLookAtLH( &Glob.mViewLight,&Glob.mLightPosition,&Glob.mAt,&Glob.mUp);
	return true;
}

//		初期化
bool InitRender( void )
{
	char			ComboString[128];

	// 初期設定
	if ( !Create3DSpace() )
	{
		MessageBox( NULL, "初期設定に失敗", "Error", MB_OK );
		return false;
	}

	// モデルデータ読み込み（頂点フォーマットを指定

	// unsigned long ModelFVF = (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);　
	pNPC = new CNPC;
 	GetWindowText(GetDlgItem(hDlg2, IDC_COMBO31), ComboString, sizeof(ComboString));
	pNPC->SetModelPath(Glob.mffdir+"\\"+strtok(ComboString,","));
	pNPC->LoadNPC();
	// 変換初期化
	pNPC->InitTransform();
	return true;
}
