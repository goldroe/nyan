#ifndef DRAW_H
#define DRAW_H

struct Draw_Bucket {
    M4_F32 xform;
    R_Handle tex;
    Rect clip;
    R_Params_Kind params_kind;
    R_Batch_List batches;
    R_Sampler_Kind sampler;
    R_Rasterizer_Kind rasterizer;
};

internal void draw_begin(OS_Handle window_handle);
internal void draw_end();

internal void draw_quad(R_Handle img, Rect dst, Rect src);
internal void draw_rect(Rect dst, V4_F32 color);
internal void draw_text(String8 text, Font *font, V4_F32 color, V2_F32 offset);

#endif // DRAW_H
