#ifndef RENDER_CORE_H
#define RENDER_CORE_H

enum R_Rasterizer_Kind {
    R_RasterizerState_Default,
    R_RasterizerState_Wireframe,
    R_RasterizerState_Text,
    R_RasterizerState_COUNT
};

enum R_Blend_State_Kind {
    R_BlendState_Draw,
    R_BlendState_Mesh,
    R_BlendState_COUNT
};

enum R_Sampler_Kind {
    R_SamplerKind_Linear,
    R_SamplerKind_Point,
    R_SamplerKind_COUNT
};

enum R_Depth_State_Kind {
    R_DepthState_Default,
    R_DepthState_Wireframe,
    R_DepthState_Disabled,
    R_DepthState_COUNT
};

enum R_Tex2D_Format {
    R_Tex2DFormat_R8,
    R_Tex2DFormat_R8G8B8A8,
};

struct R_2D_Vertex {
    V2_F32 dst;
    V2_F32 src;
    V4_F32 color;
    f32 omit_tex;
    V3_F32 padding_;
};

struct R_2D_Rect {
    Rect dst;
    Rect src;
    V4_F32 color;
    f32 border_thickness;
    f32 omit_tex;
    f32 _unused[2];
};

struct R_3D_Vertex {
    V4_F32 pos;
    V4_F32 color;
    V2_F32 tex;
};

enum R_Params_Kind {
    R_ParamsKind_Nil,
    R_ParamsKind_UI,
    R_ParamsKind_Quad,
    R_ParamsKind_Mesh,
    R_ParamsKind_COUNT,
};

struct R_Params_UI {
    M4_F32 xform;
    Rect clip;
    R_Handle tex;
};

struct R_Params_Quad {
    M4_F32 xform;
    R_Handle tex;
    R_Sampler_Kind sampler;
};

struct R_Params_Mesh {
    M4_F32 projection;
    M4_F32 view;
    R_Handle tex;
    R_Rasterizer_Kind rasterizer;
};

struct R_Params {
    R_Params_Kind kind;
    union {
        R_Params_UI *params_ui;
        R_Params_Quad *params_quad;
        R_Params_Mesh *params_mesh;
    };
};

struct R_Batch {
    R_Params params;
    u8 *v;
    int bytes;
};

struct R_Batch_Node {
    R_Batch_Node *next;
    R_Batch batch;
};

struct R_Batch_List {
    R_Batch_Node *first;
    R_Batch_Node *last;
    int count;
};

#endif // RENDER_CORE_H
