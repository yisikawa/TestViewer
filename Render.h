
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <d3d9.h>
#include <d3dx9.h>
#include <commctrl.h>
#include <string>

using namespace std;

//======================================================================
// PROTOTYPE
//======================================================================
bool InitD3D( void );
void ReleaseD3D( void );

LPDIRECT3DDEVICE9 GetDevice( void );
D3DPRESENT_PARAMETERS *GetAdapter( void );
unsigned long GetVertexShaderVersion( void );

HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf );
HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage );
bool InitRender( void );
void Rendering( void );
bool Create3DSpace( void );
//======================================================================
// �O���[�o���@�����_�����O�@�ݒ�
//======================================================================
typedef class CGlobalSet 
{
protected:

public:
	string				mffdir;
	float				mTime;
	bool				mDispWire,mDispBone;
	int					mMotionSpeed; // 2300
	float				mFov;		// FOV : 60�x
	float				mAspect;		// ��ʂ̃A�X�y�N�g��
	float				mNear_z;			// �ŋߐڋ���
	float				mFar_z;		// �ŉ�������
	D3DXVECTOR3			mLight,mLightbase;
	float				mEyeScale,mEyeAlph,mEyeBeta;
	float				mLightAlph,mLightBeta;
	D3DXVECTOR3			mEye,mEyebase;
	D3DXVECTOR3			mAt;
	D3DXVECTOR3			mUp;
	LPDIRECT3DSURFACE9	pBackBuffer;					// �o�b�N�o�b�t�@
	LPDIRECT3DSURFACE9	pZBuffer;						// Z�o�b�t�@
	float				mLightDist;
	D3DXVECTOR3			mLightPosition;
	D3DXMATRIX			mViewLight;					// ���C�g���猩���ꍇ�̃r���[�}�g���b�N�X
	D3DXMATRIX			matWorld,matView,matProj;

	CGlobalSet();
	virtual ~CGlobalSet();
} CGlobalSet,*LPCGlobalSet;

