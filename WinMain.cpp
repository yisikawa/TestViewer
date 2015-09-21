
// INCLUDE
#define _CRT_SECURE_NO_WARNINGS
#include "Render.h"
#include "model.h"
#include "resource.h"
#include <commdlg.h>

#pragma comment(lib,"comdlg32.lib")

// PROTOTYPE
LRESULT CALLBACK WinProc( HWND hWnd, UINT msg, UINT wParam, LONG lParam );
LRESULT CALLBACK Dlg2Proc( HWND,UINT,WPARAM,LPARAM);

// GLOBAL
#define SAFE_DELETES(p)		if ( (p) != NULL ) { delete [] (p); (p) = NULL; }
#define SAFE_DELETE(p)		if ( (p) != NULL ) { delete (p); (p) = NULL; }
#define PAI				(3.1415926535897932384626433832795f)
#define PAI2			(PAI*2.0f)
extern	CGlobalSet		Glob;

extern	CNPC			*pNPC;

long					g_mScreenWidth	= 800;
long					g_mScreenHeight	= 600;
static	char			*AppName = "Ewhu ver0.1";
static	char			*ClassName = "MonsViewer";
static	DWORD			FPS;

HWND hWindow;				// ウィンドウハンドル
HWND hDlg2;					// ダイアログ２

		unsigned long	Polygons;
char	execDir[512];

//		各種関数
long GetScreenWidth( void ) { return g_mScreenWidth; }
long GetScreenHeight( void ) { return g_mScreenHeight; }
HWND GetWindow( void ) { return hWindow; }
void AdDrawPolygons( unsigned long polys ) { Polygons += polys; }

//		WinMain関数
int __stdcall WinMain( HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show )
{
	GetCurrentDirectory(sizeof(execDir),execDir);

	timeBeginPeriod( 1 );

	// ウィンドウクラス登録
	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc		= WinProc; 
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= inst;
	wc.hIcon			= LoadIcon(inst, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= ( HBRUSH )GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName		= MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName	= ClassName;
	if ( RegisterClass( &wc ) == NULL ) return false;
	
	// ウィンドウサイズ取得
	long window_w = g_mScreenWidth + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
	long window_h = g_mScreenHeight + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);

	// ウィンドウ生成
	hWindow = CreateWindowEx(
				WS_EX_APPWINDOW,
				ClassName,
				AppName,
				WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE ,
				GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
				GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
				window_w,
				window_h,
				NULL,
				NULL,
				inst,
				NULL );
	// ダイアログ２作成
	hDlg2 = CreateDialog((HINSTANCE)GetWindowLong(hWindow,GWL_HINSTANCE),MAKEINTRESOURCE(IDD_DIALOG2),NULL,(DLGPROC)Dlg2Proc);
	InvalidateRect(hDlg2, NULL, TRUE);
	SetWindowPos( hDlg2,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE );
	ShowWindow( hDlg2,SW_HIDE );
	ShowWindow( hWindow,SW_SHOW );

	// DirectXGraphics初期化
	if ( InitD3D() == false ) return false;

	// 描画処理初期化
	if ( InitRender() == false ) return false;

	// メッセージループ
	MSG msg;
	D3DXVECTOR3	PosPC;
	while ( true )
	{

		// メッセージ処理
		if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
			if( !IsDialogMessage(hDlg2,&msg) ) {
				if ( msg.message == WM_QUIT ) break;
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		// 他
		else{
			// FPSの計測
			static DWORD cnt=0,BeforeTime = timeGetTime();
			DWORD NowTime = timeGetTime();
			int		x,y,z;

			if ( (NowTime - BeforeTime-25*cnt) >= 25 ) {
				cnt++;
			}
			if ( NowTime - BeforeTime >= 500 ) {
				char FpsStr[128];
				pNPC->GetWorldPosition( PosPC );
				x = (int)PosPC.x; y = (int)PosPC.y; z = (int)PosPC.z;
				sprintf( FpsStr, "%s  [ POS : %4d %4d %4d ] [ FPS : %03d/s ] [ %upolygon/sec ]", AppName,
				x,y,z,FPS*2, Polygons*2 );
				SetWindowText( hWindow, FpsStr );
				BeforeTime = NowTime;
				Polygons = 0;
				FPS = 0;
				cnt = 0;
			}

			FPS++;

			// Direct3Dの描画
			// バックバッファと Z バッファをクリア
			GetDevice()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER| D3DCLEAR_STENCIL, D3DCOLOR_XRGB(200,200,255), 1.f, 0 );
			// シーン開始
			if SUCCEEDED( GetDevice()->BeginScene() ) {
				// 各種処理
				Rendering();

				// シーン終了
				GetDevice()->EndScene();

				// バックバッファの内容をプライマリに転送
				if FAILED( GetDevice()->Present( NULL, NULL, NULL, NULL ) ) {
					// リセット
					GetDevice()->Reset( GetAdapter() );
				}
			}
		}
	}

	// DirectXGraphics開放
	ReleaseD3D();

	// おしまい
	timeEndPeriod( 1 );

	return msg.wParam;
}

 //　種族、装備等　イベント処理
LRESULT CALLBACK Dlg2Proc(HWND in_hWnd, UINT in_Message,WPARAM in_wParam, LPARAM in_lParam )
{
	char	ComboString[128];
	FILE	*fd;
	char	ListName[2048];


	switch( in_Message ) {
        case WM_INITDIALOG:
			sprintf(ListName,"%s\\List\\MON_LIST.lst",execDir);
			if ((fd = fopen(ListName, "r")) < 0) return false;
			for( int i=0 ; fgets(ComboString,sizeof(ComboString),fd) ; i++) {
				SendMessage(GetDlgItem(in_hWnd, IDC_COMBO31), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ComboString);
			}
			fclose(fd);
			SendMessage(GetDlgItem(in_hWnd, IDC_COMBO31), CB_SETCURSEL, (WPARAM)0, 0L);
			break;
 		case WM_COMMAND:
			switch( LOWORD(in_wParam ) ) {
				case IDC_COMBO31:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						SAFE_DELETE(pNPC);pNPC = new CNPC;
 						//	NPCデータ設定
						GetWindowText(GetDlgItem(in_hWnd, IDC_COMBO31), ComboString, sizeof(ComboString));
						pNPC->SetModelPath(Glob.mffdir+"\\"+strtok(ComboString,","));
						pNPC->LoadNPC();
						pNPC->InitTransform();
						SetFocus(GetDlgItem(in_hWnd, IDC_COMBO31));
					}
					break;
				case IDOK:
					ShowWindow(in_hWnd,SW_HIDE);
					break;
			}
			break;
		case WM_CLOSE:
			ShowWindow(in_hWnd,SW_HIDE);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return 0;
}

//		メッセージ処理
LRESULT CALLBACK WinProc( HWND hWnd, UINT msg, UINT wParam, LONG lParam )
{
static float	alpha = 0.,beta = 0.;
static float	Delta=0.,Step=0.2f;
	D3DXVECTOR3 Pos,Post;
	D3DXMATRIX	mm,m1,m2;
static bool		lDrag = false,rDrag = false;
static short	x1=-1,y1=-1,x2,y2;
   // ファイルオープンダイアログボックスのテスト
    OPENFILENAME sfn;
    char szFPath[256],szFName[256],strmsg[256];

    lstrcpy(szFPath, "*.mqo");
    ZeroMemory(&sfn, sizeof(sfn));
    sfn.lStructSize = sizeof(sfn);
    sfn.hwndOwner = NULL;
    sfn.lpstrFile = szFPath;
    sfn.nMaxFile = sizeof(szFPath);
    sfn.lpstrFilter = "MQO Format(*.mqo)\0*.mqo\0";
    sfn.nFilterIndex = 1;
    sfn.lpstrFileTitle = szFName;
    sfn.nMaxFileTitle = sizeof(szFName);
	sfn.lpstrTitle = "MQOセーブ";
    sfn.lpstrInitialDir = NULL;
 
	switch( msg )
	{

		//	終了時
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		case WM_MOUSEWHEEL:
			if( wParam==0xff880000 ) {
				if( GetKeyState(VK_CONTROL)&0x8000 ){
					Glob.mEyeScale += 0.05f * 10.f;
				} else {
					Glob.mEyeScale += 0.05f;
				}
				D3DXMATRIX mat,matY,matX,matS;
				D3DXMatrixRotationY(&matY,Glob.mEyeAlph);
				D3DXMatrixRotationX(&matX,Glob.mEyeBeta);
				D3DXMatrixScaling(&matS,Glob.mEyeScale,Glob.mEyeScale,Glob.mEyeScale);
				mat = matS * matX * matY;
				D3DXVec3TransformNormal(&Glob.mEye,&Glob.mEyebase,&mat);
				Glob.mEye += Glob.mAt;
				D3DXMatrixLookAtLH( &Glob.matView, &Glob.mEye, &Glob.mAt, &Glob.mUp );
			} else if( wParam ==0x00780000 ) {
				if( Glob.mEyeScale <=0.1f ) break;
				if( GetKeyState(VK_CONTROL)&0x8000 ){
					Glob.mEyeScale -= 0.05f * 10.f;
				} else {
					Glob.mEyeScale -= 0.05f;
				}
				D3DXMATRIX mat,matY,matX,matS;
				D3DXMatrixRotationY(&matY,Glob.mEyeAlph);
				D3DXMatrixRotationX(&matX,Glob.mEyeBeta);
				D3DXMatrixScaling(&matS,Glob.mEyeScale,Glob.mEyeScale,Glob.mEyeScale);
				mat = matS * matX * matY;
				D3DXVec3TransformNormal(&Glob.mEye,&Glob.mEyebase,&mat);
				Glob.mEye += Glob.mAt;
				D3DXMatrixLookAtLH( &Glob.matView, &Glob.mEye, &Glob.mAt, &Glob.mUp );
			} 
			break;

 		case WM_MOUSEMOVE:
			x2 = LOWORD(lParam);
			y2 = HIWORD(lParam);
			if( wParam & MK_LBUTTON ) {
				if( abs(x2-x1)<20 && abs(y2-y1) <20 ) {
					Glob.mEyeAlph += (float)(x2-x1)/(float)g_mScreenWidth*2.f*PAI;
					Glob.mEyeBeta += (float)(y1-y2)/(float)g_mScreenWidth*2.f*PAI;
					Glob.mEyeAlph = (Glob.mEyeAlph>PAI2)?(Glob.mEyeAlph-PAI2):Glob.mEyeAlph;
					Glob.mEyeAlph = (Glob.mEyeAlph<-PAI2)?(Glob.mEyeAlph+PAI2):Glob.mEyeAlph;
					Glob.mEyeBeta = (Glob.mEyeBeta>(PAI/2.f))?(PAI/2.f-0.02f):Glob.mEyeBeta;
					Glob.mEyeBeta = (Glob.mEyeBeta<(-PAI/2.f))?(-PAI/2.f+0.02f):Glob.mEyeBeta;
					D3DXMATRIX mat,matY,matX,matS;
					D3DXMatrixRotationY(&matY,Glob.mEyeAlph);
					D3DXMatrixRotationX(&matX,Glob.mEyeBeta);
					D3DXMatrixScaling(&matS,Glob.mEyeScale,Glob.mEyeScale,Glob.mEyeScale);
					mat = matS * matX * matY;
					D3DXVec3TransformNormal(&Glob.mEye,&Glob.mEyebase,&mat);
					Glob.mEye += Glob.mAt;
					D3DXMatrixLookAtLH( &Glob.matView, &Glob.mEye, &Glob.mAt, &Glob.mUp );
				}
			} else if( wParam & MK_MBUTTON ) {
				if( abs( y2-y1) <20 ) {
					Glob.mAt.y += (y2-y1)/100.f;
					D3DXMATRIX mat,matY,matX,matS;
					D3DXMatrixRotationY(&matY,Glob.mEyeAlph);
					D3DXMatrixRotationX(&matX,Glob.mEyeBeta);
					D3DXMatrixScaling(&matS,Glob.mEyeScale,Glob.mEyeScale,Glob.mEyeScale);
					mat = matS * matX * matY;
					D3DXVec3TransformNormal(&Glob.mEye,&Glob.mEyebase,&mat);
					Glob.mEye += Glob.mAt;
					D3DXMatrixLookAtLH( &Glob.matView, &Glob.mEye, &Glob.mAt, &Glob.mUp );
				}
			} else if( wParam & MK_RBUTTON ) {
				if( abs(x2-x1)<20 && abs(y2-y1) <20 ) {
					Glob.mLightAlph += (float)(x2-x1)/(float)g_mScreenWidth*2.f*PAI;
					Glob.mLightBeta += (float)(y1-y2)/(float)g_mScreenWidth*2.f*PAI;
					Glob.mLightAlph = (Glob.mLightAlph>PAI2)?(Glob.mLightAlph-PAI2):Glob.mLightAlph;
					Glob.mLightAlph = (Glob.mLightAlph<-PAI2)?(Glob.mLightAlph+PAI2):Glob.mLightAlph;
					Glob.mLightBeta = (Glob.mLightBeta>(PAI/2.f))?(PAI/2.f-0.02f):Glob.mLightBeta;
					Glob.mLightBeta = (Glob.mLightBeta<(-PAI/2.f))?(-PAI/2.f+0.02f):Glob.mLightBeta;
					D3DXMATRIX mat,matY,matX;
					D3DXMatrixRotationY(&matY,Glob.mLightAlph);
					D3DXMatrixRotationX(&matX,Glob.mLightBeta);
					mat = matX * matY;
					D3DXVec3TransformNormal(&Glob.mLight,&Glob.mLightbase,&mat);
					D3DXVec3Normalize( &Glob.mLight, &Glob.mLight );
					//GetDevice()->SetLight( 0, &Glob.mLight );
				}
			}
			x1 = x2; y1 = y2;
			break;
		case WM_COMMAND:
			if( LOWORD(wParam) == ID_MNU_SAVE ) {
				wsprintf(strmsg, "NPCは未サポートです");
				MessageBox(NULL, strmsg, "ＷＡＲＮＮＩＮＧ", MB_OK | MB_ICONINFORMATION);
			} else if( LOWORD(wParam) == ID_MNU_INVENT ) {
				ShowWindow( hDlg2,SW_SHOW );
			} else if(LOWORD(wParam) == ID_MNU_EXIT) {
				if( MessageBox(NULL, "本当に終了しますか？", "プログラム終了", MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
					SendMessage(hWnd, WM_CLOSE, 0L, 0L);
				}
			}

		//	その他
		default:
			return DefWindowProc( hWnd, msg, wParam, lParam );
	}

	return 0;
}

