///////////////////////////////////////////////////////////////////////////////////
//型定義
///////////////////////////////////////////////////////////////////////////////////
struct VS_IN
{
	float4 Pos : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent :TANGENT0;
	float4 BWeight : TEXCOORD1;
	float4 Color:TEXCOORD2;
	float2 f2Texture : TEXCOORD;
};
struct VS_OUT
{
    float4 Pos : POSITION;
    float3 Light : TEXCOORD0;
    float3 Eye : TEXCOORD1;
	float2 Tex : TEXCOORD2;
	float4 Col : TEXCOORD3;
};

///////////////////////////////////////////////////////////////////////////////////
//グローバル
///////////////////////////////////////////////////////////////////////////////////
float4 BoxOffSet;
float4 BoxScale;
float4x4 WVP; 
float4x4 matWorld; 
float4 LightDir;
float4 Eye; 
float4 Diffuse={0,0,0,0}; 
float4 Ambient; //環境光
texture texDecal; //メッシュのテクスチャ
texture NormalMap;
texture SpecularMap;

sampler DecalSampler = sampler_state //サンプラー
{
    Texture = <texDecal>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};
sampler NormalSampler = sampler_state
{
    Texture = <NormalMap>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};
sampler SpecularSampler = sampler_state
{
    Texture = <SpecularMap>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};
///////////////////////////////////////////////////////////////////////////////////
//バーテックス・シェーダー
///////////////////////////////////////////////////////////////////////////////////
VS_OUT VS( VS_IN In )
{
    VS_OUT Out = (VS_OUT)0; 

//	Pos.xyz = Pos * BoxScale.xyz + BoxOffSet.xyz;
	In.Pos.xyz = In.Pos * BoxScale.xyz;
	In.Pos.w = 1.;
	In.Normal.xyz = In.Normal * BoxScale.xyz;
	In.Tangent.xyz = In.Tangent * BoxScale.xyz;
	Out.Pos = mul(In.Pos, WVP); 
    In.Normal = mul(In.Normal, (float3x3)matWorld);
    In.Tangent = mul(In.Tangent,(float3x3)matWorld);
    Out.Tex.x = In.f2Texture.x;
    Out.Tex.y = 1-In.f2Texture.y;

	float3 Binormal = cross(In.Normal,In.Tangent);
	float3 PosWorld = normalize(mul(In.Pos, matWorld)); 
	float3 EyeVec = Eye - PosWorld;
//	Out.Eye.x = dot(EyeVec,In.Tangent);
//	Out.Eye.y = dot(EyeVec,Binormal);
	Out.Eye.x = dot(EyeVec,In.Normal);
	Out.Eye.y = dot(EyeVec,In.Normal);
	Out.Eye.z = dot(EyeVec,In.Normal);
	normalize(Out.Eye);

	float3 Light = LightDir.xyz;
//	Out.Light.x = dot(Light,In.Tangent);
//	Out.Light.y = dot(Light,Binormal);
	Out.Light.x = dot(Light,In.Normal);
	Out.Light.y = dot(Light,In.Normal);
	Out.Light.z = dot(Light,In.Normal);
	normalize(Out.Light);
	Out.Col = In.Color;
	return Out;
}
///////////////////////////////////////////////////////////////////////////////////
//ピクセル・シェーダー
///////////////////////////////////////////////////////////////////////////////////
float4 PS(VS_OUT In) : COLOR
{
//	float3 Normal = normalize(In.Normal);
//    float3 LightDir = normalize(In.Light);
//    float3 ViewDir = normalize(In.View); 
//    float4 NL = saturate(dot(Normal, LightDir)); 
//    
//    float3 Reflect = normalize(2 * NL * Normal - LightDir); //反射ベクトルの公式通り
//    float4 specular = pow(saturate(dot(Reflect, ViewDir)), 4); 
//
//    return  Diffuse * tex2D( DecalSampler, In.f2Texture ) * (NL + Ambient) + specular; //通常、照明モデルと言えば環境光の項を含めますが、簡単のため除きました。
	float3 Normal = 2.0*tex2D( NormalSampler, In.Tex ).xyz-1.0;
	float3 Reflect = reflect(-normalize(In.Eye), Normal);	
	
	float4 	DiffuseTerm = Diffuse *  tex2D( DecalSampler, In.Tex ) * max(0, dot(Normal, In.Light));
//	float4 	DiffuseTerm = In.Col *  tex2D( DecalSampler, In.Tex ) * max(0, dot(Normal, In.Light));
	float4 SpecularTerm = 0.7 * pow(max(0,dot(Reflect, In.Light)), 2);
//	float4 SpecularTerm = tex2D( SpecularSampler, In.Tex ) *  pow(max(0,dot(Reflect, In.Light)), 2);
	
	float4 FinalColor=DiffuseTerm+SpecularTerm;
	 
    return FinalColor;    

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