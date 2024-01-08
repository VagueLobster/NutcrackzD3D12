cbuffer MVPBuffer : register(b0)
{
	row_major float4x4 projection; // : packoffset(c0);
    row_major float4x4 model;      // : packoffset(c4);
    row_major float4x4 view;       // : packoffset(c8);
};

struct ShaderInput
{
    float3 inPos : POSITION;
    float3 inColor : COLOR;
};

struct ShaderOutput
{
    float3 outColor : COLOR;
	float4 position : SV_POSITION;
};

ShaderOutput main(ShaderInput input)
{
    ShaderOutput output;
    output.outColor = input.inColor;
    output.position = mul(mul(mul(float4(input.inPos, 1.0f), model), view), projection);
    return output;
}
