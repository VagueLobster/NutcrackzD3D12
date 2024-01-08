struct ShaderInput
{
    float3 inColor : COLOR;
};

struct ShaderOutput
{
    float4 outFragColor : SV_Target0;
};

float4 main(ShaderInput input) : SV_TARGET
{
    ShaderOutput output;
    output.outFragColor = float4(input.inColor, 1.0f);
    return output.outFragColor;
}
