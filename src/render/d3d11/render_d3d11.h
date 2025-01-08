#ifndef RENDER_D3D11_H
#define RENDER_D3D11_H

#include <d3d11.h>
#include <d3dcompiler.h>

struct R_D3D11_Tex2D {
    R_D3D11_Tex2D *next;
    ID3D11Texture2D *texture;
    ID3D11ShaderResourceView *view;
    R_Tex2D_Format format;
    V2_S32 size;
};

enum D3D11_Shader_Kind {
    D3D11_ShaderKind_UI,
    D3D11_ShaderKind_Mesh,
    D3D11_ShaderKind_COUNT
};

enum D3D11_Uniform_Kind {
    D3D11_UniformKind_UI,
    D3D11_UniformKind_Mesh,
    D3D11_UniformKind_COUNT
};

struct D3D11_Uniform_UI {
    M4_F32 xform;
};

struct D3D11_Uniform_Mesh {
    M4_F32 xform; 
};

struct R_D3D11_State {
    Arena *arena;
    IDXGISwapChain *swap_chain;
    ID3D11Device *device;
    ID3D11DeviceContext *device_context;

    //@Note Resources
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilState *depth_stencil_states[R_DepthState_COUNT];
    ID3D11DepthStencilView *depth_stencil_view;
    ID3D11RasterizerState *rasterizer_states[R_RasterizerState_COUNT];
    ID3D11BlendState *blend_states[R_BlendState_COUNT];
    ID3D11SamplerState *samplers[R_SamplerKind_COUNT];
    ID3D11VertexShader *vertex_shaders[D3D11_ShaderKind_COUNT];
    ID3D11PixelShader *pixel_shaders[D3D11_ShaderKind_COUNT];
    ID3D11InputLayout *input_layouts[D3D11_ShaderKind_COUNT];
    ID3D11Buffer *uniform_buffers[D3D11_UniformKind_COUNT];

    R_D3D11_Tex2D *first_free_tex2d;

    //@Note Immediate context
    Rect draw_region;
    R_Handle fallback_tex;
};

#endif //RENDER_D3D11_H
