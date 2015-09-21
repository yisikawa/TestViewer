
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <math.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <list>
#include <vector>
#include <string>


using namespace std;


//======================================================================
// GLOBAL
//======================================================================

#define M_PI 3.14159265358979 /* 円周率 */

//======================================================================
// TYPE DEFINE
//======================================================================
class CENVD;
class CMesh;
class CBone;
class CMotionFrame;
class CModel;

//======================================================================
// リソースタイプクラス
//======================================================================
const int PWIB = 0x42495750;
const int GTEX = 0x58455447;
const int SSCF = 0x46435353;
const int SEDB = 0x42444553;
const int SQEX = 0x58455153;
const int RLE  = 0x0C656C72;
const int SANE = 0x656E6173;
const int SPKL = 0x4C4B5053;

enum MagicNumber : long long
{
	Texture		= 0x0062787442444553, // SEDBtxb\0
	Resource	= 0x2053455242444553, // 'SEDBRES '
	Sound		= 0x4643535342444553, // SEDBSSCF
	Vins		= 0x736E697642444553, // SEDBvins
	Vtex		= 0x7865747642444553, // SEDBvtex
	Vmdl		= 0x6C646D7642444553, // SEDBvmdl
	Veff		= 0x6666657642444553, // SEDBveff
	Leaf		= 0x6661656C42444553, // SEDBleaf
	Phb			= 0x0042485042444553, // SEDBPHB\0
	Mtb			= 0x0062746D42444553,  // SEDBmtb\0
	UnMagic		= -1
};

enum SectionType : int
{
    mcb  = 0x006D6362,
    mtb  = 0x006D7462,
    scb  = 0x00736362,
    bin  = 0x0062696E,
    skl  = 0x00736B6C,
    sscf = 0x46435353,
    txb  = 0x00747862,
    eqp  = 0x00657170,
    phb  = 0x00706862,
    sdrb = 0x73647262,
    wrb  = 0x00777262,
    trb  = 0x00747262,
    elb  = 0x00656C62,
    veff = 0x76656666,
    vins = 0x76696E73,
    vmdl = 0x766D646C,
    vtex = 0x76746578,
    leaf = 0x6C656166,
    acb  = 0x00616362,
    ciba = 0x63696261,
    cibb = 0x63696262,
    cibc = 0x63696263,
    cibe = 0x63696265,
    cibf = 0x63696266,
    cibg = 0x63696267,
    cibh = 0x63696268,
    cibm = 0x6369626D,
    cibp = 0x63696270,
    cibs = 0x63696273,
    cibt = 0x63696274,
    cibw = 0x63696277,
	unsect = -1
};

typedef struct{ // tag data
	int		section,fsize,un,offset;
} TAGhead;

typedef struct{
	MagicNumber			magicNumber;
	int					version,un,sectSize;
	char				junk[28];
} SECThead;


typedef struct {
	int	numRes,strOffset,numStr,resType;
} REShead;

typedef struct{
	int index,offset,size,type,chidx;
} RESinfo;

typedef class CSection
{
protected:

public:
	int					m_resPos,m_resNum;
	SECThead			m_sectH;
	REShead				m_resH;
	vector<RESinfo>		a_resInfos;
	vector<SectionType>	a_resTypes;
	vector<string>		a_resIDs;
	vector<string>		a_strTables;
	vector<CSection>		a_Resources;
	CSection();
	virtual ~CSection();
	virtual void LoadRes(int pos, char *pbuf);
	virtual int  LoadSectionHead(int pos, char *pbuf);
	virtual int  LoadResourceHead(int pos, char *pbuf);
	virtual int  LoadResourceInfo(int pos, char *pbuf);
	virtual int  LoadResourceTypes(int pos, char *pbuf);
	virtual int  LoadResourceIDs(int pos, char *pbuf);
	virtual int  LoadStringTables(int pos, char *pbuf);
	virtual int  LoadResources(int pos, char *pbuf);
	virtual MagicNumber ChangeSectionType( SectionType type );
	virtual int  GetSectionStart( SectionType type );
	virtual int  GetSectionStart( SectionType type,string resid );
} 
CSection, LPCSection;

 enum ChunkType : int
{
    WRB  = 0x00425257,
    MDLC = 0x434C444D,
    MDL  = 0x004C444D,  // model
    NAME = 0x454D414E,
    HEAD = 0x44414548,
    MESH = 0x4853454D,
    STR  = 0x00525453,
    RSID = 0x44495352,
    RSTP = 0x50545352,
    PRID = 0x44495250,
    PRTP = 0x50545250,
    STMS = 0x534D5453,
    ENVD = 0x44564E45,
    AABB = 0x42424141,
    COMP = 0x504D4F43,
    LTCD = 0x4443544C,
    MICT = 0x5443494D,
    MINS = 0x534E494D,
    PGRP = 0x50524750,
    SHAP = 0x50414853,
	SHD  = 0x00444853,  // shader 
    HLSL = 0x454C4946,  // shader FILE
    VCAP = 0x50414356,
    PCAP = 0x50414350,
    PRAM = 0x4D415250

};

typedef struct {
	ChunkType	chunkType;
	int			un,dataSize,nextChunk;
} CHUNKhead;

typedef struct {
	int		numChild,un1,un2,un3;
} CHUNKdata;

typedef class CChunk
{
protected:

public:
	int					m_chunkPos;
	CHUNKhead			m_chunkH;
	CHUNKdata			m_chunkD;
	vector<CChunk>		a_Chunks;
	CChunk();
	virtual ~CChunk();
	virtual void LoadChunk(ChunkType type,int pos, char *pbuf);
	virtual int  LoadChunkHead(int pos, char *pbuf);
	virtual int  LoadChunkData(int pos, char *pbuf);
	virtual int  GetChunkOffset( ChunkType type );
	virtual bool IsChilds( ChunkType type );
} 
CChunk, LPCChunk;

//======================================================================
// インデックスデータ
//======================================================================


typedef class CHalfHelper
{
protected:
public:
   unsigned int* mantissaTable;
   unsigned int* exponentTable;
   unsigned short* offsetTable;
	CHalfHelper();
	virtual ~CHalfHelper();
    // Transforms the subnormal representation to a normalized one. 
    unsigned int  ConvertMantissa(int i)
    {
        unsigned int m = (unsigned int)(i << 13); // Zero pad mantissa bits
        unsigned int  e = 0; // Zero exponent

        // While not normalized
        while ((m & 0x00800000) == 0)
        {
            e -= 0x00800000; // Decrement exponent (1<<23)
            m <<= 1; // Shift mantissa                
        }
        //m &= unchecked((unsigned int)~0x00800000); // Clear leading 1 bit
        m &= (unsigned int)~0x00800000; // Clear leading 1 bit
        e += 0x38800000; // Adjust bias ((127-14)<<23)
        return m | e; // Return combined number
    }

    void GenerateMantissaTable(void)
    {
        mantissaTable[0] = 0;
        for (int i = 1; i < 1024; i++)
        {
            mantissaTable[i] = ConvertMantissa(i);
        }
        for (int i = 1024; i < 2048; i++)
        {
            mantissaTable[i] = (unsigned int)(0x38000000 + ((i - 1024) << 13));
        }

        return;
    }

    void GenerateExponentTable(void)
    {
		exponentTable[0] = 0;
        for (int i = 1; i < 31; i++)
        {
            exponentTable[i] = (unsigned int)(i << 23);
        }
        exponentTable[31] = 0x47800000;
        exponentTable[32] = 0x80000000;
        for (int i = 33; i < 63; i++)
        {
            exponentTable[i] = (unsigned int)(0x80000000 + ((i - 32) << 23));
        }
        exponentTable[63] = 0xc7800000;

        return;
    }

    void GenerateOffsetTable(void)
    {
       offsetTable[0] = 0;
        for (int i = 1; i < 32; i++)
        {
            offsetTable[i] = 1024;
        }
        offsetTable[32] = 0;
        for (int i = 33; i < 64; i++)
        {
            offsetTable[i] = 1024;
        }

        return;
    }

    float HalfToFloat(unsigned short half)
    {
        unsigned int result = mantissaTable[offsetTable[half >> 10] + (half & 0x3ff)] + exponentTable[half >> 10];
        return *((float*)&result); // Unsafe Equivalent: return *((float*)&result);
    }

}
CHalfHelper,LPCHalfHelper;

enum STMSDataType : int
{
    Position	= 0,
    Normal		= 2,
    Color		= 3,
    Binormal	= 4,
    UV_1		= 8,
    UV_2		= 9,
    UV_3		= 10,
    UV_4		= 11,
    Tangent		= 13,
    BoneWeight	= 14,
    BoneIndex	= 15,
    Index		= 0xFF
};

enum STMSCompType : int
{
        UInt16 = 0,
        Float = 1,
        Half = 2,
        Byte = 3,
        Int16 = 4,
        SByte = 6
};

typedef struct {
	int	numElements,vertexCnt,cmpBytes,un;
	// 4B,4B,4B,4B
} STMShead;

typedef struct {
	int	un1,offInVer,numConts,un2;
	STMSCompType	compressType;
	// 2B,2B,4B,4B,2B
	STMSDataType	dataType;
	// 2B
} STMSdata;

typedef class CSTMS
{
protected:

public:
	STMShead			m_stmsH;
	vector<STMSdata>	a_stmsD;
	int					m_vertexSize,m_vertexNo;
	char*				a_vertexDat;

	
	CSTMS();
	virtual ~CSTMS();
	virtual void LoadSTMS( int pos,char *pbuf );
	virtual int	 CalcVertSize( void );
	virtual int	 ConvDataType2Size( STMSDataType type );
	virtual int  CompressSize( STMSCompType ctype );
	virtual int	 Decompress( char* pbuf, int pos, STMSDataType dtype, STMSCompType ctype, int num );
	virtual float FloatDecomp( char *pbuf , int pos ,STMSCompType compType );
	virtual int  IntDecomp( char *pbuf , int pos ,STMSCompType compType );
}
CSTMS, LPCSTMS;

//======================================================================
// 頂点データ
//======================================================================
typedef struct { 
	int	nameOff,numVerts,vertsOff,weightOff,numWeight,un1,un2,un3;
	// 2B,2B,2B,2B,2B,2B,2B,2B
} ENVDhead;

typedef class CENVD
{
protected:


public:
	string			m_boneName;
	ENVDhead		m_envdH;
	vector<short>	a_verIndices;
	vector<unsigned char>	a_verWeights;

	CENVD();
	virtual ~CENVD();
	virtual void LoadENVD( int pos,char *pbuf );
}
CENVD, LPCENVD;

//======================================================================
// シェーダーデーター
//======================================================================
typedef class CHLSL
{
protected:

public:
	char	un1[16];
	string	m_Name;
	int		m_blkSize,m_strOff,m_hlslOff,un2,m_hlslLen;
	char	*p_shader;

	CHLSL();
	virtual ~CHLSL();
	virtual void LoadHLSL( int pos,char *pbuf );
}
CHLSL, LPCHLSL;


//======================================================================
// シェーダーパラメーター
//======================================================================
typedef struct{
	string		name;
//	EffectHandle	effect;
	bool		IsPShader;
	int			numVals;
	D3DXVECTOR4	defVec4;
} PARAM;

typedef struct {
	int			magic;
	int			strTblLen;
} a;

typedef class CPRAM
{
protected:

public:
	int				m_numParam,m_paraNameStart;
	int				m_numSamp,m_sampDatStart,m_sampNameStart;
	vector<int>		a_paraDatOff,a_paraNamOff,a_sampDatOff,a_sampNamOff;
	vector<PARAM>	a_params;

	CPRAM();
	virtual ~CPRAM();
	virtual void LoadPRAM( int pos,char *pbuf );
}
CPRAM, LPCPRAM;

//======================================================================
// バウンティングボックス
//======================================================================
typedef class CBOX
{
protected:

public:
	D3DXVECTOR3		m_max,m_min;
	D3DXVECTOR3		m_scale,m_offset;

	CBOX();
	virtual ~CBOX();
	virtual void LoadBOX( int pos,char *pbuf );
	virtual void CalcBOX( void );
}
CBOX, LPCBOX;


//======================================================================
// メッシュクラス
//======================================================================
typedef struct {
	int	un,pgrpCnt,stmsCnt,envdCnt,un1,un2,un3,un4,un5,shapCnt,mgmsCnt,un6,un7;
	// 1,1,1,1,4B,1,1,1,1,1,1,1,1 
} MSHhead;

#define FVF_VERTEX		(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_NORMAL | D3DFVF_NORMAL | D3DFVF_NORMAL | D3DFVF_TEX1)

static D3DVERTEXELEMENT9 VS_Formats[] = {
	{ 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	{ 0, 24, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
	{ 0, 40, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
	{ 0, 56, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
	{ 0, 72, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 80, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
D3DDECL_END()
};
// D3DDECLUSAGE_COLOR
typedef struct 
{
	D3DXVECTOR3 p;       // vertex position
	D3DXVECTOR3 n;       // vertex normal
	D3DXVECTOR4 i,w,c;
	float		u,v;
	D3DXVECTOR3 t;		// vertex tangent
} CUSTOMVERTEX ;

typedef class CMesh
{


protected:
	WORD					m_mBoneNum;
public:
IDirect3DVertexDeclaration9	*p_VertexFormat;
	unsigned long			m_IndexSize,m_VertexSize;
	unsigned long			m_NumVertices,m_NumFaces,m_NumIndex,m_VBSize,m_IBSize,m_FVF;
	LPDIRECT3DVERTEXBUFFER9 m_lpVB;
	LPDIRECT3DINDEXBUFFER9	m_lpIB;
	CHUNKhead				m_mshchunkH;
	MSHhead					m_mshH;
	string					m_mshName;
	vector<CSTMS>			a_stms;
	vector<CENVD>			a_envd;
	CBOX					m_box;

	CMesh();
	virtual ~CMesh();

	virtual void LoadMesh( int pos ,char *pbuf );
	virtual void LoadMeshHead( int pos ,char *pbuf );
}
CMesh, *LPCMesh;

//======================================================================
// ボーンクラス
//======================================================================
typedef class CBone
{

protected:

public:
	int						m_strIdx;
	D3DXVECTOR3				m_trans,m_scale;
	D3DXQUATERNION			m_rot;
	int						m_parBoneIdx,m_chdBoneIdx,
							m_sibBoneIdx,m_BoneIdx;
	D3DXMATRIX				m_matrix;
	CBone();
	virtual ~CBone();
	virtual void CalcMatrix( void );
}
CBone, *LPCBone;

//======================================================================
// ボーン配列クラス
//======================================================================
typedef class CBoneArry
{

protected:

public:
	int						m_strIdx;
	vector<float>			a_time;
	vector<D3DXVECTOR3>		a_trans,a_scale;
	vector<D3DXQUATERNION>	a_rot;
	int						m_parBoneIdx,m_chdBoneIdx,
							m_sibBoneIdx,m_BoneIdx;
	vector<D3DXMATRIX>		a_matrix;
	CBoneArry();
	virtual ~CBoneArry();
	virtual int GetQuaternion( int pos, char *pbuf,int count,int type);
	virtual int GetCompressedLinear( int pos, char *pbuf, int count,int type );
	virtual int GetConstant( int pos, char *pbuf,int count,int type);
}
CBoneArry, *LPCBoneArry;
//======================================================================
// モーションクラス
//======================================================================
typedef class CMotion
{
protected:

public:
	vector<CBoneArry> a_boneArry;
	CMotion();
	virtual ~CMotion();
}
CMotion, *LPCMotion;

//======================================================================
// テクスチャクラス
//======================================================================
typedef struct {
	int un1,un2,un3,un4;
} TexTempHead;

typedef struct {
	int				mMagic;		// GTEX
	char			un1;
	char			un2;
	char			mFormat;
	char			mMipMapCnt;
	char			un3;
	char			isCubeMap;
	unsigned short	mWidth;
	unsigned short	mHeight;
	short			mDepth;
	int				un4;
	unsigned int	mOffset;
	int				mNumLayers;
} TexHead;

typedef struct {
	unsigned int	mipmapOffset;
	int				mipmapLength;

} MipMapData;

typedef class CTexture
{

protected:

public:
	string				m_texName;
	TexTempHead			tempH;
	TexHead				texH;
	vector<MipMapData>	a_mipData;
	vector<LPDIRECT3DTEXTURE9>		a_texData;
	CTexture();
	virtual ~CTexture();
	void LoadTextureBase(int base,int pos, char *pbuf );
	LPDIRECT3DTEXTURE9 ConvertTexture( int format, int width, int height,int length,char *pbuf);
}
CTexture, *LPCTexture;

//======================================================================
// ベースデータクラス
//======================================================================
typedef class CData 
{
protected:
	D3DXMATRIX					m_mRootTransform;

public:
	CData();
	virtual ~CData();

	virtual void	InitTransform( void );
	virtual void	GetMatrix( D3DXMATRIX &mat );
	virtual void	SetMatrix( D3DXMATRIX &mat );
	virtual void	MulMatrix( D3DXMATRIX &mat );

	virtual void	Translation( float px, float py, float pz );
	virtual void	Scaling( float sx, float sy, float sz );
	virtual void	RotationX( float rot );
	virtual void	RotationY( float rot );
	virtual void	RotationZ( float rot );
	virtual void	RotationDir( float rot );
	virtual void	RotationCenter( D3DXVECTOR3 pos, float rot );
	virtual void	MirrorX( void );
	virtual void	MirrorY( void );
	virtual void	MirrorZ( void );

	virtual bool	LookAt( D3DXVECTOR3 &at, D3DXVECTOR3 &up );

	virtual void	GetWorldPosition( D3DXVECTOR3 &pos );
	virtual void	GetXAxis( D3DXVECTOR3 &pos );
	virtual void	GetYAxis( D3DXVECTOR3 &pos );
	virtual void	GetZAxis( D3DXVECTOR3 &pos );
	virtual bool	GetScreenPosition( D3DXVECTOR4 &pos );
}
CData, *LPCData;

//======================================================================
// モデルクラス
//======================================================================
typedef struct {
	int  un1,strOffset,numStr,LKs,strIdx,un2,boneLen,un3;
} SKLhead;

typedef struct {
	int			numSects,numSectNames,numChunkNames;
	vector<int> a_sectOffsets,a_sectNameOffsets,a_chunkNameOffsets;
	vector<string> a_sectNames,a_chunkNames;
} MotHead;

typedef struct {
	float	Float1;
	int		numBones,numIndices;
	int		flags;
	vector<int>	a_index,a_flag;
} MotMtbHead;

typedef struct {
	int		offset,length,numChildren,flag;
} MotSpuChunk;

typedef struct {
	string	name;
	int		idx,offset,numChunks;
	vector<MotSpuChunk> a_spuChunks;
} MotSpuSection;

typedef struct {
	int		sectLength;
	int		numBones,numSections;
	vector<MotSpuSection>	a_spuSections;
} MotSpuHead;

typedef struct {
	vector<int> a_resIdx,a_resOff,a_resSiz,a_resTyp;
} MotResTbl;

typedef class CModel : public CData
{
protected:

public:
	int						m_nBone,m_sklPos,m_motPos; //boneのよみこみ位置
	SECThead				m_sklsectH,m_motsectH; // boneのセクションヘッダ
	SKLhead					m_sklH;			// boneのヘッダ
	MotHead					m_motH;
	MotMtbHead				m_motMtbH;
	MotSpuHead				m_motSpuH;
	vector<CBoneArry>		a_Motions;
	vector<string>			a_sklstrTables;
	vector<CBone>			a_Bones;		// boneの配列
	vector<CMesh>			a_Meshes;
	vector<CHLSL>			a_hlsls;
	vector<CTexture>		a_tex;
	CBOX					m_box,m_comp;
	CPRAM					m_pram;

	CModel();
	virtual ~CModel();
	virtual void    LoadBoneBase( int pos, char *pbuf );
	virtual void    LoadMeshBase( int pos, char *pbuf );
	virtual void    LoadShaderBase( int pos, char *pbuf );
	virtual void	LoadSpuBinaryDetail( int pos, char *pbuf );
	virtual void	LoadMtbHeader( int pos, char *pbuf );
	virtual void	LoadSpuBinary( int pos, char *pbuf );
	virtual void	LoadMtbGeneral( int pos, char *pbuf );
	virtual void    LoadMotionBase( int pos, char *pbuf );
	virtual HRESULT LoadTextureFromFile( string filename );
	virtual HRESULT LoadBoneFromFile( string filename );
	virtual HRESULT LoadMeshFromFile( string filename );
	virtual HRESULT LoadShaderFromFile( string filename );
	virtual HRESULT LoadMotionFromFile( string filename );
} CModel, *LPCModel;

//======================================================================
// NPCクラス
//======================================================================
typedef class CNPC : public CModel
{
protected:
	int			m_mBody;	// 種族
public:
	string		m_mdlPath;
	CNPC();
	virtual ~CNPC();
	virtual void SetModelPath( string path ) { m_mdlPath = path; }
	virtual string GetModelPath( void ) { return m_mdlPath; }
	virtual	bool	LoadNPC( void );
} CNPC,*LPCNPC;