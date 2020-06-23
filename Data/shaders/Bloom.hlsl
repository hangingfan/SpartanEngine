/*
Copyright(c) 2016-2020 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =========
#include "Common.hlsl"
#include "Scale.hlsl"
//====================

#if DOWNSAMPLE
float4 mainPS(Pixel_PosUv input) : SV_TARGET
{
    // g_texel_size refers to the current render target, which is half the size of the input texture.
    // So the texel size is g_texel_size * 2, but we will use half of that so that we get some sample overlap and as result, enliminate some blockiness.
    // Same papers call this, a "tent" filter.
    float2 texel_size = g_texel_size;
    return Box_Filter_AntiFlicker(input.uv, tex, texel_size);
}
#endif

#if DOWNSAMPLE_LUMINANCE
float4 mainPS(Pixel_PosUv input) : SV_TARGET
{
    float2 texel_size = g_texel_size; // same as above, tent filter
    float4 color = Box_Filter_AntiFlicker(input.uv, tex, texel_size);
    return saturate_16(luminance(color) * color);
}
#endif

#if UPSAMPLE_BLEND
float4 mainPS(Pixel_PosUv input) : SV_TARGET
{
    float4 sourceColor  = tex.Sample(sampler_point_clamp, input.uv);
    // g_texel_size refers to the current render target, which is twice the size of the input texture, so we multiply by 0.5
    float2 texel_size = g_texel_size * 0.5f;
    float4 sourceColor2 = Box_Filter(input.uv, tex2, texel_size);
    return saturate_16(sourceColor + sourceColor2 * g_bloom_intensity);
}
#endif