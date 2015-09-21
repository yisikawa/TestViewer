///////////////////////////////////////////////////////////////////////////////////
//�^��`
///////////////////////////////////////////////////////////////////////////////////
struct VS_OUT
{
    float4 Position : POSITION;
    float3 Light : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 View : TEXCOORD2;
	float2 f2Texture : TEXCOORD3;
};

///////////////////////////////////////////////////////////////////////////////////
//�O���[�o��
///////////////////////////////////////////////////////////////////////////////////
float4 BoxOffSet;
float4 BoxScale;
float4x4 WVP; 
float4x4 matWorld; 
float4 LightDir;
float4 Eye; 
float4 Diffuse={0,0,0,0}; 
float4 Ambient; //����
texture texDecal; //���b�V���̃e�N�X�`��

sampler Sampler = sampler_state //�T���v���[
{
    Texture = <texDecal>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};
///////////////////////////////////////////////////////////////////////////////////
//�o�[�e�b�N�X�E�V�F�[�_�[
///////////////////////////////////////////////////////////////////////////////////
VS_OUT VS(float4 Pos : POSITION, float3 Normal : NORMAL, float2 f2Texture : TEXCOORD)
{
    VS_OUT Out = (VS_OUT)0; 

//	Pos.xyz = Pos * BoxScale.xyz + BoxOffSet.xyz;
	Pos.xyz = Pos * BoxScale.xyz;
	Pos.w = 1.;
	Out.Position = mul(Pos, WVP); 
    Out.Light = LightDir;           
    float3 PosWorld = normalize(mul(Pos, matWorld)); 
    Out.View = Eye - PosWorld;      
    Out.Normal = mul(Normal, (float3x3)matWorld); 
    Out.f2Texture.x = f2Texture.x;
    Out.f2Texture.y = 1-f2Texture.y;
	return Out;
}
///////////////////////////////////////////////////////////////////////////////////
//�s�N�Z���E�V�F�[�_�[
///////////////////////////////////////////////////////////////////////////////////
float4 PS(VS_OUT In) : COLOR
{
    float3 Normal = normalize(In.Normal);
    float3 LightDir = normalize(In.Light);
    float3 ViewDir = normalize(In.View); 
    float4 NL = saturate(dot(Normal, LightDir)); 
    
    float3 Reflect = normalize(2 * NL * Normal - LightDir); //���˃x�N�g���̌����ʂ�
    float4 specular = pow(saturate(dot(Reflect, ViewDir)), 4); 

    return  Diffuse * tex2D( Sampler, In.f2Texture ) * (NL + Ambient) + specular; //�ʏ�A�Ɩ����f���ƌ����Ί����̍����܂߂܂����A�ȒP�̂��ߏ����܂����B
}

///////////////////////////////////////////////////////////////////////////////////
// �e�N�j�b�N
///////////////////////////////////////////////////////////////////////////////////
technique tecPhong
{
    pass Phong
    {
        VertexShader = compile vs_2_0 VS();
        PixelShader  = compile ps_2_0 PS();
    }
}