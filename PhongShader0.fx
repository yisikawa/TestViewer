///////////////////////////////////////////////////////////////////////////////////
//型定義
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
//グローバル
///////////////////////////////////////////////////////////////////////////////////
float4x4 WVP; 
float4x4 matWorld; 
float4 LightDir;
float4 Eye; 
float4 Diffuse={0,0,0,0}; 
float4 Ambient; //環境光
texture texDecal; //メッシュのテクスチャ

sampler Sampler = sampler_state //サンプラー
{
    Texture = <texDecal>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};
///////////////////////////////////////////////////////////////////////////////////
//バーテックス・シェーダー
///////////////////////////////////////////////////////////////////////////////////
VS_OUT VS(float4 Pos : POSITION, float3 Normal : NORMAL, float2 f2Texture : TEXCOORD)
{
    VS_OUT Out = (VS_OUT)0; 
    Out.Position = mul(Pos, WVP); 
    Out.Light = LightDir;           
    float3 PosWorld = normalize(mul(Pos, matWorld)); 
    Out.View = Eye - PosWorld;      
    Out.Normal = mul(Normal, matWorld); 
    Out.f2Texture = f2Texture;  
	return Out;
}
///////////////////////////////////////////////////////////////////////////////////
//ピクセル・シェーダー
///////////////////////////////////////////////////////////////////////////////////
float4 PS(VS_OUT In) : COLOR
{
    float3 Normal = normalize(In.Normal);
    float3 LightDir = normalize(In.Light);
    float3 ViewDir = normalize(In.View); 
    float4 NL = saturate(dot(Normal, LightDir)); 
    
    float3 Reflect = normalize(2 * NL * Normal - LightDir); //反射ベクトルの公式通り
    float4 specular = pow(saturate(dot(Reflect, ViewDir)), 4); 

    return  Diffuse * tex2D( Sampler, In.f2Texture ) * (NL + Ambient) + specular; //通常、照明モデルと言えば環境光の項を含めますが、簡単のため除きました。
}

///////////////////////////////////////////////////////////////////////////////////
// テクニック
///////////////////////////////////////////////////////////////////////////////////
technique tecPhong
{
    pass Phong
    {
        VertexShader = compile vs_2_0 VS();
        PixelShader  = compile ps_2_0 PS();
    }
}