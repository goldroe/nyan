#include "render_d3d11.h"
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")

global R_D3D11_State *r_d3d11_state;

global ID3D11ShaderResourceView *g_atlas_uv_buffer_res;
global ID3D11ShaderResourceView *g_block_color_table_res;

global ID3D11Buffer *chunk_index_buffer;

global const char *r_d3d11_g_shader_rect =
    "cbuffer Constants : register(b0) {\n"
    "    matrix xform;\n"
    "};\n"
    "\n"
    "struct Vertex_In {\n"
    "    float4 dst_rect    : POS;\n"
    "    float4 src_rect    : TEX;\n"
    "    float4 color       : COL;\n"
    "    nointerpolation float4 style : STY;  //x=omit_tex\n"
    "    uint vertex_id     : SV_VertexID;\n"
    "};\n"
    "\n"
    "struct Vertex_Out {\n"
    "    float4 pos            : SV_POSITION;\n"
    "    nointerpolation float2 rect_half_size_px : PSIZE;\n"
    "    float2 sdf_sample_pos : SDF;\n"
    "    float2 src            : TEX;\n"
    "    float4 color          : COL;\n"
    "    nointerpolation float border_thickness_px  : BTP;\n"
    "    nointerpolation float omit_tex             : OTX;\n"
    "};\n"
    "\n"
    "Texture2D main_tex : register(t0);\n"
    "SamplerState tex_sampler : register(s0);\n"
    "\n"
    "float rect_sdf(float2 sample_pos, float2 size, float radius) {\n"
    "    return length(max(abs(sample_pos) - size + radius, 0.0)) - radius;\n"
    "}\n"
    "\n"
    "Vertex_Out vs_main(Vertex_In vertex) {\n"
    "    float2 dst_p0 = vertex.dst_rect.xy;\n"
    "    float2 dst_p1 = vertex.dst_rect.zw;\n"
    "    float2 src_p0 = vertex.src_rect.xy;\n"
    "    float2 src_p1 = vertex.src_rect.zw;\n"
    "\n"
    "    float2 dst_verts_px[] = {\n"
    "        float2(dst_p0.x, dst_p1.y),\n"
    "        float2(dst_p0.x, dst_p0.y),\n"
    "        float2(dst_p1.x, dst_p1.y),\n"
    "        float2(dst_p1.x, dst_p0.y)\n"
    "    };\n"
    "    float2 src_verts_px[] = {\n"
    "        float2(src_p0.x, src_p1.y),\n"
    "        float2(src_p0.x, src_p0.y),\n"
    "        float2(src_p1.x, src_p1.y),\n"
    "        float2(src_p1.x, src_p0.y)\n"
    "    };\n"
    "\n"
    "    float2 dst_size_px = abs(dst_p1 - dst_p0);\n"
    "    float2 dst_verts_pct = float2((vertex.vertex_id >> 1) ? 1.f : 0.f,\n"
    "                                 (vertex.vertex_id & 1)  ? 0.f : 1.f);\n"
    "\n"
    "    float border_thickness_px = vertex.style.x;\n"
    "    float omit_tex = vertex.style.y;\n"
    "\n"
    "    Vertex_Out vertex_out;\n"
    "    vertex_out.pos = mul(xform, float4(dst_verts_px[vertex.vertex_id], 0, 1));\n"
    "    vertex_out.src = src_verts_px[vertex.vertex_id];\n"
    "    vertex_out.color = vertex.color;\n"
    "    vertex_out.rect_half_size_px = dst_size_px / 2.f;\n"
    "    vertex_out.sdf_sample_pos = (2.f * dst_verts_pct - 1.f) * vertex_out.rect_half_size_px;\n"
    "    vertex_out.omit_tex = omit_tex;\n"
    "    vertex_out.border_thickness_px = border_thickness_px;\n"
    "    return vertex_out;\n"
    "}\n"
    "\n"
    "float4 ps_main(Vertex_Out vertex) : SV_TARGET {\n"
    "    float4 tint = vertex.color;\n"
    "    float4 albedo_sample = float4(1, 1, 1, 1);\n"
    "    if (vertex.omit_tex < 1.f) {\n"
    "        albedo_sample = main_tex.Sample(tex_sampler, vertex.src);\n"
    "    }\n"
    "\n"
    "    //@Note SDF border\n"
    "    float softness = 1.f;\n"
    "    float2 sdf_sample_pos = vertex.sdf_sample_pos;\n"
    "    float border_sdf_t = 1;\n"
    "    if (vertex.border_thickness_px > 0) {\n"
    "        float border_sdf_s = rect_sdf(sdf_sample_pos, vertex.rect_half_size_px, vertex.border_thickness_px);\n"
    "        border_sdf_t = 1.f - smoothstep(0, softness * 2.f, border_sdf_s);\n"
    "    }\n"
    "\n"
    "    if (border_sdf_t < 0.001f) {\n"
    "        discard;\n"
    "    }\n"
    "\n"
    "    float4 final_color;\n"
    "    final_color = albedo_sample;\n"
    "    final_color *= tint;\n"
    "    final_color.a *= border_sdf_t;\n"
    "    return final_color;\n"
    "}\n";

global const char *r_d3d11_g_shader_quad =
    "cbuffer Constants : register(b0) {\n"
    "    matrix xform;\n"
    "};\n"
    "\n"
    "struct Vertex_In {\n"
    "    float2 dst   : POSITION;\n"
    "    float2 src   : TEXCOORD;\n"
    "    float4 color : COLOR;\n"
    "    float4 style_params : STY; //x = omit_tex\n"
    "}; \n"
    "\n"
    "struct Vertex_Out {\n"
    "    float4 pos_h : SV_POSITION;\n"
    "    float2 tex   : TEXCOORD;\n"
    "    float4 color : COLOR;\n"
    "    float omit_tex : OTX;\n"
    "};\n"
    "\n"
    "Texture2D tex : register(t0);\n"
    "SamplerState tex_sampler : register(s0);\n"
    "\n"
    "Vertex_Out vs_main(Vertex_In input) {\n"
    "    Vertex_Out output;\n"
    "    output.pos_h = mul(xform, float4(input.dst, 0, 1));\n"
    "    output.tex = input.src;\n"
    "    output.color = input.color;\n"
    "    output.omit_tex = input.style_params.x;\n"
    "    return output;\n"
    "}\n"
    "\n"
    "float4 ps_main(Vertex_Out input) : SV_TARGET {\n"
    "    float4 tex_color = tex.Sample(tex_sampler, input.tex);\n"
    "    float4 color = tex_color * input.color;\n"
    "    return color;\n"
    "}\n";

global const char *r_d3d11_g_shader_mesh =
    " cbuffer Constants : register(b0) {\n"
    "    matrix xform;\n"
    "};\n"
    "\n"
    "struct Vertex_In {\n"
    "    float4 pos_l : POSITION;\n"
    "    float4 color : COLOR;\n"
    "    float2 src   : TEXCOORD;\n"
    "};\n"
    "\n"
    "struct Vertex_Out {\n"
    "    float4 pos_h : SV_POSITION;\n"
    "    float4 color : COLOR;\n"
    "    float2 tex   : TEXCOORD;\n"
    "};\n"
    "\n"
    "Texture2D tex : register(t0);\n"
    "SamplerState tex_sampler : register(s0);\n"
    "\n"
    "Vertex_Out vs_main(Vertex_In input) {\n"
    "    Vertex_Out output;\n"
    "    output.pos_h = mul(xform, float4(input.pos_l.xyz, 1.0));\n"
    "    output.color = input.color;\n"
    "    output.tex = input.src;\n"
    "    return output;\n"
    "}\n"
    "\n"
    "float4 ps_main(Vertex_Out input) : SV_TARGET {\n"
    "    float4 tex_color = tex.Sample(tex_sampler, input.tex);\n"
    "    float4 final_color;\n"
    "    final_color = input.color * tex_color;\n"
    "    return final_color;\n"
    "}\n";

internal bool d3d11_make_shader(String8 shader_name, String8 source, const char *vs_entry, const char *ps_entry, D3D11_INPUT_ELEMENT_DESC *items, int item_count, ID3D11VertexShader **vshader_out, ID3D11PixelShader **pshader_out, ID3D11InputLayout **input_layout_out) {
    printf("Compiling shader '%s'... ", shader_name.data);
    ID3DBlob *vertex_blob, *pixel_blob, *vertex_error_blob, *pixel_error_blob;
    UINT flags = 0;
    #if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
    #endif
    bool compilation_success = true;
    if (D3DCompile(source.data, source.count, (LPCSTR)shader_name.data, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs_entry, "vs_5_0", flags, 0, &vertex_blob, &vertex_error_blob) != S_OK) {
        compilation_success = false;
    }
    if (D3DCompile(source.data, source.count, (LPCSTR)shader_name.data, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps_entry, "ps_5_0", flags, 0, &pixel_blob, &pixel_error_blob) != S_OK) {
        compilation_success = false;
    }

    if (compilation_success) {
        printf("SUCCESS\n");
    } else {
        printf("FAILED\n");
        if (vertex_error_blob) printf("%s", (char *)vertex_error_blob->GetBufferPointer());
        if (pixel_error_blob) printf("%s", (char *)pixel_error_blob->GetBufferPointer());
    }
    if (compilation_success) {
        r_d3d11_state->device->CreateVertexShader(vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), NULL, vshader_out);
        r_d3d11_state->device->CreatePixelShader(pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize(), NULL, pshader_out);
        HRESULT hr = r_d3d11_state->device->CreateInputLayout(items, item_count, vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), input_layout_out);

        if (FAILED(hr)) {
            printf("Error creating input layout: %ld\n", hr);
        }
    }
    if (vertex_error_blob) vertex_error_blob->Release();
    if (pixel_error_blob)  pixel_error_blob->Release();
    if (vertex_blob) vertex_blob->Release();
    if (pixel_blob) pixel_blob->Release();

    Assert(compilation_success);

    return compilation_success;
}

internal bool d3d11_make_shader_from_file(String8 file_name, const char *vs_entry, const char *ps_entry, D3D11_INPUT_ELEMENT_DESC *items, int item_count, ID3D11VertexShader **vshader_out, ID3D11PixelShader **pshader_out, ID3D11InputLayout **input_layout_out) {
    OS_Handle file_handle = os_open_file(file_name, OS_AccessFlag_Read);
    String8 source = os_read_file_string(file_handle);
    os_close_handle(file_handle);
    bool result = d3d11_make_shader(file_name, source, vs_entry, ps_entry, items, item_count, vshader_out, pshader_out, input_layout_out);
    free(source.data);
    return result;
}

internal ID3D11Buffer *d3d11_make_uniform_buffer(u32 size) {
    ID3D11Buffer *constant_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (r_d3d11_state->device->CreateBuffer(&desc, nullptr, &constant_buffer) != S_OK) {
        printf("Failed to create constant buffer\n");
    }
    return constant_buffer;
}

internal void d3d11_upload_uniform(ID3D11Buffer *constant_buffer, void *constants, u32 size) {
    D3D11_MAPPED_SUBRESOURCE res{};
    if (r_d3d11_state->device_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res) == S_OK) {
        memcpy(res.pData, constants, size);
        r_d3d11_state->device_context->Unmap(constant_buffer, 0);
    } else {
        printf("Failed to map constant buffer\n");
    }
}

internal ID3D11Buffer *d3d11_make_vertex_buffer(void *data, u64 buffer_size) {
    ID3D11Buffer *vertex_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = (UINT)buffer_size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA resource{};
    resource.pSysMem = data;
    HRESULT hr = r_d3d11_state->device->CreateBuffer(&desc, &resource, &vertex_buffer);
    if (hr != S_OK) {
        printf("Failed to create vertex buffer\n");
    }
    return vertex_buffer;
}

internal ID3D11Buffer *d3d11_make_index_buffer(void *data, u32 buffer_size) {
    ID3D11Buffer *index_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = buffer_size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA resource{};
    resource.pSysMem = data;
    resource.SysMemPitch = 0;
    resource.SysMemSlicePitch = 0;
    HRESULT hr = r_d3d11_state->device->CreateBuffer(&desc, &resource, &index_buffer);
    if (hr != S_OK) {
        printf("Failed to create index buffer\n");
    }
    return index_buffer;
}

internal ID3D11Buffer *d3d11_make_vertex_buffer_writable(void *data, u32 buffer_size) {
    ID3D11Buffer *vertex_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = buffer_size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA resource{};
    resource.pSysMem = data;
    if (r_d3d11_state->device->CreateBuffer(&desc, &resource, &vertex_buffer) != S_OK) {
        printf("Failed to create vertex buffer\n");
    }
    return vertex_buffer;
}

internal void d3d11_write_vertex_buffer(ID3D11Buffer *vertex_buffer, void *data, u32 size) {
    D3D11_MAPPED_SUBRESOURCE res{};
    if (r_d3d11_state->device_context->Map(vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res) == S_OK) {
        memcpy(res.pData, data, size);
        r_d3d11_state->device_context->Unmap(vertex_buffer, 0);
    } else {
        printf("Failed to map vertex buffer\n");
    }
}

internal DXGI_FORMAT dxgi_from_r_format(R_Tex2D_Format format) {
    DXGI_FORMAT result = {};
    switch (format) {
    case R_Tex2DFormat_R8:
        result = DXGI_FORMAT_R8_UNORM;
        break;
    case R_Tex2DFormat_R8G8B8A8:
        result = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    }
    return result;
}

internal V2_S32 r_texture_size(R_Handle handle) {
    V2_S32 result = v2_s32(0, 0);
    R_D3D11_Tex2D *tex = (R_D3D11_Tex2D *)handle;
    if (tex) {
        result = tex->size;
    }
    return result;
}

internal u32 r_format_size(R_Tex2D_Format format) {
    u32 result = 1;
    switch (format) {
    case R_Tex2DFormat_R8:
        result = 1;
        break;
    case R_Tex2DFormat_R8G8B8A8:
        result = 4;
        break;
    }
    return result;
}

internal R_Handle d3d11_create_texture_mipmap(R_Tex2D_Format format, V2_S32 size, u8 *data) {
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = size.x;
    desc.Height = size.y;
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = dxgi_from_r_format(format);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    ID3D11Texture2D *texture = NULL;
    hr = r_d3d11_state->device->CreateTexture2D(&desc, NULL, &texture);
    r_d3d11_state->device_context->UpdateSubresource(texture, 0, NULL, data, size.x * r_format_size(format), 0);

    ID3D11ShaderResourceView *view = NULL;
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = desc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 4;
    hr = r_d3d11_state->device->CreateShaderResourceView(texture, &srv_desc, &view);

    r_d3d11_state->device_context->GenerateMips(view);

    R_D3D11_Tex2D *tex2d = r_d3d11_state->first_free_tex2d;
    if (tex2d == NULL) {
        tex2d = push_array(r_d3d11_state->arena, R_D3D11_Tex2D, 1);
    } else {
        SLLStackPop(r_d3d11_state->first_free_tex2d);
    }
    tex2d->texture = texture;
    tex2d->view = view;
    tex2d->size = size;

    R_Handle result = (u64)tex2d;
    return result;
}

internal R_Handle d3d11_create_texture(R_Tex2D_Format format, V2_S32 size, u8 *data) {
    R_Handle result = {0};
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = size.x;
    desc.Height = size.y;
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = dxgi_from_r_format(format);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA res_data{};
    res_data.pSysMem = data;
    res_data.SysMemPitch = size.x * r_format_size(format);
    res_data.SysMemSlicePitch = 0;

    ID3D11Texture2D *tex2d = nullptr;
    ID3D11ShaderResourceView *view = nullptr;
    HRESULT hr = r_d3d11_state->device->CreateTexture2D(&desc, &res_data, &tex2d);
    if (hr == S_OK) {
        hr = r_d3d11_state->device->CreateShaderResourceView(tex2d, NULL, &view);
        if (hr != S_OK) {
            printf("Failed to create shader resouce view\n");
            return result;
        }
    } else {
        printf("Failed to create texture 2d\n");
        return result;
    }

    R_D3D11_Tex2D *texture = r_d3d11_state->first_free_tex2d;
    if (texture == nullptr) {
        texture = push_array(r_d3d11_state->arena, R_D3D11_Tex2D, 1);
    } else {
        SLLStackPop(r_d3d11_state->first_free_tex2d);
    }

    texture->texture = tex2d;
    texture->view = view;
    texture->size = size;
    result = (u64)texture;
    return result;
}

internal D3D11_RECT rect_to_d3d11_rect(Rect rect) {
    D3D11_RECT result;
    result.left = (int)rect.x0;
    result.right = (int)rect.x1;
    result.top = (int)rect.y0;
    result.bottom = (int)rect.y1;
    return result;
}

internal void d3d11_resize_render_target_view(UINT width, UINT height) {
    HRESULT hr = S_OK;

    // NOTE: Resize render target view
    r_d3d11_state->device_context->OMSetRenderTargets(0, 0, 0);

    // Release all outstanding references to the swap chain's buffers.
    if (r_d3d11_state->render_target_view) r_d3d11_state->render_target_view->Release();

    // Preserve the existing buffer count and format.
    // Automatically choose the width and height to match the client rect for HWNDs.
    hr = r_d3d11_state->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    // Get buffer and create a render-target-view.
    ID3D11Texture2D *backbuffer;
    hr = r_d3d11_state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);

    hr = r_d3d11_state->device->CreateRenderTargetView(backbuffer, NULL, &r_d3d11_state->render_target_view);

    backbuffer->Release();

    if (r_d3d11_state->depth_stencil_view) r_d3d11_state->depth_stencil_view->Release();

    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        ID3D11Texture2D *depth_stencil_buffer = nullptr;
        hr = r_d3d11_state->device->CreateTexture2D(&desc, NULL, &depth_stencil_buffer);
        hr = r_d3d11_state->device->CreateDepthStencilView(depth_stencil_buffer, NULL, &r_d3d11_state->depth_stencil_view);
    }

    r_d3d11_state->device_context->OMSetRenderTargets(1, &r_d3d11_state->render_target_view, r_d3d11_state->depth_stencil_view);
}

internal void d3d11_render(OS_Handle window_handle, Draw_Bucket *draw_bucket) {
    HRESULT hr = S_OK;
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = r_d3d11_state->draw_region.x0;
    viewport.TopLeftY = r_d3d11_state->draw_region.y0;
    viewport.Width = rect_width(r_d3d11_state->draw_region);
    viewport.Height = rect_height(r_d3d11_state->draw_region);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    r_d3d11_state->device_context->RSSetViewports(1, &viewport);

    float clear_color[4] = {1.0f, 0.0f, 1.0f, 1.0f};
    r_d3d11_state->device_context->ClearRenderTargetView(r_d3d11_state->render_target_view, clear_color);
    r_d3d11_state->device_context->ClearDepthStencilView(r_d3d11_state->depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    r_d3d11_state->device_context->OMSetDepthStencilState(r_d3d11_state->depth_stencil_states[R_DepthState_Default], 0);
    r_d3d11_state->device_context->OMSetRenderTargets(1, &r_d3d11_state->render_target_view, r_d3d11_state->depth_stencil_view);
    r_d3d11_state->device_context->OMSetBlendState(r_d3d11_state->blend_states[0], NULL, 0xffffffff);
    r_d3d11_state->device_context->PSSetSamplers(0, 1, &r_d3d11_state->samplers[R_SamplerKind_Point]);

    for (R_Batch_Node *batch_node = draw_bucket->batches.first; batch_node; batch_node = batch_node->next) {
        R_Batch batch = batch_node->batch;
        R_Params params = batch.params;

        switch (params.kind) {
        default:
            Assert(0);
            break;

        case R_ParamsKind_Mesh:
        {
            R_Params_Mesh *mesh = params.params_mesh;
            int vertex_count = batch.bytes / sizeof(R_3D_Vertex);
            R_3D_Vertex *vertices = (R_3D_Vertex *)batch.v;
            if (vertex_count == 0) continue;

            r_d3d11_state->device_context->RSSetState(r_d3d11_state->rasterizer_states[mesh->rasterizer]);
            r_d3d11_state->device_context->OMSetBlendState(r_d3d11_state->blend_states[R_BlendState_Mesh], NULL, 0xffffffff);
            r_d3d11_state->device_context->PSSetSamplers(0, 1, &r_d3d11_state->samplers[R_SamplerKind_Linear]);

            R_Depth_State_Kind depth_state = R_DepthState_Default;
            if (mesh->rasterizer == R_RasterizerState_Wireframe) depth_state = R_DepthState_Wireframe;
            r_d3d11_state->device_context->OMSetDepthStencilState(r_d3d11_state->depth_stencil_states[depth_state], 0);

            R_Handle tex = mesh->tex;
            if (tex == 0) {
                tex = r_d3d11_state->fallback_tex;
            }
            R_D3D11_Tex2D *tex2d = (R_D3D11_Tex2D *)tex;
            r_d3d11_state->device_context->PSSetShaderResources(0, 1, &tex2d->view);

            ID3D11VertexShader *vertex_shader = r_d3d11_state->vertex_shaders[D3D11_ShaderKind_Mesh];
            ID3D11PixelShader *pixel_shader = r_d3d11_state->pixel_shaders[D3D11_ShaderKind_Mesh];
            r_d3d11_state->device_context->VSSetShader(vertex_shader, NULL, 0);
            r_d3d11_state->device_context->PSSetShader(pixel_shader, NULL, 0);

            D3D11_Uniform_Mesh uniform = {};
            uniform.xform = mesh->projection * mesh->view;
            ID3D11Buffer *uniform_buffer = r_d3d11_state->uniform_buffers[D3D11_ShaderKind_Mesh];
            d3d11_upload_uniform(uniform_buffer, (void *)&uniform, sizeof(D3D11_Uniform_Mesh));
            r_d3d11_state->device_context->VSSetConstantBuffers(0, 1, &uniform_buffer);

            int index_count = vertex_count * 6 / 4;
            Arena *scratch = arena_alloc(get_virtual_allocator(), sizeof(u32) * index_count);
            u32 *indices = push_array(scratch, u32, index_count);
            for (int index = 0, it = 0; index < vertex_count; index += 4) {
                indices[it++] = index;
                indices[it++] = index + 1;
                indices[it++] = index + 2;
                indices[it++] = index;
                indices[it++] = index + 2;
                indices[it++] = index + 3;
            }

            ID3D11Buffer *index_buffer = d3d11_make_index_buffer(indices, sizeof(u32) * index_count);
            ID3D11Buffer *vertex_buffer = d3d11_make_vertex_buffer(vertices, sizeof(R_3D_Vertex) * vertex_count);
            r_d3d11_state->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            r_d3d11_state->device_context->IASetInputLayout(r_d3d11_state->input_layouts[D3D11_ShaderKind_Mesh]);

            UINT stride = sizeof(R_3D_Vertex), offset = 0;
            r_d3d11_state->device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
            r_d3d11_state->device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);
            r_d3d11_state->device_context->DrawIndexed((UINT)index_count, 0, 0);

            arena_release(scratch);
            if (index_buffer) index_buffer->Release();
            if (vertex_buffer) vertex_buffer->Release();
            break;
        }

        case R_ParamsKind_UI:
        {
            R_Params_UI *params_ui = params.params_ui;
            int rect_count = batch.bytes / sizeof(R_2D_Rect);
            R_2D_Rect *rects = (R_2D_Rect *)batch.v;
            if (rect_count == 0) continue;

            r_d3d11_state->device_context->OMSetBlendState(r_d3d11_state->blend_states[R_BlendState_Draw], NULL, 0xffffffff);
            r_d3d11_state->device_context->OMSetDepthStencilState(r_d3d11_state->depth_stencil_states[R_DepthState_Disabled], 0);

            r_d3d11_state->device_context->RSSetState(r_d3d11_state->rasterizer_states[R_RasterizerState_Text]);

            R_Handle tex = params_ui->tex;
            if (tex == 0) {
                tex = r_d3d11_state->fallback_tex;
            }
            
            Rect clip = params_ui->clip;
            D3D11_RECT d3d_clip = rect_to_d3d11_rect(clip);
            r_d3d11_state->device_context->RSSetScissorRects(1, &d3d_clip);

            R_D3D11_Tex2D *tex2d = (R_D3D11_Tex2D *)tex;
            r_d3d11_state->device_context->PSSetShaderResources(0, 1, &tex2d->view);

            ID3D11VertexShader *vertex_shader = r_d3d11_state->vertex_shaders[D3D11_ShaderKind_UI];
            ID3D11PixelShader *pixel_shader = r_d3d11_state->pixel_shaders[D3D11_ShaderKind_UI];
            r_d3d11_state->device_context->VSSetShader(vertex_shader, nullptr, 0);
            r_d3d11_state->device_context->PSSetShader(pixel_shader, nullptr, 0);

            Rect draw_region = r_d3d11_state->draw_region;

            D3D11_Uniform_UI ui_uniform = {};
            ui_uniform.xform = ortho_rh_zo(draw_region.x0, draw_region.x1, draw_region.y1, draw_region.y0, -1.f, 1.f);

            ID3D11Buffer *uniform_buffer = r_d3d11_state->uniform_buffers[D3D11_ShaderKind_UI];
            d3d11_upload_uniform(uniform_buffer, (void *)&ui_uniform, sizeof(ui_uniform));
            r_d3d11_state->device_context->VSSetConstantBuffers(0, 1, &uniform_buffer);

            ID3D11Buffer *instance_buffer = nullptr;
            {
                D3D11_BUFFER_DESC desc = {};
                desc.ByteWidth = batch.bytes;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                D3D11_SUBRESOURCE_DATA res{};
                res.pSysMem = batch.v;
                HRESULT hr = r_d3d11_state->device->CreateBuffer(&desc, &res, &instance_buffer);
                if (hr != S_OK) {
                    printf("Failed to create instance buffer of '%d' bytes: %lu\n", batch.bytes, hr);
                }
            }

            // IA
            u32 stride = sizeof(R_2D_Rect), offset = 0;
            r_d3d11_state->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            r_d3d11_state->device_context->IASetInputLayout(r_d3d11_state->input_layouts[D3D11_ShaderKind_UI]);
            r_d3d11_state->device_context->IASetVertexBuffers(0, 1, &instance_buffer, &stride, &offset);

            r_d3d11_state->device_context->DrawInstanced(4, rect_count, 0, 0);

            if (instance_buffer) instance_buffer->Release();
            break;
        }
        }
    }
}

internal void d3d11_render_initialize(HWND window_handle) {
    Arena *arena = arena_alloc(get_virtual_allocator(), MB(4));
    r_d3d11_state = push_array(arena, R_D3D11_State, 1);
    r_d3d11_state->arena = arena;

    HRESULT hr = S_OK;
    DXGI_MODE_DESC buffer_desc{};
    buffer_desc.Width = 0;
    buffer_desc.Height = 0;
    buffer_desc.RefreshRate.Numerator = 0;
    buffer_desc.RefreshRate.Denominator = 1;
    buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    buffer_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    buffer_desc.Scaling = DXGI_MODE_SCALING_STRETCHED;
    DXGI_SWAP_CHAIN_DESC swapchain_desc{};
    swapchain_desc.BufferDesc = buffer_desc;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.OutputWindow = window_handle;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &swapchain_desc, &r_d3d11_state->swap_chain, &r_d3d11_state->device, NULL, &r_d3d11_state->device_context);
    Assert(SUCCEEDED(hr));

    ID3D11Texture2D *backbuffer;
    hr = r_d3d11_state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
    hr = r_d3d11_state->device->CreateRenderTargetView(backbuffer, NULL, &r_d3d11_state->render_target_view);
    backbuffer->Release();

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        r_d3d11_state->device->CreateDepthStencilState(&desc, &r_d3d11_state->depth_stencil_states[R_DepthState_Default]);
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        r_d3d11_state->device->CreateDepthStencilState(&desc, &r_d3d11_state->depth_stencil_states[R_DepthState_Wireframe]);
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = false;
        desc.StencilEnable = false;
        r_d3d11_state->device->CreateDepthStencilState(&desc, &r_d3d11_state->depth_stencil_states[R_DepthState_Disabled]);
    }

    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.ScissorEnable = false;
        desc.DepthClipEnable = false;
        desc.FrontCounterClockwise = true;
        desc.MultisampleEnable = true;
        desc.AntialiasedLineEnable = false;
        r_d3d11_state->device->CreateRasterizerState(&desc, &r_d3d11_state->rasterizer_states[R_RasterizerState_Default]);
    }

    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_WIREFRAME;
        desc.CullMode = D3D11_CULL_BACK;
        desc.DepthBias = -1000;
        desc.DepthBiasClamp = 0.0001f;
        desc.SlopeScaledDepthBias = 0.01f;
        desc.ScissorEnable = false;
        desc.DepthClipEnable = false;
        desc.FrontCounterClockwise = true;
        desc.AntialiasedLineEnable = true;
        r_d3d11_state->device->CreateRasterizerState(&desc, &r_d3d11_state->rasterizer_states[R_RasterizerState_Wireframe]);
    }

    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = false;
        desc.FrontCounterClockwise = true;
        desc.MultisampleEnable = false;
        desc.AntialiasedLineEnable = true;
        r_d3d11_state->device->CreateRasterizerState(&desc, &r_d3d11_state->rasterizer_states[R_RasterizerState_Text]);
    }

    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        r_d3d11_state->device->CreateSamplerState(&desc, &r_d3d11_state->samplers[R_SamplerKind_Linear]);
    }

    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        r_d3d11_state->device->CreateSamplerState(&desc, &r_d3d11_state->samplers[R_SamplerKind_Point]);
    }

    {
        D3D11_BLEND_DESC desc{};
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        // desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        // desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRESULT hr = r_d3d11_state->device->CreateBlendState(&desc, &r_d3d11_state->blend_states[R_BlendState_Draw]);
        if (FAILED(hr)) {
            printf("Error CreateBlendState\n");
        }
    }

    {
        D3D11_BLEND_DESC desc{};
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRESULT hr = r_d3d11_state->device->CreateBlendState(&desc, &r_d3d11_state->blend_states[R_BlendState_Mesh]);
        if (FAILED(hr)) {
            printf("Error CreateBlendState\n");
        }
    }

    {
        u8 bitmap[16] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };
        r_d3d11_state->fallback_tex = d3d11_create_texture(R_Tex2DFormat_R8G8B8A8, {2, 2}, bitmap);
    }

    D3D11_INPUT_ELEMENT_DESC rect_ilay[] = {
        { "POS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(R_2D_Rect, dst),      D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(R_2D_Rect, src),      D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(R_2D_Rect, color),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "STY", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(R_2D_Rect, border_thickness), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };
    d3d11_make_shader(str8_lit("Rect"), str8_cstring(r_d3d11_g_shader_rect), "vs_main", "ps_main", rect_ilay, ArrayCount(rect_ilay), &r_d3d11_state->vertex_shaders[D3D11_ShaderKind_UI], &r_d3d11_state->pixel_shaders[D3D11_ShaderKind_UI], &r_d3d11_state->input_layouts[D3D11_ShaderKind_UI]);
    r_d3d11_state->uniform_buffers[D3D11_UniformKind_UI] = d3d11_make_uniform_buffer(sizeof(D3D11_Uniform_UI));

    D3D11_INPUT_ELEMENT_DESC mesh_ilay[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(R_3D_Vertex, pos),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(R_3D_Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(R_3D_Vertex, tex),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    d3d11_make_shader(str8_lit("Mesh"), str8_cstring(r_d3d11_g_shader_mesh), "vs_main", "ps_main", mesh_ilay, ArrayCount(mesh_ilay), &r_d3d11_state->vertex_shaders[D3D11_ShaderKind_Mesh], &r_d3d11_state->pixel_shaders[D3D11_ShaderKind_Mesh], &r_d3d11_state->input_layouts[D3D11_ShaderKind_Mesh]);
    r_d3d11_state->uniform_buffers[D3D11_UniformKind_Mesh] = d3d11_make_uniform_buffer(sizeof(D3D11_Uniform_Mesh));
}
