// INCLUDE
#define _CRT_SECURE_NO_WARNINGS
#include <d3d9.h>
#include <d3dx9.h>
#include "Render.h"
#include "Model.h"
//Function

HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf );
HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage );
// DEFINE
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }
#define SAFE_DELETES(p)		if ( (p) != NULL ) { delete [] (p); (p) = NULL; }
#define SAFE_DELETE(p)		if ( (p) != NULL ) { delete (p); (p) = NULL; }
#define PAI					(3.1415926535897932384626433832795f)
#define PAI2				(3.1415926535897932384626433832795f*2.0f)

static const D3DXMATRIX matrixMirrorX(-1.0f,0,0,0,  0, 1.0f,0,0,  0,0, 1.0f,0,  0,0,0,1.0f);
static const D3DXMATRIX matrixMirrorY( 1.0f,0,0,0,  0,-1.0f,0,0,  0,0, 1.0f,0,  0,0,0,1.0f);
static const D3DXMATRIX matrixMirrorZ( 1.0f,0,0,0,  0, 1.0f,0,0,  0,0,-1.0f,0,  0,0,0,1.0f);
// グローバル
extern	CGlobalSet		Glob;
extern	BOOL			g_mIsUseSoftware;
CHalfHelper				g_halfHelp;

// バイトスワップ

float SWAPF( char *val )
{
	float dat;
	char *pd;

	pd = (char*)&dat;
	*pd = *(val+3); *(pd+1) = *(val+2);
	*(pd+2) = *(val+1); *(pd+3) = *val;
	return ( dat ) ;
}

int SWAP4( int val )
{
	int dat;
	char *pv,*pd;

	pv = (char*)&val; pd = (char*)&dat;
	*pd = *(pv+3); *(pd+1) = *(pv+2);
	*(pd+2) = *(pv+1); *(pd+3) = *pv;
	return ( dat ) ;
}

short SWAP2( short val )
{
	short dat;
	char *pv,*pd;

	pv = (char*)&val; pd = (char*)&dat;
	*pd = *(pv+1); *(pd+1) = *pv;
	return ( dat ) ;
}

//		CSection
CSection::CSection()
{
	m_resPos = 0;
	m_resNum = 0;
	a_resInfos.clear();
	a_resTypes.clear();
	a_resIDs.clear();
	a_strTables.clear();
	a_Resources.clear();
}

CSection::~CSection()
{
	m_resPos = 0;
	m_resNum = 0;
	a_resInfos.clear();
	a_resTypes.clear();
	a_resIDs.clear();
	a_strTables.clear();
	a_Resources.clear();
}

void CSection::LoadRes(int pos, char *pbuf)
{
	int num = 0;
	num += LoadSectionHead( pos+num, pbuf);
	num += LoadResourceHead( pos+num, pbuf);
	num += LoadResourceInfo( pos+num, pbuf);
	m_resPos = pos+num;
	LoadResourceTypes( pos+num, pbuf);
	LoadResourceIDs( pos+num, pbuf);
	LoadStringTables( pos+num, pbuf);
	LoadResources( pos+num, pbuf);
}

int CSection::LoadSectionHead(int pos, char *pbuf)
{
	int num=0;
	m_sectH.magicNumber	= *( (MagicNumber *)(pbuf+pos+num));num+=sizeof( long long );
	m_sectH.version		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sectH.un			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sectH.sectSize	= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	memcpy(&m_sectH.junk,pbuf+pos+num,28);num+=28;
	return( num );
}

int CSection::LoadResourceHead(int pos, char *pbuf)
{
	int num=0;
	m_resH.numRes		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_resH.strOffset	= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_resH.numStr		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_resH.resType		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	return( num );
}

int CSection::LoadResourceInfo(int pos, char *pbuf)
{
	int     num=0;
	RESinfo	resinfo;

	for( int i=0 ; i<m_resH.numRes ; i++ ) {
		resinfo.index		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
		resinfo.offset		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
		resinfo.size		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
		resinfo.type		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
		resinfo.chidx		= -1;

		a_resInfos.push_back( resinfo );
	}
	return( num );
}

int CSection::LoadResourceTypes(int pos, char *pbuf)
{
	int     num=0;
	SectionType type;

	pos   = m_resPos + a_resInfos[m_resH.numRes - 2].offset;
      
	for( int i=0 ; i<m_resH.numRes ; i++ ) {
		type				= *( (SectionType *)(pbuf+pos+num));num+=sizeof(int);
		a_resTypes.push_back( type );
	}
	return( num );
}

int CSection::LoadResourceIDs(int pos, char *pbuf)
{
	char	str[16];
	int     num=0;
	string	ID;

	pos   = m_resPos + a_resInfos[m_resH.numRes - 1].offset;
	for( int i=0 ; i<m_resH.numRes ; i++ ) {
		memcpy(str,pbuf+pos+num,0x10); num+=0x10;
		ID.append(str);
		a_resIDs.push_back( ID );
		ID.clear();
	}
	return( num );
}

int CSection::LoadStringTables(int pos, char *pbuf)
{
	int     num=0;
	string	str;

	pos = m_resPos +m_resH.strOffset;
	for( int i=0 ; i<m_resH.numStr ; i++ ) {
		str.append(pbuf+pos+num);
		a_strTables.push_back( str );
		num += str.length()+1;
		str.clear();
	}
	return( num );
}

int CSection::LoadResources(int pos, char *pbuf)
{
	int num=0;
	MagicNumber mg;
	pos = m_resPos;
	for( int i=0 ; i<m_resH.numRes ; i++ ) {
		if( a_resInfos[i].type == 0 ) continue;
		if( a_resInfos[i].size == 0 ) continue;
		if( a_resTypes[i] == 0 ) continue;
		num = a_resInfos[i].offset;
		mg = ChangeSectionType(a_resTypes[i]);
		if( mg == Resource ) {
			CSection *pDir = new CSection;
			pDir->LoadRes( pos+num , pbuf);
			a_Resources.push_back( *pDir );	
			a_resInfos[i].chidx = m_resNum++;
			SAFE_DELETE( pDir );
		} else if( mg==Texture ){
		}
	}
	return( num );
}

MagicNumber CSection::ChangeSectionType( SectionType type ) {
	MagicNumber mnum;

	switch ( type ) {
		case bin:
		case trb:
			mnum = Resource;
			break;
		case txb:
			mnum = Texture;
			break;
		case sscf:
			mnum = Sound;
			break;
		case phb:
			mnum = Phb;
			break;
		case mtb:
			mnum = Mtb;
			break;
		case veff:
			mnum = Veff;
			break;
		case vins:
			mnum = Vins;
			break;
		case vmdl:
			mnum = Vmdl;
			break;
		case vtex:
			mnum = Vtex;
			break;
		case leaf:
			mnum = Leaf;
			break;
		default:
			mnum = UnMagic;
	}
	return( mnum );
}

int CSection::GetSectionStart( SectionType type ) {
	int ret=0;

	for( int i=0 ; i<m_resH.numRes ; i++ ) {
		if( a_resInfos[i].chidx<0 ) {
			if( a_resTypes[i] == type ) {
				return( m_resPos + a_resInfos[i].offset );
			}
		} else {
			ret=a_Resources[a_resInfos[i].chidx].GetSectionStart( type );
			if( ret>0 ) return( ret );
		}
	}
	return( ret );
}

int CSection::GetSectionStart( SectionType type,string resid ) {
	int ret=0;

	for( int i=0 ; i<m_resH.numRes ; i++ ) {
		if( a_resInfos[i].chidx<0 ) {
			if( a_resTypes[i] == type && a_resIDs[i]==resid ) {
				return( m_resPos + a_resInfos[i].offset );
			}
		} else {
			ret=a_Resources[a_resInfos[i].chidx].GetSectionStart( type );
			if( ret>0 ) return( ret );
		}
	}
	return( ret );
}

//		CChunk
CChunk::CChunk()
{
	m_chunkPos = 0;
	memset(&m_chunkH,0,sizeof(CHUNKhead));
	memset(&m_chunkD,0,sizeof(CHUNKdata));
	a_Chunks.clear();
}

CChunk::~CChunk()
{
	m_chunkPos = 0;
	a_Chunks.clear();
}

void CChunk::LoadChunk(ChunkType type,int pos, char *pbuf)
{
	int num = 0;
	num += LoadChunkHead( pos+num, pbuf);
	if( !IsChilds( m_chunkH.chunkType ) ) {
		m_chunkD.numChild=0;
		return;
	} else if( m_chunkH.chunkType == type ) {
		m_chunkD.numChild=0;
		return;
	}
	num += LoadChunkData( pos+num, pbuf);
	m_chunkPos = pos+num;
	for( int i=0,off=m_chunkPos ; i<m_chunkD.numChild ; i++ ) {
		CChunk *pchunk = new CChunk;
		pchunk->LoadChunk(m_chunkH.chunkType,off,pbuf);
		a_Chunks.push_back( *pchunk );
		off += pchunk->m_chunkH.nextChunk;
		SAFE_DELETE( pchunk );
	}

}

int CChunk::GetChunkOffset( ChunkType type ) {
	int ret=0;

	if( m_chunkH.chunkType == type ) {
		return ( m_chunkPos-0x20 );
	} else {
		for( int i=0 ; i<m_chunkD.numChild ; i++ ) {
			if( (ret=a_Chunks[i].GetChunkOffset( type ))>0 ) return( ret );
		}
	}
	return( ret );
}

bool CChunk::IsChilds( ChunkType type ) {
	bool ret=false;
	switch( type ) {
		case WRB:
		case SHD:
		case MDL:
		case MDLC:
		case MESH:
		case AABB:
		   ret = true;
		   break;
		default:
		   ret = false;
	}
	return( ret );
}

int CChunk::LoadChunkHead(int pos, char *pbuf)
{
	int num=0;
	m_chunkH.chunkType	= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	m_chunkH.un			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_chunkH.dataSize	= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_chunkH.nextChunk	= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	return( num );
}

int CChunk::LoadChunkData(int pos, char *pbuf)
{
	int num=0;
	m_chunkD.numChild	= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_chunkD.un1		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_chunkD.un2		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_chunkD.un3		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	return( num );
}

//		CSTMS
CSTMS::CSTMS()
{
	m_vertexSize = 0;
	m_vertexNo   = 0;
	a_vertexDat = NULL;
	memset(&m_stmsH,0,sizeof(STMShead));
}

CSTMS::~CSTMS()
{
	m_vertexSize = 0;
	memset(&m_stmsH,0,sizeof(STMShead));
	a_stmsD.clear();
	a_vertexDat = NULL;
	//SAFE_DELETES(a_vertexDat);
}

void CSTMS::LoadSTMS( int pos,char *pbuf )
{
	CHUNKhead	chunkH;
	int num=0,off=0;
	// チャンクヘッド
	chunkH.chunkType		= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	chunkH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.dataSize			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.nextChunk		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// STMSヘッダー
	m_stmsH.numElements		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_stmsH.vertexCnt		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_stmsH.cmpBytes		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_stmsH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// 要素の読み込み
	for( int i=0 ; i<m_stmsH.numElements ; i++ ) {
		STMSdata *pstms = new STMSdata;
		pstms->un1			= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
		pstms->offInVer		= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
		pstms->compressType	= (STMSCompType)SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
		pstms->numConts		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
		pstms->dataType		= (STMSDataType)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
		pstms->un2			= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
		a_stmsD.push_back( *pstms );
		SAFE_DELETE( pstms );
	}
	pos = pos+num; num = 0;
	m_vertexNo = 0;
	m_vertexSize = CalcVertSize();
	a_vertexDat = new char[m_vertexSize * m_stmsH.vertexCnt];
	for( int i=0 ; i<m_stmsH.vertexCnt ; i++ ) {
		for( int j=0 ; j<m_stmsH.numElements ; j++ ) {
			num += Decompress( pbuf,pos+a_stmsD[j].offInVer,a_stmsD[j].dataType,a_stmsD[j].compressType,a_stmsD[j].numConts );
		}
		pos += m_stmsH.cmpBytes;
	}
	return;
}

#define MAXBONESIZE 4		// 4の間違い？

int CSTMS::CalcVertSize()
{
    int result = 0;
	for( int i=0 ; i<m_stmsH.numElements ; i++ ) {
       //if ( a_stmsD[i].dataType == BoneIndex || a_stmsD[i].dataType == BoneWeight) {
       //     // Number of bones per vert may vary so calculate the real size used (4 bytes per bone weight or bone index)
       //     result += min(a_stmsD[i].numConts, MAXBONESIZE ) * 4;
       // } else {
            result += ConvDataType2Size( a_stmsD[i].dataType );
        //}
    }

    return result;
}

int CSTMS::ConvDataType2Size( STMSDataType type )
{
	switch( type ) {
		case Position:
		case Normal:
		case Tangent:
			return 4*3;
		case Color:
		//case Binormal:
		case BoneWeight:
			return 4*4;
		case UV_1:
		case UV_2:
		case UV_3:
		case UV_4:
			return 4*2;
		case BoneIndex:
			return 4*4;
		case Index:
			return 2;
		default:
			return 0;
	}
	return 0;
}

int  CSTMS::CompressSize( STMSCompType ctype )
{
	switch( ctype ) {
	case UInt16:
		return 2;
	case Float:
		return 4;
	case Half:
		return 2;
	case Byte:
		return 1;
	case Int16:
		return 2;
	case SByte:
		return 1;
	default:
		return 0;
	}
	return 0;
}

int CSTMS::Decompress( char* pbuf, int pos, STMSDataType dtype, STMSCompType ctype, int num )
{
	int				size = 0;
	unsigned short	uval2;
	float			val4;

	switch( dtype ) {
		case Position:
		case Normal:
		case Tangent:
			 for ( int i=0 ; i<num ; i++ ) {
				val4 = FloatDecomp( pbuf , pos+size , ctype );
				if( i<3 ) {
					memcpy((void*)&a_vertexDat[m_vertexNo],(void*)&val4,sizeof(float));
					m_vertexNo+=sizeof(float);
					size += CompressSize( ctype );
				}
			 }
			 return size;
		case Color:
			 for ( int i=0 ; i<num ; i++ ) {
				val4 = FloatDecomp( pbuf , pos+size , ctype );
				memcpy((void*)&a_vertexDat[m_vertexNo],(void*)&val4,sizeof(float));
				m_vertexNo+=sizeof(float);
				size += CompressSize( ctype );
			 }
			 return size;		//case Binormal:
		case BoneWeight:
		case UV_1:
		case UV_2:
		case UV_3:
		case UV_4:
			 for ( int i=0 ; i<num ; i++ ) {
				val4 = FloatDecomp( pbuf , pos+size , ctype );
				memcpy((void*)&a_vertexDat[m_vertexNo],(void*)&val4,sizeof(float));
				m_vertexNo+=sizeof(float);
				size += CompressSize( ctype );
			 }
			 return size;
		case BoneIndex:
			 for ( int i=0 ; i<num ; i++ ) {
				val4 = (float)IntDecomp( pbuf , pos+size , ctype );
				memcpy((void*)&a_vertexDat[m_vertexNo],(void*)&val4,sizeof(float));
				m_vertexNo+=sizeof(float);
				size += CompressSize( ctype );
			 }
			 return size;
		case Index:
			for( int i=0 ; i<num ; i++ ) {
				uval2 = (unsigned short)SWAP2(*((short *)(pbuf+pos+size)));
				memcpy((void*)&a_vertexDat[m_vertexNo],(void*)&uval2,sizeof(short));
				m_vertexNo+=sizeof(unsigned short);
				size += CompressSize( ctype );
			}
			return size;
		default:
			return size;

	}
	return size;
}

float CSTMS::FloatDecomp( char* pbuf, int pos, STMSCompType ctype )
{
	unsigned char  uval1;
	unsigned short uval2;
	short			val2;
	float			val4;
	char			val1;
	switch( ctype ) {
		case UInt16 :
			uval2 = (unsigned short)SWAP2(*((short*)(pbuf+pos)));
			return (float)uval2/65535.f;
		case Float :
			val4 = SWAPF((pbuf+pos));
			return val4;
		case Half:
			uval2 = (unsigned short)SWAP2(*((short*)(pbuf+pos)));
			return  g_halfHelp.HalfToFloat(uval2);
		case Byte:
			uval1 = *((unsigned char*)(pbuf+pos));
			return (float)uval1/255.f;
		case Int16:
			val2 = SWAP2(*((short*)(pbuf+pos)));
			return (float)val2/32768.f;
		case SByte :
			val1 = *((char*)(pbuf+pos));
			return (float)val1/127.f;
		default:
			return 0.f;
	}
	return 0.f;
}

int CSTMS::IntDecomp( char *pbuf , int pos ,STMSCompType ctype )
{
	unsigned char  uval1;
	unsigned short uval2;
	short			val2;
	char			val1;

	switch( ctype ) {
		case UInt16 :
			uval2 = (unsigned short)SWAP2(*((short*)(pbuf+pos)));
			return (int)uval2;
		case Byte:
			uval1 = *((unsigned char*)(pbuf+pos));
			return (int)uval1;
		case Int16:
			val2 = SWAP2(*((short*)(pbuf+pos)));
			return (int)val2;
		case SByte :
			val1 = *((char*)(pbuf+pos));
			return (int)val1;
		default:
			return 0;
	}
	return 0;
}

CHalfHelper::CHalfHelper()
{
	mantissaTable = new unsigned int[2048];
	exponentTable = new unsigned int[64];
	offsetTable = new unsigned short[64];
	GenerateMantissaTable();
	GenerateExponentTable();
	GenerateOffsetTable();
}

CHalfHelper::~CHalfHelper()
{
}

//			CENVD
CENVD::CENVD()
{
	m_boneName.clear();
	memset(&m_envdH,0,sizeof(ENVDhead));
	//a_verIndices.clear();
	//a_verWeights.clear();
}

CENVD::~CENVD()
{
	m_boneName.clear();
	memset(&m_envdH,0,sizeof(ENVDhead));
	a_verIndices.clear();
	a_verWeights.clear();
}

void CENVD::LoadENVD( int pos,char *pbuf )
{
	CHUNKhead	chunkH;
	int num=0,off=0;
	// チャンクヘッド
	chunkH.chunkType		= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	chunkH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.dataSize			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.nextChunk		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// ENVDヘッダー
	m_envdH.nameOff			= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.numVerts		= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.vertsOff		= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.weightOff		= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.numWeight		= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.un1				= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.un2				= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_envdH.un3				= (int)SWAP2(*( (unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
	m_boneName.append( pbuf+pos+0x10+m_envdH.nameOff );
	num = 0x10+m_envdH.vertsOff;
	for( int i=0 ; i<m_envdH.numVerts ; i++ ) {
		short index = SWAP2(*( (short *)(pbuf+pos+num)));num+=sizeof(short);
		a_verIndices.push_back( index );
	}
	num = 0x10+m_envdH.weightOff;
	for( int i=0 ; i<m_envdH.numWeight ; i++ ) {
		unsigned char weight =*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
		a_verWeights.push_back( weight );
	}
	return;
}

//			CHLSL
CHLSL::CHLSL()
{
	memset(un1,0,sizeof(un1));
	m_Name.clear();
	m_blkSize=m_strOff=m_hlslOff=un2=m_hlslLen=0;
	p_shader = NULL;
}

CHLSL::~CHLSL()
{
	memset(un1,0,sizeof(un1));
	m_Name.clear();
	m_blkSize=m_strOff=m_hlslOff=un2=m_hlslLen=0;
	p_shader = NULL;
}

void CHLSL::LoadHLSL( int pos,char *pbuf )
{
	CHUNKhead	chunkH;
	int num=0,off=0;
	// チャンクヘッド
	chunkH.chunkType		= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	chunkH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.dataSize			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.nextChunk		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// HLSLヘッダー
	m_blkSize				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_strOff				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_hlslOff				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_Name.append(pbuf+pos+num);
	num = m_hlslOff+0x10;
	memcpy(un1,pbuf+pos+num,sizeof(un1));num+=sizeof(un1);
	m_hlslLen				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	un2						= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	num = m_hlslOff+0x20;
	p_shader = new char[m_hlslLen];
	memcpy(p_shader,pbuf+pos+num,m_hlslLen);
	return;
}

//			CPRAM
CPRAM::CPRAM()
{
	m_numParam=m_paraNameStart=0;
	m_numSamp=m_sampDatStart=m_sampNameStart=0;
	//a_paraDatOff.clear();
	//a_paraNamOff.clear();
	//a_sampDatOff.clear();
	//a_sampNamOff.clear();
	//a_params.clear();
}

CPRAM::~CPRAM()
{
	m_numParam=m_paraNameStart=0;
	m_numSamp=m_sampDatStart=m_sampNameStart=0;
	a_paraDatOff.clear();
	a_paraNamOff.clear();
	a_sampDatOff.clear();
	a_sampNamOff.clear();
	a_params.clear();
}

void CPRAM::LoadPRAM( int pos,char *pbuf )
{
	int		un;
	float	unf;

	CHUNKhead	chunkH;
	int num=0,off=0;
	// チャンクヘッド
	chunkH.chunkType		= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	chunkH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.dataSize			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.nextChunk		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// PRAMヘッダー
	un	= (int)*((short *)(pbuf+pos+num));num+=sizeof(short);
	un	= (int)*((short *)(pbuf+pos+num));num+=sizeof(short);
	unf = *((float *)(pbuf+pos+num));num+=sizeof(float);
	m_numParam  = *((int *)(pbuf+pos+num));num+=sizeof(int);
	un			= *((int *)(pbuf+pos+num));num+=sizeof(int);
	m_paraNameStart = *((int *)(pbuf+pos+num));num+=sizeof(int);
	m_numSamp = *((int *)(pbuf+pos+num));num+=sizeof(int);
	m_sampDatStart = *((int *)(pbuf+pos+num));num+=sizeof(int);
	m_sampNameStart = *((int *)(pbuf+pos+num));num+=sizeof(int);
	for( int i=0 ; i<m_numParam ; i++ ) {
		un = *((int *)(pbuf+pos+num));num+=sizeof(int);
		a_paraDatOff.push_back( un );
	}
	for( int i=0 ; i<m_numParam ; i++ ) {
		un = *((int *)(pbuf+pos+num));num+=sizeof(int);
		a_paraNamOff.push_back( un );
	}
	for( int i=0 ; i<m_numParam ; i++ ) {
		un = *((int *)(pbuf+pos+num));num+=sizeof(int);
		a_sampDatOff.push_back( un );
	}
	for( int i=0 ; i<m_numParam ; i++ ) {
		un = *((int *)(pbuf+pos+num));num+=sizeof(int);
		a_sampNamOff.push_back( un );
	}
	return;
}

//		CBOX
CBOX::CBOX()
{
	m_max.x=m_max.y = m_max.z=0.f;
	m_min.x=m_min.y = m_min.z=0.f;
}

CBOX::~CBOX()
{
	m_max.x=m_max.y = m_max.z=0.f;
	m_min.x=m_min.y = m_min.z=0.f;
	m_scale.x=m_scale.y = m_scale.z=1.f;
	m_offset.x=m_offset.y = m_offset.z=0.f;
}

void CBOX::LoadBOX( int pos,char *pbuf )
{
	CHUNKhead	chunkH;
	int num=0,off=0;
	// チャンクヘッド
	chunkH.chunkType		= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	chunkH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.dataSize			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.nextChunk		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// BOXヘッダー
	m_min.x = SWAPF((pbuf+pos+num));num+=sizeof(float);
	m_min.y = SWAPF((pbuf+pos+num));num+=sizeof(float);
	m_min.z = SWAPF((pbuf+pos+num));num+=sizeof(float);
	m_max.x = SWAPF((pbuf+pos+num));num+=sizeof(float);
	m_max.y = SWAPF((pbuf+pos+num));num+=sizeof(float);
	m_max.z = SWAPF((pbuf+pos+num));num+=sizeof(float);
	return;
}

void CBOX::CalcBOX( void )
{
    m_scale.x = (m_max.x - m_min.x) / 2.f;
    m_scale.y = (m_max.y - m_min.y) / 2.f;
    m_scale.z = (m_max.z - m_min.z) / 2.f;

    m_offset.x = m_max.x - m_scale.x;
    m_offset.y = m_max.y - m_scale.y;
    m_offset.z = m_max.z - m_scale.z;

	return;
}

//		CMesh
CMesh::CMesh()
{
	m_mBoneNum				= 0;
	m_FVF					= FVF_VERTEX;
	GetDevice()->CreateVertexDeclaration( VS_Formats, &p_VertexFormat );
	m_IndexSize=m_VertexSize=0;
	m_NumVertices=m_NumFaces=m_NumIndex=m_VBSize=m_IBSize=m_FVF=0;
	m_lpVB=NULL;
	m_lpIB=NULL;
	memset(&m_mshchunkH,0,sizeof(CHUNKhead));
	memset(&m_mshH,0,sizeof(MSHhead));
	m_mshName.clear();
	//a_stms.clear();
	//a_envd.clear();
	m_box.~CBOX();
}

CMesh::~CMesh()
{
	m_mBoneNum		= 0;
	p_VertexFormat=NULL;
	m_IndexSize=m_VertexSize=0;
	m_NumVertices=m_NumFaces=m_NumIndex=m_VBSize=m_IBSize=m_FVF=0;
	//SAFE_DELETE(m_lpVB);
	//SAFE_DELETE(m_lpIB);
	m_IndexSize=m_VertexSize=0;
	m_NumVertices=m_NumFaces=m_NumIndex=m_VBSize=m_IBSize=m_FVF=0;
	m_lpVB=NULL;
	m_lpIB=NULL;
	memset(&m_mshchunkH,0,sizeof(CHUNKhead));
	memset(&m_mshH,0,sizeof(MSHhead));
	m_mshName.clear();
	a_stms.clear();
	a_envd.clear();
	m_box.~CBOX();
}

//		メッシュヘッダー読み込み
void CMesh::LoadMeshHead( int pos, char *pbuf )
{
	CHUNKhead	chunkH;
	int num=0,off=0;
	// チャンクヘッド
	chunkH.chunkType		= *( (ChunkType *)(pbuf+pos+num));num+=sizeof(int);
	chunkH.un				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.dataSize			= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	chunkH.nextChunk		= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	// メッシュヘッダー
	m_mshH.un				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.pgrpCnt			= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.stmsCnt			= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.envdCnt			= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.un1				= SWAP4(*( (int *)(pbuf+pos+num)));num+=sizeof(int);
	m_mshH.un2				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.un3				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.un4				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.un5				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.shapCnt			= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.mgmsCnt			= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.un6				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_mshH.un7				= (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
}

//		メッシュ読み込み
void CMesh::LoadMesh( int pos, char *pbuf )
{
	int num=0,off=0;

	CChunk *pChunk = new CChunk;

	// MESHチャンクデータ読み込み
	pChunk->LoadChunk( MDL,pos,pbuf );

	//　子チャンク中のMESHチャンクを読み込み
	pos=pChunk->m_chunkPos;
	for( int i=0 ; i<pChunk->m_chunkD.numChild ; i++ ) {
		if( pChunk->a_Chunks[i].m_chunkH.chunkType==HEAD ) {
			LoadMeshHead( pos , pbuf );
		} else if( pChunk->a_Chunks[i].m_chunkH.chunkType==STR ) {
			m_mshName.append((char*)(pbuf+pos+0x10));
		} else if( pChunk->a_Chunks[i].m_chunkH.chunkType==STMS ) {
			CSTMS *pstms = new CSTMS;
			pstms->LoadSTMS( pos,pbuf );
			a_stms.push_back( *pstms );
			SAFE_DELETE( pstms );
		} else if( pChunk->a_Chunks[i].m_chunkH.chunkType==ENVD ) {
			CENVD *penvd = new CENVD;
			penvd->LoadENVD( pos,pbuf );
			a_envd.push_back( *penvd );
			SAFE_DELETE( penvd );
		} else if( pChunk->a_Chunks[i].m_chunkH.chunkType==AABB ) {
			CChunk *pchk = new CChunk;
			pchk->LoadChunk( pChunk->m_chunkH.chunkType,pos,pbuf );// AABBチャンクデータ読み込み
			for( int j=0,tpos=pchk->m_chunkPos ; j<pchk->m_chunkD.numChild ; j++ ) {
				if( pchk->a_Chunks[j].m_chunkH.chunkType==AABB ) {
					m_box.LoadBOX( tpos,pbuf );
					m_box.CalcBOX();
				}
				tpos += pchk->a_Chunks[j].m_chunkH.nextChunk;
			}
			SAFE_DELETE( pchk );
		}
		pos+=pChunk->a_Chunks[i].m_chunkH.nextChunk;
	}
	SAFE_DELETE( pChunk );

	// 頂点バッファへのセット
	bool indexF=true,vertexF=true;
	WORD	*pIdst,*pIsrc;
	CUSTOMVERTEX	*pVsrc,*pVdst;
	for( int i=0 ; i< (int)a_stms.size() ; i++ ) {
		switch( a_stms[i].m_vertexSize ) {
			case 2:
				if( indexF ) {
					m_IndexSize = a_stms[i].m_vertexSize;
					m_NumIndex = a_stms[i].m_stmsH.vertexCnt;m_NumFaces = m_NumIndex/3;
					m_IBSize = a_stms[i].m_vertexSize * sizeof(WORD);
					CreateIB( &m_lpIB, m_NumIndex*sizeof(WORD), 0 );
					if( FAILED( m_lpIB->Lock( 0,m_IBSize,(void**)&pIdst,D3DLOCK_DISCARD ) ) )  break;
					pIsrc = (WORD*)a_stms[i].a_vertexDat;
					for( int j=0 ; j<(int)m_NumIndex ; j++ ) {
						*pIdst++ = *pIsrc++;
					}
					if( FAILED( m_lpIB->Unlock() ) ) {
						break;
					}
					indexF = false;
				}
				break;
			default:
				if( vertexF ) {
					m_VertexSize = a_stms[i].m_vertexSize;
					m_NumVertices = a_stms[i].m_stmsH.vertexCnt;
					m_VBSize = m_VertexSize * m_VertexSize;
					if( FAILED(CreateVB( &m_lpVB, m_NumVertices*m_VertexSize,0, FVF_VERTEX ) ) ) break;
					//m_VBSize = m_VertexSize * D3DXGetFVFVertexSize(FVF_VERTEX);
					//if( FAILED(CreateVB( &m_lpVB, m_NumVertices*D3DXGetFVFVertexSize(FVF_VERTEX),0, FVF_VERTEX ) ) ) break;
					if( FAILED( m_lpVB->Lock( 0,m_VBSize,(void**)&pVdst,D3DLOCK_DISCARD ) ) ) break;
					pVsrc = (CUSTOMVERTEX*)a_stms[i].a_vertexDat;
					for( int j=0 ; j<(int)m_NumVertices ; j++ ) {
						*pVdst++ = *pVsrc++;
					}
					if( FAILED( m_lpVB->Unlock() ) ) {
						break;
					}
					vertexF = false;
				}
				break;
		}
	}
		
}

//		CTexture
CTexture::CTexture()
{
	memset(&tempH,0,sizeof(TexTempHead));
	memset(&texH,0,sizeof(TexHead));
	a_mipData.clear();
	a_texData.clear();
}

CTexture::~CTexture()
{
	memset(&tempH,0,sizeof(TexTempHead));
	memset(&texH,0,sizeof(TexHead));
	a_mipData.clear();
	a_texData.clear();
}

void CTexture::LoadTextureBase(int base, int pos, char *pbuf )
{
	int	num=0;

	CSection *pSect = new CSection;
	num += pSect->LoadSectionHead(pos+num,pbuf);
	SAFE_DELETE(pSect);
	tempH.un1 = *( (int *)(pbuf+pos+num));num+=sizeof(int);
	tempH.un2 = *( (int *)(pbuf+pos+num));num+=sizeof(int);
	tempH.un3 = *( (int *)(pbuf+pos+num));num+=sizeof(int);
	tempH.un4 = *( (int *)(pbuf+pos+num));num+=sizeof(int);
	texH.mMagic		=  *( (int *)(pbuf+pos+num));num+=sizeof(int);
	texH.un1		=  *(pbuf+pos+num);num+=sizeof(char);
	texH.un2		=  *(pbuf+pos+num);num+=sizeof(char);
	texH.mFormat	=  *(pbuf+pos+num);num+=sizeof(char);
	texH.mMipMapCnt	=  *(pbuf+pos+num);num+=sizeof(char);
	texH.un3		=  *(pbuf+pos+num);num+=sizeof(char);
	texH.isCubeMap	=  *(pbuf+pos+num);num+=sizeof(char);
	texH.mWidth		=  (unsigned short)SWAP2(*((short *)(pbuf+pos+num)));num+=sizeof(short);
	texH.mHeight	=  (unsigned short)SWAP2(*((short *)(pbuf+pos+num)));num+=sizeof(short);
	texH.mDepth		=  SWAP2(*((short *)(pbuf+pos+num)));num+=sizeof(short);
	texH.un4		=  SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
	texH.mOffset	=  (unsigned int)SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
	if( texH.isCubeMap ) {
		texH.mNumLayers = 6;
	} else {
		texH.mNumLayers = texH.mMipMapCnt;
	}
	for( int i=0 ; i<texH.mNumLayers ; i++ ) {
		MipMapData *pmipmap = new MipMapData;
		pmipmap->mipmapOffset = (unsigned int)SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
		pmipmap->mipmapLength = SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
		a_mipData.push_back(*pmipmap);
		SAFE_DELETE(pmipmap);
	}
	if( texH.mOffset<=0 ) {
		if( base >0 ) {
			texH.mOffset = base;
		} else {
			unsigned int offset = 24 + texH.mNumLayers*8;
            if (offset % 16 == 0) {
                texH.mOffset = offset;
            } else {
                texH.mOffset = ((offset >> 5) + 1) << 5;
            }
		}
	}
	for( int i=0 ; i<(int)a_mipData.size() ; i++ ) {
		LPDIRECT3DTEXTURE9 ptex = ConvertTexture((int)texH.mFormat,(int)texH.mWidth,
			(int)texH.mHeight,(int)a_mipData[i].mipmapLength,pbuf+texH.mOffset+a_mipData[i].mipmapOffset);
		a_texData.push_back( ptex );
	}
	//for( int i=0 ; i<(int)a_mipData.size() ; i++ ) {
	//	char *ptex = new char[a_mipData[i].mipmapLength];
	//	memcpy(ptex,pbuf+texH.mOffset+a_mipData[i].mipmapOffset,a_mipData[i].mipmapLength);
	//	a_texData.push_back( ptex );
	//}
}

LPDIRECT3DTEXTURE9 CTexture::ConvertTexture( int format, int width, int height,int length,char *pbuf)
{
	HRESULT hr	= S_OK;
	LPDIRECT3DTEXTURE9 pTex;

    //0x03: Format.A8R8G8B8;
    //0x04: Format.X8R8G8B8;
    //0x18: Format.Dxt1;
    //0x19: Format.Dxt3;
    //0x1A: Format.Dxt5;

	D3DLOCKED_RECT rc;

	if( format == 0x19 ){
		hr = GetDevice()->CreateTexture(width,height,0,0 ,D3DFMT_DXT3,D3DPOOL_MANAGED,&pTex,NULL);
		if( hr!=D3D_OK ) return NULL;
		hr = pTex->LockRect(0,&rc,NULL,0);
		if(hr==D3D_OK){
			CopyMemory(rc.pBits,pbuf,length );
			//CopyMemory(rc.pBits,pbuf,(width/4) * (height/4) * 16 );
			pTex->UnlockRect(0);
		}
	} else if( format == 0x18 ){
		hr = GetDevice()->CreateTexture(width,height,0,0,D3DFMT_DXT1,D3DPOOL_MANAGED,&pTex,NULL);
		if( hr!=D3D_OK ) return NULL;
		hr = pTex->LockRect(0,&rc,NULL,0);
		if(hr==D3D_OK){
			CopyMemory(rc.pBits,pbuf,length  );
			pTex->UnlockRect(0);
		}
	} else if( format == 0x03 ) {
		hr = GetDevice()->CreateTexture(width,height,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&pTex,NULL);
		if( hr!=D3D_OK ) return NULL;
		hr = pTex->LockRect(0,&rc,NULL,0);
		if(hr==D3D_OK){
			for( DWORD jy=0; jy<(DWORD)height; jy++ ){
				for( DWORD jx=0; jx<(DWORD)width; jx++ ){
					DWORD *pp  = (DWORD *)rc.pBits;
					BYTE  *idx = (BYTE  *)(pbuf+0x400);
					DWORD *pal = (DWORD *)pbuf;
					pp[(height-jy-1)*width+jx] = pal[idx[jy*width+jx]];
				}
			}
		}
		pTex->UnlockRect(0);
	} else if( format == 0x04 ) {
		hr = GetDevice()->CreateTexture(width,height,0,0,D3DFMT_X8R8G8B8,D3DPOOL_MANAGED,&pTex,NULL);
		if( hr!=D3D_OK ) return NULL;
		hr = pTex->LockRect(0,&rc,NULL,0);
		if(hr==D3D_OK){
			for( DWORD jy=0; jy<(DWORD)height; jy++ ){
				for( DWORD jx=0; jx<(DWORD)width; jx++ ){
					DWORD *pp  = (DWORD *)rc.pBits;
					BYTE  *idx = (BYTE  *)(pbuf+0x400);
					DWORD *pal = (DWORD *)pbuf;
					pp[(height-jy-1)*width+jx] = pal[idx[jy*width+jx]];
				}
			}
		}
		pTex->UnlockRect(0);
	}
	return (pTex);
}

//									CBone													//
CBone::CBone()
{
	m_sibBoneIdx=m_BoneIdx=m_parBoneIdx=m_chdBoneIdx=m_strIdx=0;
	m_trans.x = m_trans.y = m_trans.z = 0.f;
	m_scale.x = m_scale.y = m_scale.z = 0.f;
	m_rot.x = m_rot.y = m_rot.z = m_rot.w = 0.f;
	D3DXMatrixIdentity( &m_matrix );
}

CBone::~CBone()
{
	m_sibBoneIdx=m_BoneIdx=m_parBoneIdx=m_chdBoneIdx=m_strIdx=0;
	m_trans.x = m_trans.y = m_trans.z = 0.f;
	m_scale.x = m_scale.y = m_scale.z = 0.f;
	m_rot.x = m_rot.y = m_rot.z = m_rot.w = 0.f;
	D3DXMatrixIdentity( &m_matrix );
}

void CBone::CalcMatrix( void )
{
	D3DXMATRIX m1,m2,m3;

    D3DXMatrixRotationQuaternion(&m1,&m_rot);
    D3DXMatrixScaling(&m2,m_scale.x,m_scale.y,m_scale.z);
    D3DXMatrixTranslation(&m3,m_trans.x,m_trans.y,m_trans.z);
    D3DXMatrixMultiply(&m1,&m1,&m2);
    D3DXMatrixMultiply(&m1,&m1,&m3);
	m_matrix = m1;
}

//		CMotionBone												//
CBoneArry::CBoneArry()
{
	m_sibBoneIdx=m_BoneIdx=m_parBoneIdx=m_chdBoneIdx=m_strIdx=0;
	a_trans.clear();
	a_scale.clear();
	a_rot.clear();
	a_matrix.clear();
}

CBoneArry::~CBoneArry()
{
	m_sibBoneIdx=m_BoneIdx=m_parBoneIdx=m_chdBoneIdx=m_strIdx=0;
	a_trans.clear();
	a_scale.clear();
	a_rot.clear();
	a_matrix.clear();
}
//		CMotionBone												//
CMotion::CMotion()
{
	a_boneArry.clear();
}

CMotion::~CMotion()
{
	a_boneArry.clear();
}

//		CData
CData::CData()
{
	D3DXMatrixIdentity( &m_mRootTransform );
}

CData::~CData()
{
	D3DXMatrixIdentity( &m_mRootTransform );
}

//		モデルの行列初期化
void CData::InitTransform( void )
{
	D3DXMatrixIdentity( &m_mRootTransform );
}
 
//		行列取得
void CData::GetMatrix( D3DXMATRIX &mat )
{
	mat = m_mRootTransform;
}

//		行列設定
void CData::SetMatrix( D3DXMATRIX &mat )
{
	m_mRootTransform = mat;
}

//		行列乗算
void CData::MulMatrix( D3DXMATRIX &mat )
{
	m_mRootTransform *= mat;
}

//		移動
void CData::Translation( float px, float py, float pz )
{
	D3DXMATRIX mat;
	D3DXMatrixTranslation( &mat, px, py, pz );
	MulMatrix( mat );
}

//		拡大縮小
void CData::Scaling( float sx, float sy, float sz )
{
	D3DXVECTOR3 pos;
	D3DXMATRIX	m1,m2,m3;

	GetWorldPosition( pos );
	D3DXMatrixTranslation( &m1, -pos.x, -pos.y, -pos.z );
	D3DXMatrixScaling( &m2, sx, sy, sz );
	D3DXMatrixTranslation( &m3, pos.x, pos.y, pos.z );
	m1 = m1*m2*m3;
	MulMatrix( m1 );
}

//		Ｘ軸回転
void CData::RotationX( float rot )
{
	D3DXMATRIX mat;
	D3DXMatrixRotationX( &mat, rot );
	MulMatrix( mat );
}

//		Ｙ軸回転
void CData::RotationY( float rot )
{
	D3DXMATRIX mat;
	D3DXMatrixRotationY( &mat, rot );
	MulMatrix( mat );
}

//		Ｚ軸回転
void CData::RotationZ( float rot )
{
	D3DXMATRIX mat;
	D3DXMatrixRotationZ( &mat, rot );
	MulMatrix( mat );
}

//		指定位置を中心に回転
void CData::RotationCenter( D3DXVECTOR3 pos, float rot )
{
	D3DXMATRIX mat;
	D3DXMatrixTranslation( &mat, -pos.x, -pos.y, -pos.z );
	MulMatrix( mat );
	D3DXMatrixRotationY( &mat, rot );
	MulMatrix( mat );
	D3DXMatrixTranslation( &mat, pos.x, pos.y, pos.z );
	MulMatrix( mat );
}

//		その位置を中心に回転
void CData::RotationDir( float rot )
{
	D3DXMATRIX mat;
	D3DXVECTOR3 pos;

	GetMatrix( mat );
	pos.x = mat._41;
	pos.y = mat._42;
	pos.z = mat._43;
	D3DXMatrixTranslation( &mat, -pos.x, -pos.y, -pos.z );
	MulMatrix( mat );
	D3DXMatrixRotationY( &mat, rot );
	MulMatrix( mat );
	D3DXMatrixTranslation( &mat, pos.x, pos.y, pos.z );
	MulMatrix( mat );
}

//		Ｘ軸ミラー
void CData::MirrorX( void )
{
	D3DXMATRIX mat=matrixMirrorX;
	MulMatrix( mat );
}

//		Ｙ軸ミラー
void CData::MirrorY( void )
{
	D3DXMATRIX mat=matrixMirrorY;
	MulMatrix( mat );
}

//		Ｚ軸ミラー
void CData::MirrorZ( void )
{
	D3DXMATRIX mat=matrixMirrorZ;
	MulMatrix( mat );
}

//		現在のワールド空間位置から at 方向を注視します。
//		このとき上方向は up ベクトルで指定します。
//
//		D3DXVECTOR3 &at		: 注視点
//		D3DXVECTOR3 &up		: 上方向（省略時 { 0, 1, 0 }
bool CData::LookAt( D3DXVECTOR3 &at, D3DXVECTOR3 &up )
{
	float length;
	D3DXVECTOR3 from, right;

	// モデルの位置を取得
	GetWorldPosition( from );

	// 視点のＺベクトルを取得
	D3DXVECTOR3 eye = at - from;
	length = D3DXVec3Length( &eye );
	if ( length < 1e-6f ) return false;
	// 正規化
	eye *= 1.0f/length;

	up -= -D3DXVec3Dot( &up, &eye ) * eye;
	length = D3DXVec3Length( &up );

	// 正しいアップベクトルの検索
	if ( length < 1e-6f )
	{
		up = D3DXVECTOR3(0,1,0) - eye.y*eye;
		length = D3DXVec3Length( &up );

		if ( length < 1e-6f )
		{
			up = D3DXVECTOR3(0,0,1) - eye.z*eye;
			length = D3DXVec3Length( &up );

			if ( length < 1e-6f) return false;
		}
	}

	// Ｙ及びＸベクトルを取得
	up *= 1.0f/length;
	D3DXVec3Cross( &right, &up, &eye );

	// オブジェクトを注視
	D3DXMATRIX mat;
	GetMatrix( mat );
	mat._11 = right.x;	mat._12 = right.y;	mat._13 = right.z;	mat._14 = 0;
	mat._21 = up.x;		mat._22 = up.y;		mat._23 = up.z;		mat._24 = 0;
	mat._31 = eye.x;	mat._32 = eye.y;	mat._33 = eye.z;	mat._34 = 0;
	mat._41 = from.x;	mat._42 = from.y;	mat._43 = from.z;	mat._43 = 1;
	SetMatrix( mat );

	return true;
}

//		ワールド空間での位置
void CData::GetWorldPosition( D3DXVECTOR3 &pos )
{
	D3DXMATRIX mat;
	GetMatrix( mat );
	pos.x = mat._41;
	pos.y = mat._42;
	pos.z = mat._43;
}

//		X軸を取り出します。
void CData::GetXAxis( D3DXVECTOR3 &pos )
{
	D3DXMATRIX mat;
	GetMatrix( mat );
	pos.x = mat._11;
	pos.y = mat._12;
	pos.z = mat._13;
}

//		Y軸を取り出します。
void CData::GetYAxis( D3DXVECTOR3 &pos )
{
	D3DXMATRIX mat;
	GetMatrix( mat );
	pos.x = mat._21;
	pos.y = mat._22;
	pos.z = mat._23;
}

//		Z軸を取り出します。
void CData::GetZAxis( D3DXVECTOR3 &pos )
{
	D3DXMATRIX mat;
	GetMatrix( mat );
	pos.x = mat._31;
	pos.y = mat._32;
	pos.z = mat._33;
}

//======================================================================
//
//		スクリーンでの位置
//
//		カメラの前方にある場合はX,Y座標は0～1が格納される。
//		カメラの後方にある場合の値は不定。
//
//	input
//		D3DXVECTOR4 &pos		: 位置を格納するベクトル
//
//	output
//		true カメラの前 / false カメラの後ろ
//
//======================================================================
bool CData::GetScreenPosition( D3DXVECTOR4 &pos )
{
	// オブジェクト×ビュー×投影のマトリクスを取得
	D3DXMATRIX mat;
	GetMatrix( mat );
	D3DXMATRIX mTransform = mat * Glob.matView * Glob.matProj;

	// スクリーン座標の算出
	float x = mTransform._41;
	float y = mTransform._42;
	float z = mTransform._43;
	float w = mTransform._44;
	float sz = z / w;

	// オブジェクトがカメラの前方にある場合
	if ( (sz >= 0.0f) && (sz < w) )
	{
		pos.x = (1.0f+(x/w)) * 0.5f;
		pos.y = (1.0f-(y/w)) * 0.5f;
		pos.z = sz;
		pos.w = w;

		return true;
	}

	return false;
}

//		CModel
CModel::CModel()
{
	m_nBone=m_sklPos=0;
	memset(&m_sklsectH,0,sizeof(SECThead));
	memset(&m_sklH,0,sizeof(SKLhead));
	a_sklstrTables.clear();
	a_Bones.clear();
	a_Meshes.clear();
	a_hlsls.clear();
	a_tex.clear();
	m_box.~CBOX();
	m_comp.~CBOX();
	//m_pram.~CPRAM();
}

//======================================================================
//
//		デストラクタ
//
//======================================================================
CModel::~CModel()
{
	m_nBone=m_sklPos=0;
	memset(&m_sklsectH,0,sizeof(SECThead));
	memset(&m_sklH,0,sizeof(SKLhead));
	a_sklstrTables.clear();
	a_Bones.clear();
	a_Meshes.clear();
	a_hlsls.clear();
	a_tex.clear();
	m_box.~CBOX();
	m_comp.~CBOX();
	//m_pram.~CPRAM();
}

//		テクスチャの読み込み
HRESULT CModel::LoadTextureFromFile( string FileName )
{

	HRESULT hr							= S_OK;

	// ファイルをメモリに取り込む
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
		dwSize = dwSize%16?(dwSize/16+1)*16:dwSize;
	    pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}

	// テクスチャの読み込み
	int			type,pos=0,next;
	while( pos<dwSize ) {
		next = SWAP4(*((int *)(pdat+pos+12)));
		if( next<=0 || next+pos>dwSize ) break;
		type = *((int*)(pdat+pos));
		switch( type ) {
			case PWIB : //
				int num=0;
				TAGhead tagh;
				tagh.section	= *((int*)(pdat+pos+num));num+=sizeof(int);
				tagh.fsize		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.un			= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.offset		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				CSection *pSect = new CSection;
				pSect->LoadRes(pos+num,pdat);
				for( int i=0 ; i<pSect->m_resH.numRes ; i++ ) {
					if( pSect->a_resInfos[i].chidx<0 ) {
						if( pSect->a_resTypes[i] == (SectionType)txb ) {
							CTexture *pTex = new CTexture;
							pTex->m_texName = pSect->a_strTables[i];
							pTex->LoadTextureBase( tagh.offset,pSect->m_resPos + pSect->a_resInfos[i].offset, pdat );
							a_tex.push_back( *pTex );
							SAFE_DELETE( pTex );
						}
					} 
				}
				break;
		}
		pos+=next;
	}
	delete pdat;
	return hr;
}

#define BONE_SIZE	(0xb0)

void CModel::LoadBoneBase( int pos, char *pbuf ) {
	int num=0;
	string str;
	// セクションヘッダ読み込み
	m_sklsectH.magicNumber	= *( (MagicNumber *)(pbuf+pos+num));num+=sizeof( long long );
	m_sklsectH.version		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklsectH.un			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklsectH.sectSize	= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	memcpy(&m_sklsectH.junk,pbuf+pos+num,28);num+=28;
	//スケルトンヘッダ読み込み
	m_sklH.un1				= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.strOffset		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.numStr			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.LKs				= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.strIdx			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.un2				= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.boneLen			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklH.un3				= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_sklPos = pos+num;
	num = 0;	pos = m_sklPos+m_sklH.strOffset;
	//スケルトン ストリングの読み込み
	for( int i=0 ; i<m_sklH.numStr ; i++ ) {
		str.append(pbuf+pos+num);
		a_sklstrTables.push_back( str );
		num += str.length()+1;
		str.clear();
	}
	m_nBone =  m_sklH.boneLen/BONE_SIZE;
	num = 0;	pos = m_sklPos;
	for( int i=0 ; i<m_nBone ; i++ ) {
		int un;
		CBone *pbone = new CBone;
		pbone->m_strIdx = *((int*)(pbuf+pos+num));num+=sizeof(int);
		un = *((int*)(pbuf+pos+num));num+=sizeof(int);
		un = *((int*)(pbuf+pos+num));num+=sizeof(int);
		un = *((int*)(pbuf+pos+num));num+=sizeof(int);
		pbone->m_trans.x = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_trans.y = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_trans.z = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_rot.x = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_rot.y = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_rot.z = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_rot.w = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_scale.x = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_scale.y = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_scale.z = *((float*)(pbuf+pos+num));num+=sizeof(float);
		pbone->m_parBoneIdx = *((int*)(pbuf+pos+num));num+=sizeof(int);
		pbone->m_chdBoneIdx = *((int*)(pbuf+pos+num));num+=sizeof(int);
		pbone->m_sibBoneIdx = *((int*)(pbuf+pos+num));num+=sizeof(int);
		pbone->m_BoneIdx = *((int*)(pbuf+pos+num));num+=sizeof(int);
		un = *((int*)(pbuf+pos+num));num+=sizeof(int);
		un = *((int*)(pbuf+pos+num));num+=sizeof(int);
		pbone->CalcMatrix();
		num+=96;
		a_Bones.push_back( *pbone );
		SAFE_DELETE( pbone );
	}
}

//		ボーンの読み込み
HRESULT CModel::LoadBoneFromFile( string FileName )
{
	HRESULT hr							= S_OK;
	
	// ファイルをメモリに取り込む
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
		dwSize = dwSize%16?(dwSize/16+1)*16:dwSize;
		pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}
	// ボーンの読み込み
	int			type,pos=0,next,offset;
	while( pos<dwSize ) {
		next = SWAP4(*((int *)(pdat+pos+12)));
		if( next<=0 || next+pos>dwSize ) break;
		type = *((int*)(pdat+pos));
		switch( type ) {
			case PWIB : //
				int num=0;
				TAGhead tagh;
				tagh.section	= *((int*)(pdat+pos+num));num+=sizeof(int);
				tagh.fsize		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.un			= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.offset		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				CSection *pDir = new CSection;
				pDir->LoadRes(pos+num,pdat);
				offset = pDir->GetSectionStart((SectionType)skl);
				LoadBoneBase( offset, pdat );
				SAFE_DELETE( pDir );
				break;
		}
		pos+=next;
	}
	// 終了
	delete pdat;
	return hr;
}

void CModel::LoadMeshBase( int pos, char *pbuf ) {
	int num=0,off=0;
	string str;
	CChunk *pChunk = new CChunk;

	// MDLチャンクデータ読み込み
	pChunk->LoadChunk( (ChunkType)0,pos,pbuf );

	//　子チャンク中のMESHチャンクを読み込み
	pos=pChunk->m_chunkPos;
	for( int i=0 ; i<pChunk->m_chunkD.numChild ; i++ ) {
		if( pChunk->a_Chunks[i].m_chunkH.chunkType==MESH ) {
			CMesh *pMesh = new CMesh;
			pMesh->LoadMesh( pos , pbuf );
			a_Meshes.push_back( *pMesh );
			SAFE_DELETE( pMesh );
		} else if( pChunk->a_Chunks[i].m_chunkH.chunkType==AABB ) {
			CChunk *pchk = new CChunk;
			pchk->LoadChunk( pChunk->m_chunkH.chunkType,pos,pbuf );// AABBチャンクデータ読み込み
			for( int j=0,tpos=pchk->m_chunkPos ; j<pchk->m_chunkD.numChild ; j++ ) {
				if( pchk->a_Chunks[j].m_chunkH.chunkType==AABB ) {
					m_box.LoadBOX( tpos,pbuf );
					m_box.CalcBOX();
				}
				if( pchk->a_Chunks[j].m_chunkH.chunkType==COMP ) {
					m_comp.LoadBOX( tpos,pbuf );
					m_comp.CalcBOX();
				}
				tpos += pchk->a_Chunks[j].m_chunkH.nextChunk;
			}
			SAFE_DELETE( pchk );
		}
		pos+=pChunk->a_Chunks[i].m_chunkH.nextChunk;
	}
	SAFE_DELETE( pChunk );
}

//		メッシュの読み込み
HRESULT CModel::LoadMeshFromFile( string FileName )
{

	HRESULT hr							= S_OK;

	// ファイルをメモリに取り込む
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
		dwSize = dwSize%16?(dwSize/16+1)*16:dwSize;
	    pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}

	// メッシュの読み込み
	int			type,pos=0,next,offset;
	while( pos<dwSize ) {
		next = SWAP4(*((int *)(pdat+pos+12)));
		if( next<=0 || next+pos>dwSize ) break;
		type = *((int*)(pdat+pos));
		switch( type ) {
			case PWIB : //
				int num=0;
				TAGhead tagh;
				tagh.section	= *((int*)(pdat+pos+num));num+=sizeof(int);
				tagh.fsize		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.un			= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.offset		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				CSection *pDir = new CSection;
				pDir->LoadRes(pos+num,pdat);
				offset = pDir->GetSectionStart((SectionType)wrb);
				SAFE_DELETE( pDir );
				CChunk *pChunk = new CChunk;
				pChunk->LoadChunk( (ChunkType)0,offset+0x30, pdat ); // オフセット＋セクションヘッダ　48バイト
				offset = pChunk->GetChunkOffset( (ChunkType)MDL );
				SAFE_DELETE( pChunk );
				LoadMeshBase( offset,pdat );
				break;
		}
		pos+=next;
	}
	delete pdat;
	return hr;
}

void CModel::LoadShaderBase( int pos, char *pbuf ) {
	int num=0,off=0;
	string str;
	CChunk *pChunk = new CChunk;

	// SHDチャンクデータ読み込み
	pChunk->LoadChunk( (ChunkType)0,pos,pbuf );

	//　子チャンク中のHLSLチャンクを読み込み
	pos=pChunk->m_chunkPos;
	for( int i=0 ; i<pChunk->m_chunkD.numChild ; i++ ) {
		if( pChunk->a_Chunks[i].m_chunkH.chunkType==HLSL ) {
			CHLSL *phlsl = new CHLSL;
			phlsl->LoadHLSL( pos , pbuf );
			a_hlsls.push_back( *phlsl );
			SAFE_DELETE( phlsl );
		} else if( pChunk->a_Chunks[i].m_chunkH.chunkType==PRAM ) {
			m_pram.LoadPRAM( pos , pbuf );
		}
		pos+=pChunk->a_Chunks[i].m_chunkH.nextChunk;
	}
	SAFE_DELETE( pChunk );
}

//		シェーダーの読み込み
HRESULT CModel::LoadShaderFromFile( string FileName )
{

	HRESULT hr							= S_OK;

	// ファイルをメモリに取り込む
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
		dwSize = dwSize%16?(dwSize/16+1)*16:dwSize;
	    pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}

	// メッシュの読み込み
	int			type,pos=0,next,offset;
	while( pos<dwSize ) {
		next = SWAP4(*((int *)(pdat+pos+12)));
		if( next<=0 || next+pos>dwSize ) break;
		type = *((int*)(pdat+pos));
		switch( type ) {
			case PWIB : //
				int num=0;
				TAGhead tagh;
				tagh.section	= *((int*)(pdat+pos+num));num+=sizeof(int);
				tagh.fsize		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.un			= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.offset		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				CSection *pDir = new CSection;
				pDir->LoadRes(pos+num,pdat);
				offset = pDir->GetSectionStart((SectionType)sdrb);
				SAFE_DELETE( pDir );
				CChunk *pChunk = new CChunk;
				pChunk->LoadChunk( (ChunkType)0,offset+0x30, pdat ); // オフセット＋セクションヘッダ　48バイト
				offset = pChunk->GetChunkOffset( (ChunkType)SHD );
				SAFE_DELETE( pChunk );
				LoadShaderBase( offset,pdat );
				break;
		}
		pos+=next;
	}
	delete pdat;
	return hr;
}

void CModel::LoadMtbHeader( int pos, char *pbuf )
{
	int num=0,index=0;

	index = *( (int *)(pbuf+pos+num));num+=sizeof(int);
	num += 12;
	m_motMtbH.Float1 = *( (float *)(pbuf+pos+num));num+=sizeof(float);
	m_motMtbH.numBones = (int)*( (unsigned short *)(pbuf+pos+num));num+=sizeof(short);
	m_motMtbH.numIndices = (int)*( (unsigned short *)(pbuf+pos+num));num+=sizeof(short);
	m_motMtbH.numIndices &= 0x3fff;
	m_motMtbH.flags = (int)*( (unsigned char *)(pbuf+pos+num));num+=sizeof(char);
	m_motMtbH.a_index.clear();m_motMtbH.a_flag.clear();
	for( int i=0 ; i<m_motMtbH.numIndices ; i++ ) 
	{
		int idx = *( (int *)(pbuf+pos+num));num+=sizeof(int);
		m_motMtbH.a_index.push_back(idx&0x7fffffff);
		m_motMtbH.a_flag.push_back((idx&0x80000000) == 0x80000000 );
	}
}

void CModel::LoadSpuBinaryDetail( int pos, char *pbuf )
{
		int base=0,base2 = 0,num=0,index=0;
}


/*四捨五入（10のn乗の位を処理）*/

double    round(double src, int n)
{
    double dst;
    
    dst = src * pow(10., -n - 1);          /*処理を行う桁を10-1 の位にする*/
    dst = (double)(int)(dst + 0.5);
    
    return    dst * pow(10., n + 1);       /*処理を行った桁を元に戻す*/
}

double    rounddown(double src, int n)
{
    double dst;
    
    dst = src * pow(10., -n - 1);          /*処理を行う桁を10-1 の位にする*/
//    dst = (double)(int)(dst + 0.5);
    
    return    dst * pow(10., n + 1);       /*処理を行った桁を元に戻す*/
}
int CBoneArry::GetConstant( int pos,char *pbuf,int count,int type ) 
{
	int num=0;

	float val = SWAPF(pbuf+pos+num);num+=sizeof(float);
	float			time=0.f;
	D3DXVECTOR3		trans=D3DXVECTOR3(0.f,0.f,0.f),scale=D3DXVECTOR3(0.f,0.f,0.f);
	D3DXQUATERNION	rot=D3DXQUATERNION(0.f,0.f,0.f,1.f);
	D3DXMATRIX		 matrix; D3DXMatrixIdentity(&matrix);
	switch( type ) 
	{
		case 0x04:
		case 0x0b:
		case 0x11:
			trans.x = val;
			break;
		case 0x05:
		case 0x0c:
		case 0x12:
			trans.y = val;
			break;
		case 0x06:
		case 0x0d:
		case 0x13:
			trans.z = val;
			break;
		case 0x07:
		case 0x0e:
			scale.x = val;
			break;
		case 0x08:
		case 0x0f:
			scale.y = val;
			break;
		case 0x09:
		case 0x10:
			scale.z = val;
			break;
	}
	a_time.push_back(time);
	a_trans.push_back(trans);
	a_scale.push_back(scale);
	a_rot.push_back(rot);
	a_matrix.push_back(matrix);
	return(num);
}

int CBoneArry::GetCompressedLinear( int pos, char *pbuf,int count,int type )
{
	int num=0;
	bool flag = (count & 0x8000) != 0;
	int  numValue = count & 0x7fff;
	float Offset = SWAPF(pbuf+pos+num);num+=sizeof(float);
	float Scale = SWAPF(pbuf+pos+num);num+=sizeof(float);
	unsigned char *pLen = new unsigned char[numValue];
	memcpy(pLen,pbuf+pos+num,numValue);num+=numValue;
	for( int i=0 ; i<numValue ; i++ ) 
	{
		unsigned char len = pLen[i];
		if( len==0 ) len = 1;
		unsigned char *pIndice = new unsigned char[len];
		memcpy(pIndice,pbuf+pos+num,len);num+=len;
		if( flag ) 
		{
			for( int j=0 ; j<len ; j++ ) 
			{
				float			time=0.f;
				D3DXVECTOR3		trans=D3DXVECTOR3(0.f,0.f,0.f),scale=D3DXVECTOR3(0.f,0.f,0.f);
				D3DXQUATERNION	rot=D3DXQUATERNION(0.f,0.f,0.f,1.f);
				D3DXMATRIX		 matrix; D3DXMatrixIdentity(&matrix);

				unsigned short sval = SWAP2(*((unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
				bool isNeg = (sval&0x8000) !=0;
				float val = (float)(sval & 0x7fff)/32767.f;
				if( isNeg ) val = -val;
				val = val*Scale+Offset;
				time = (float)((float)pIndice[j]+256.*(float)i);
				switch( type ) 
				{
				case 0x04:
				case 0x0b:
				case 0x11:
					trans.x = val;
					break;
				case 0x05:
				case 0x0c:
				case 0x12:
					trans.y = val;
					break;
				case 0x06:
				case 0x0d:
				case 0x13:
					trans.z = val;
					break;
				case 0x07:
				case 0x0e:
					scale.x = val;
					break;
				case 0x08:
				case 0x0f:
					scale.y = val;
					break;
				case 0x09:
				case 0x10:
					scale.z = val;
					break;
				}
				a_time.push_back(time);
				a_trans.push_back(trans);
				a_scale.push_back(scale);
				a_rot.push_back(rot);
				a_matrix.push_back(matrix);
			}
		}
		else
		{
			for( int j=0 ; j<len ; j++ ) 
			{
				float			time=0;
				D3DXVECTOR3		trans=D3DXVECTOR3(0.f,0.f,0.f),scale=D3DXVECTOR3(0.f,0.f,0.f);
				D3DXQUATERNION	rot=D3DXQUATERNION(0.f,0.f,0.f,1.f);
				D3DXMATRIX		 matrix; D3DXMatrixIdentity(&matrix);

				unsigned char sval =*((unsigned char *)(pbuf+pos+num));num+=sizeof(char);
				bool isNeg = (sval&0x80) !=0;
				float val = (float)(sval & 0x7f)/127.f;
				if( isNeg ) val = -val;
				val = val * Scale + Offset;

				time = (float)((float)pIndice[j]+256.*(float)i);
				switch( type ) 
				{
				case 0x04:
				case 0x0b:
				case 0x11:
					trans.x = val;
					break;
				case 0x05:
				case 0x0c:
				case 0x12:
					trans.y = val;
					break;
				case 0x06:
				case 0x0d:
				case 0x13:
					trans.z = val;
					break;
				case 0x07:
				case 0x0e:
					scale.x = val;
					break;
				case 0x08:
				case 0x0f:
					scale.y = val;
					break;
				case 0x09:
				case 0x10:
					scale.z = val;
					break;
				}
				a_time.push_back(time);
				a_trans.push_back(trans);
				a_scale.push_back(scale);
				a_rot.push_back(rot);
				a_matrix.push_back(matrix);
			}
		}
	}
	num = ((pos+num+3)&0x7FFFFFFFFFFFFFFC)-pos;
//	num += num%4;
	return(num);
}

int  CBoneArry::GetQuaternion( int pos, char *pbuf,int count,int type )
{
	int num=0;
	bool flag = (count & 0x80) != 0;
	int  numLength = count & 0xffffff7f;
	unsigned char *pLen = new unsigned char[numLength];
	memcpy(pLen,pbuf+pos+num,numLength);num += numLength;
	int skip = ((numLength+3)&0x7ffffffc)-numLength;
	num += skip;
	for( int i=0 ; i<numLength ; i++ ) 
	{
		unsigned char len = *(pLen+i);
		for( int j=0 ; j<len ; j++ ) 
		{
			float			time=0;
			D3DXVECTOR3 trans=D3DXVECTOR3(0.f,0.f,0.f),scale=D3DXVECTOR3(0.f,0.f,0.f);
			D3DXQUATERNION rot=D3DXQUATERNION(0.f,0.f,0.f,1.f);
			D3DXMATRIX  matrix; D3DXMatrixIdentity(&matrix);

			long long data = 0;
			data  = SWAP2(*((unsigned short *)(pbuf+pos+num)));num+=sizeof(short);data = data << 32;
			data |= SWAP2(*((unsigned short *)(pbuf+pos+num)))<< 16;num+=sizeof(short);
			data |= SWAP2(*((unsigned short *)(pbuf+pos+num)));num+=sizeof(short);
			int index = (int)(data>>43) &0x1f;
			int signFlag = (int)(data >> 39) & 0x0f;
			int val = (int)(data >> 17) & 0x3fffff;
			int wVal = (int)(data & 0x1ffff);
			int globalIdx = index + 32*i;

			float w = (float)wVal / 131071.f;
			if( flag )
			{
				w = 1-w*w;
			}
			double halfPi = M_PI/.5;
			double dVal = (double)val;
			double sqrtVal = sqrt(dVal);
			double sqrtValWholePart = rounddown(sqrtVal,-1);
			double v44 = halfPi* (1.-(sqrtValWholePart/2047.));
			double tmp = (sqrtValWholePart < 0.001) ? 0 : halfPi * (dVal - sqrtValWholePart * sqrtValWholePart) / (2. * sqrtValWholePart);
			double tmp6 = sqrt(1. - w*w);
			double tx = sin(-tmp);
			double ty = sin(halfPi - tmp);
			double tz = sin(v44);
			double tw = sin(v44-halfPi);

			float rx = (float)(ty*-tw*tmp6);
			float ry = (float)(tz*tmp6);
			float rz = (float)(-tx*-tw*tmp6);
			if( (signFlag & 0x8) !=0 ) 
			{
				signFlag ^=0xf;
				rot.w = -w;
			}
			else
			{
				rot.w = w;
			}
			rot.x = ((signFlag & 0x4)!=0)? -rx:rx;
			rot.y = ((signFlag & 0x2)!=0)? -ry:ry;
			rot.z = ((signFlag & 0x1)!=0)? -rz:rz;
			D3DXVec3Normalize((D3DXVECTOR3 *)&rot,(D3DXVECTOR3 *)&rot);
			time = (float)globalIdx;
			a_time.push_back(time);
			a_trans.push_back(trans);
			a_scale.push_back(scale);
			a_rot.push_back(rot);
			a_matrix.push_back(matrix);
		}
	}
	return(num);
}

void CModel::LoadSpuBinary( int pos, char *pbuf )
{
	int base=0,num=0,index=0;
	MotSpuChunk	    *tChunk;
	MotSpuSection	*tSection;
	CBoneArry  *pBoneArry;

	index = *( (int *)(pbuf+pos+num));num+=sizeof(int);
	num += 12;
	m_motSpuH.sectLength	= SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
	num += 12;
	base = num;
	m_motSpuH.numSections	= (int)*( (unsigned short *)(pbuf+pos+num));num+=sizeof(short);
	m_motSpuH.numBones		= (int)*( (unsigned short *)(pbuf+pos+num));num+=sizeof(short);
	num += 12;
	m_motSpuH.a_spuSections.clear();
	for( int i=0 ; i<m_motSpuH.numSections ; i++ )
	{
		tSection = new MotSpuSection;
		tSection->a_spuChunks.clear();
		tSection->name = m_motH.a_chunkNames[i];
		tSection->idx  = i;
		tSection->offset = SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
		tSection->numChunks = SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
		m_motSpuH.a_spuSections.push_back(*tSection);
	}
	for( int i=0 ; i<m_motSpuH.numSections ; i++ )
	{
		num =base + m_motSpuH.a_spuSections[i].offset;
		for( int j=0 ; j<m_motSpuH.a_spuSections[i].numChunks ; j++ )
		{
			m_motSpuH.a_spuSections[i].a_spuChunks.clear();
			tChunk = new MotSpuChunk;
			tChunk->offset = SWAP4(*((int *)(pbuf+pos+num)));num+=sizeof(int);
			tChunk->length = SWAP2(*((unsigned short*)(pbuf+pos+num)));num+=sizeof(short);
			tChunk->numChildren = *((unsigned char *)(pbuf+pos+num));num++;
			tChunk->flag = *((unsigned char *)(pbuf+pos+num));num++;
			m_motSpuH.a_spuSections[i].a_spuChunks.push_back(*tChunk);
			num = base+m_motSpuH.a_spuSections[i].a_spuChunks[j].offset;
			a_Motions.clear();
			for( int k=0 ; k<m_motSpuH.a_spuSections[i].a_spuChunks[j].numChildren ; k++ )
			{
				pBoneArry = new CBoneArry;
				int boneId = SWAP2(*((unsigned short*)(pbuf+pos+num)));num+=sizeof(short);
				int type = SWAP2(*((unsigned short*)(pbuf+pos+num)));num+=sizeof(short);
				int count = SWAP2(*((unsigned short*)(pbuf+pos+num)));num+=sizeof(short);
				num+=2;
				pBoneArry->a_matrix.clear();
				pBoneArry->a_trans.clear();
				pBoneArry->a_scale.clear();
				pBoneArry->a_rot.clear();
				pBoneArry->a_time.clear();
				switch( type ) 
				{
				case 0x00:// quaternion
					num += pBoneArry->GetQuaternion( pos+num, pbuf,count,type);
					break;
				case 0x04:// compressed Linear 
				case 0x05:
				case 0x06:
				case 0x07:
				case 0x08:
				case 0x09:
					num += pBoneArry->GetCompressedLinear(pos+num,pbuf,count,type);
//					num += (pos+num)%4;
					break;
				case 0x0B:// constant 
				case 0x0C:
				case 0x0D:
				case 0x0E:
				case 0x0F:
				case 0x10:
					num += pBoneArry->GetConstant(pos+num,pbuf,count,type);
					break;
				case 0x11:// uncompressed Linear
				case 0x12:
				case 0x13:
					num += 6*count;
					break;
//				default:
				}
			}
		}
	}
}

void CModel::LoadMtbGeneral( int pos, char *pbuf )
{
}

void CModel::LoadMotionBase( int pos, char *pbuf ) {
	int toff=0,num=0,index=0;
	string str;
	m_motPos = pos; // セクションスタート
	// セクションヘッダ読み込み
	m_motsectH.magicNumber	= *( (MagicNumber *)(pbuf+pos+num));num+=sizeof( long long );
	m_motsectH.version		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_motsectH.un			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_motsectH.sectSize	= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	memcpy(&m_motsectH.junk,pbuf+pos+num,28);num+=28;
	num +=16;
	//モーションヘッダ読み込み
	m_motH.numSects			= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_motH.a_sectOffsets.clear();
	for( int i=0 ; i<m_motH.numSects ; i++ ) {
		toff = *( (int *)(pbuf+pos+num));num+=sizeof(int);
		m_motH.a_sectOffsets.push_back(toff);
	}
	m_motH.numSectNames		= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_motH.a_sectNameOffsets.clear();
	for( int i=0 ; i<m_motH.numSects ; i++ ) {
		toff = *( (int *)(pbuf+pos+num));num+=sizeof(int);
		m_motH.a_sectNameOffsets.push_back(toff);
	}
	m_motH.numChunkNames	= *( (int *)(pbuf+pos+num));num+=sizeof(int);
	m_motH.a_chunkNameOffsets.clear();
	for( int i=0 ; i<m_motH.numChunkNames ; i++ ) {
		toff = *( (int *)(pbuf+pos+num));num+=sizeof(int);
		m_motH.a_chunkNameOffsets.push_back(toff);
	}
	m_motH.a_sectNames.clear();
	for( int i=0 ; i<m_motH.numSectNames ; i++ ) {
		str.append(pbuf+m_motPos+m_motH.a_sectNameOffsets[i]);
		m_motH.a_sectNames.push_back(str);
		num += str.length()+1;
		str.clear();
	}
	m_motH.a_chunkNames.clear();
	for( int i=0 ; i<m_motH.numChunkNames ; i++ ) {
		str.append(pbuf+m_motPos+m_motH.a_chunkNameOffsets[i]);
		m_motH.a_chunkNames.push_back(str);
		num += str.length()+1;
		str.clear();
	}
//	m_motPos = pos+num;
	for( int i=0 ; i<m_motH.numSects; i++ ) {
		pos = m_motPos+m_motH.a_sectOffsets[i];
		index	 = *( (int *)(pbuf+pos));
        string sectionName = m_motH.a_sectNames[index];
		if( sectionName == "Header" ) {
                LoadMtbHeader( pos, pbuf);
		} else if( sectionName == "SpuBinary" ) {
                LoadSpuBinary( pos, pbuf );
		} else {
                LoadMtbGeneral( pos,pbuf );
        }
		
	}
}

//		モーションの読み込み
HRESULT CModel::LoadMotionFromFile( string FileName )
{
	HRESULT hr							= S_OK;

	//====================================================
	// ファイルをメモリに取り込む
	//====================================================
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
	    pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}

	//====================================================
	// モーションの読み込み
	//====================================================
	int	 type,pos=0,next,offset;
	while( pos<dwSize ) {
		next = SWAP4(*((int *)(pdat+pos+12)));
		if( next<=0 || next+pos>dwSize ) break;
		type = *((int*)(pdat+pos));
		switch( type ) {
			case PWIB : //
				int num=0;
				TAGhead tagh;
				tagh.section	= *((int*)(pdat+pos+num));num+=sizeof(int);
				tagh.fsize		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.un			= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				tagh.offset		= SWAP4(*((int *)(pdat+pos+num)));num+=sizeof(int);
				CSection *pDir = new CSection;
				pDir->LoadRes(pos+num,pdat);
//				offset = pDir->GetSectionStart((SectionType)mtb,"cbnm_id0");
				offset = pDir->GetSectionStart((SectionType)mtb,"cbba_add_dmg_f");
				SAFE_DELETE( pDir );
				LoadMotionBase( offset,pdat );
				break;
		}
		pos+=next;
	}
	// 終了
	delete pdat;
	return hr;
}

//		CNPC
CNPC::CNPC()
{
	m_mBody		= 0;
}

CNPC::~CNPC()
{
	m_mBody		= 0;
	m_mdlPath.empty();
}

//		モンス　データ読み込み
bool CNPC::LoadNPC()
{
	string  fName;

	// データ初期化

	//  ファイル名の取得
	fName = GetModelPath();
	//  データのロード
	HRESULT hr;
	// ボーンのロード
	hr = LoadBoneFromFile( fName+"\\skl\\0001" );
	//if( hr ) return false;
	//　テクスチャ、メッシュのロード
	hr = LoadTextureFromFile( fName+"\\equ\\e001\\top_tex1\\0000" );
	//if( hr ) return false;
	hr = LoadMeshFromFile( fName+"\\equ\\e001\\top_mdl\\0001" );
	//if( hr ) return false;
	hr = LoadShaderFromFile( fName+"\\equ\\e001\\top_mdl\\0001" );
	//if( hr ) return false;
//	LoadMotionFromFile( fName+"\\act\\emp_emp\\bid\\base\\0000");

	return true;
}




