#include "draw.h"

global Arena *draw_arena;
global Arena *draw_temp_arena;
global Draw_Bucket *draw_bucket;

internal void draw_begin(OS_Handle window_handle) {
    if (draw_arena == NULL) {
        draw_arena = arena_alloc(get_virtual_allocator(), MB(4));
    }

    if (draw_temp_arena == NULL) {
        draw_temp_arena = arena_alloc(get_virtual_allocator(), MB(4));
    }

    draw_bucket = push_array(draw_arena, Draw_Bucket, 1);
    *draw_bucket = {};

    V2_F32 window_dim = os_get_window_dim(window_handle);
    draw_bucket->clip = make_rect(0.f, 0.f, window_dim.x, window_dim.y);
}

internal void draw_end() {
    arena_clear(draw_arena);
    arena_clear(draw_temp_arena);
    draw_bucket = nullptr;
}

internal void draw_push_batch_node(R_Batch_List *list, R_Batch_Node *node) {
    SLLQueuePush(list->first, list->last, node);
    list->count += 1;
}

internal void draw_set_texture(R_Handle tex) {
    // R_Batch_Node *node = draw_bucket->batches.last;
    // if (node == NULL || node->batch.bytes > 0) {
    //     node = push_array(draw_arena, R_Batch_Node, 1);
    //     draw_push_batch_node(&draw_bucket->batches, node);
    // }
    draw_bucket->tex = tex;
}

internal void draw_set_xform(M4_F32 xform) {
    // R_Batch_Node *node = draw_bucket->batches.last;
    // if (node == NULL || node->batch.bytes > 0) {
    //     node = push_array(draw_arena, R_Batch_Node, 1);
    //     draw_push_batch_node(&draw_bucket->batches, node);
    // }
    draw_bucket->xform = xform;
}

internal void draw_set_clip(Rect clip) {
    draw_bucket->clip = clip;
    R_Batch_Node *node = draw_bucket->batches.last;
    node = push_array(draw_arena, R_Batch_Node, 1);
    R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
    node->batch.params.kind = R_ParamsKind_UI;
    node->batch.params.params_ui = params_ui;
    params_ui->tex = draw_bucket->tex;
    params_ui->xform = draw_bucket->xform;
    node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
}

internal void draw_set_rasterizer(R_Rasterizer_Kind rasterizer) {
    R_Batch_Node *node = draw_bucket->batches.last;
    if (node == NULL || node->batch.bytes > 0) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        draw_push_batch_node(&draw_bucket->batches, node);
    }
    draw_bucket->rasterizer = rasterizer;
}

internal void draw_set_sampler(R_Sampler_Kind sampler) {
    draw_bucket->sampler = sampler;
}

internal void draw_batch_push_vertex(R_Batch *batch, R_2D_Vertex src) {
    R_2D_Vertex *dst = push_array(draw_arena, R_2D_Vertex, 1);
    *dst = src;
    batch->bytes += sizeof(R_2D_Vertex);
}

inline internal void draw_batch_push_3d_vertex(R_Batch *batch, R_3D_Vertex src) {
    R_3D_Vertex *dst = push_array(draw_arena, R_3D_Vertex, 1);
    *dst = src;
    batch->bytes += sizeof(R_3D_Vertex);
}

internal void draw_batch_push_rect(R_Batch *batch, R_2D_Rect rect) {
    R_2D_Rect *dst = push_array(draw_arena, R_2D_Rect, 1);
    *dst = rect;
    batch->bytes += sizeof(R_2D_Rect);
}

internal void draw_string_truncated(String8 string, Font *font, V4_F32 color, V2_F32 offset, Rect bounds) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != font->texture || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.params_ui = params_ui;
        params_ui->tex = font->texture;
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->tex = font->texture;
        draw_bucket->params_kind = R_ParamsKind_UI;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    V2_F32 cursor = offset;
    for (u64 i = 0; i < string.count; i++) {
        u8 c = string.data[i];
        if (c == '\n') {
            cursor.x = offset.x;
            cursor.y += font->glyph_height;
            continue;
        }
        Glyph g = font->glyphs[c];

        Rect dst;
        dst.x0 = cursor.x + g.bl;
        dst.x1 = dst.x0 + g.bx;
        dst.y0 = cursor.y - g.bt + font->ascend;
        dst.y1 = dst.y0 + g.by;

        cursor.x += g.ax;

        if (dst.y1 < bounds.y0 || dst.x1 < bounds.x0 || dst.y0 > bounds.y1 || dst.x0 > bounds.x1) {
            continue; 
        }

        Rect src;
        src.x0 = g.to;
        src.y0 = 0.f;
        src.x1 = src.x0 + (g.bx / (f32)font->width);
        src.y1 = src.y0 + (g.by / (f32)font->height);

        R_2D_Vertex tl = r_2d_vertex(dst.x0, dst.y0, src.x0, src.y0, color);
        R_2D_Vertex tr = r_2d_vertex(dst.x1, dst.y0, src.x1, src.y0, color);
        R_2D_Vertex bl = r_2d_vertex(dst.x0, dst.y1, src.x0, src.y1, color);
        R_2D_Vertex br = r_2d_vertex(dst.x1, dst.y1, src.x1, src.y1, color);

        draw_batch_push_vertex(&node->batch, bl);
        draw_batch_push_vertex(&node->batch, tl);
        draw_batch_push_vertex(&node->batch, tr);
        draw_batch_push_vertex(&node->batch, br);
    }
}

internal void draw_text(String8 text, Font *font, V4_F32 color, V2_F32 offset) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != font->texture || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        node->batch.params.params_ui = params_ui;
        params_ui->tex = font->texture;
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->tex = font->texture;
        draw_bucket->params_kind = R_ParamsKind_UI;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    V2_F32 cursor = offset;
    for (u64 i = 0; i < text.count; i++) {
        u8 c = text.data[i];
        if (c == '\n') {
            cursor.x = offset.x;
            cursor.y += font->glyph_height;
            continue;
        }
        Glyph g = font->glyphs[c];

        Rect dst;
        dst.x0 = cursor.x + g.bl;
        dst.x1 = dst.x0 + g.bx;
        dst.y0 = cursor.y - g.bt + font->ascend;
        dst.y1 = dst.y0 + g.by;

        Rect src;
        src.x0 = g.to;
        src.y0 = 0.f;
        src.x1 = src.x0 + (g.bx / (f32)font->width);
        src.y1 = src.y0 + (g.by / (f32)font->height);

        R_2D_Rect rect = r_2d_rect(dst, src, color, 0.f, 0.f);
        draw_batch_push_rect(&node->batch, rect);
        cursor.x += g.ax;
    }
}

internal void draw_textf(Font *font, V4_F32 color, V2_F32 offset, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(draw_temp_arena, fmt, args);
    va_end(args);
    draw_text(string, font, color, offset);
}

internal void draw_ui_graph(f32 *values, int values_count, f32 line_width, f32 max_line_height, V2_F32 start, V4_F32 color) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != r_handle_zero() || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        node->batch.params.params_ui = params_ui;
        params_ui->tex = r_handle_zero();
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->params_kind = R_ParamsKind_UI;
        draw_bucket->tex = r_handle_zero();
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    f32 x = start.x;
    for (int i = 0; i < values_count; i++) {
        f32 value = values[i];
        f32 line_height = value * max_line_height;
        Rect line_rect;
        line_rect.x0 = x + 1.0f;
        line_rect.x1 = line_rect.x0 + line_width - 2.0f;
        //@Note Draws top-bottom so y1 is the baseline of the graph
        line_rect.y0 = start.y - line_height;
        line_rect.y1 = line_rect.y0 + line_height;
        
        // Rect line_rect = make_rect(x + 1.0f, start.y, line_width - 2.0f, line_height);
        R_2D_Rect rect = r_2d_rect(line_rect, rect_zero(), color, 0.0f, 0.0f);
        draw_batch_push_rect(&node->batch, rect);
        x += line_width;
    }
}

internal void draw_ui_img(R_Handle img, Rect dst, Rect src, V4_F32 color) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != img || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        node->batch.params.params_ui = params_ui;
        params_ui->tex = img;
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->params_kind = R_ParamsKind_UI;
        draw_bucket->tex = img;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    R_2D_Rect rect = r_2d_rect(dst, src, color, 0.f, 0.f);
    draw_batch_push_rect(&node->batch, rect);
}

internal void draw_ui_rect(Rect dst, V4_F32 color, f32 border_thickness) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != 0 || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        node->batch.params.params_ui = params_ui;
        params_ui->tex = {0};
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->params_kind = R_ParamsKind_UI;
        draw_bucket->tex = {0};
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    Rect src = {};
    R_2D_Rect rect = r_2d_rect(dst, src, color, border_thickness, 1.f);
    draw_batch_push_rect(&node->batch, rect);
}

internal void draw_ui_rect_outline(Rect rect, V4_F32 color) {
    draw_ui_rect(make_rect(rect.x0, rect.y0, rect_width(rect), 1), color, 0.f);
    draw_ui_rect(make_rect(rect.x0, rect.y0, 1, rect_height(rect)), color, 0.f);
    draw_ui_rect(make_rect(rect.x1 - 1, rect.y0, 1, rect_height(rect)), color, 0.f);
    draw_ui_rect(make_rect(rect.x0, rect.y1 - 1, rect_width(rect), 1), color, 0.f);
}

internal void draw_ui_box(UI_Box *box) {
    if (box->flags & UI_BoxFlag_DrawBackground) {
        draw_ui_rect(box->rect, box->background_color, box->border_thickness);
    }
    if (box->flags & UI_BoxFlag_DrawBorder) {
        draw_ui_rect_outline(box->rect, box->border_color);
    }
    if (box->flags & UI_BoxFlag_DrawHotEffects) {
        V4_F32 hot_color = box->hover_color;
        hot_color.w *= box->hot_t;
        draw_ui_rect(box->rect, hot_color, box->border_thickness);
    }
    // if (box->flags & UI_BoxFlag_DrawActiveEffects) {
    //     V4_F32 active_color = VECTOR4(1.f, 1.f, 1.f, 1.f);
    //     active_color.w *= box->hot_t;
    //     draw_ui_rect(box->rect, active_color);
    // }
    if (box->flags & UI_BoxFlag_DrawText) {
        V2_F32 text_position = ui_text_position(box);
        text_position += box->view_offset;
        draw_text(box->string, box->font, box->text_color, text_position);
    }

    if (box->custom_draw_proc) {
        box->custom_draw_proc(box, box->draw_data);
    }
}

internal void draw_ui_layout(UI_Box *box) {
    if (box->flags & UI_BoxFlag_Disabled) return;

    if (box != ui_root()) {
        draw_ui_box(box);
    }

    for (UI_Box *child = box->first; child != nullptr; child = child->next) {
        draw_ui_layout(child);
    }
}

internal void draw_quad_pro(R_Handle img, Rect src, Rect dst, V2_F32 origin, f32 rotation, V4_F32 color) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (node == NULL || tex != img || params_kind != R_ParamsKind_Quad) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        R_Params_Quad *params_quad = push_array(draw_arena, R_Params_Quad, 1);
        draw_push_batch_node(&draw_bucket->batches, node);
        node->batch.params.kind = R_ParamsKind_Quad;
        node->batch.params.params_quad = params_quad;
        params_quad->xform = draw_bucket->xform;
        params_quad->tex = img;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
        draw_bucket->tex = img;
        draw_bucket->params_kind = R_ParamsKind_Quad;
        params_quad->sampler = draw_bucket->sampler;
    }

    V2_F32 tl = V2_Zero;
    V2_F32 tr = V2_Zero;
    V2_F32 bl = V2_Zero;
    V2_F32 br = V2_Zero;

    if (rotation == 0.f) {
        tl = v2_f32(dst.x0, dst.y1);
        tr = v2_f32(dst.x1, dst.y1);
        bl = v2_f32(dst.x0, dst.y0);
        br = v2_f32(dst.x1, dst.y0);
    } else {
        f32 S = sinf(DegToRad(rotation));
        f32 C = cosf(DegToRad(rotation));
        f32 x = dst.x0;
        f32 y = dst.y0;
        f32 dx = -origin.x;
        f32 dy = -origin.y;
        f32 dw = rect_width(dst);
        f32 dh = rect_height(dst);

        tl.x = x + dx * C - (dy + dh) * S;
        tl.y = y + dx * S + (dy + dh) * C;
        tr.x = x + (dx + dw) * C - (dy + dh) * S;
        tr.y = y + (dx + dw) * S + (dy + dh) * C;

        bl.x = x + dx * C - dy * S;
        bl.y = y + dx * S + dy * C;
        br.x = x + (dx + dw) * C - dy * S;
        br.y = y + (dx + dw) * S + dy * C;
    }

    R_2D_Vertex top_left     = r_2d_vertex(tl.x, tl.y, src.x0, src.y0, color);
    R_2D_Vertex top_right    = r_2d_vertex(tr.x, tr.y, src.x1, src.y0, color);
    R_2D_Vertex bottom_left  = r_2d_vertex(bl.x, bl.y, src.x0, src.y1, color);
    R_2D_Vertex bottom_right = r_2d_vertex(br.x, br.y, src.x1, src.y1, color);

    draw_batch_push_vertex(&node->batch, bottom_left);
    draw_batch_push_vertex(&node->batch, bottom_right);
    draw_batch_push_vertex(&node->batch, top_right);
    draw_batch_push_vertex(&node->batch, top_left);
}

internal void draw_quad(R_Handle img, Rect dst, Rect src) {
    draw_quad_pro(img, src, dst, V2_Zero, 0.f, V4_One);
}

internal void draw_rect(Rect dst, V4_F32 color) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (node == NULL || tex != r_handle_zero() || params_kind != R_ParamsKind_Quad) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        R_Params_Quad *params_quad = push_array(draw_arena, R_Params_Quad, 1);
        draw_push_batch_node(&draw_bucket->batches, node);
        node->batch.params.kind = R_ParamsKind_Quad;
        node->batch.params.params_quad = params_quad;
        params_quad->xform = draw_bucket->xform;
        params_quad->tex = 0;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
        draw_bucket->tex = 0;
        draw_bucket->params_kind = R_ParamsKind_Quad;
    }

    R_2D_Vertex tl = r_2d_vertex(dst.x0, dst.y0, 0.f, 0.f, color);
    R_2D_Vertex tr = r_2d_vertex(dst.x1, dst.y0, 0.f, 0.f, color);
    R_2D_Vertex bl = r_2d_vertex(dst.x0, dst.y1, 0.f, 0.f, color);
    R_2D_Vertex br = r_2d_vertex(dst.x1, dst.y1, 0.f, 0.f, color);

    draw_batch_push_vertex(&node->batch, bl);
    draw_batch_push_vertex(&node->batch, tl);
    draw_batch_push_vertex(&node->batch, tr);
    draw_batch_push_vertex(&node->batch, br);
}

internal void draw_rect_outline(Rect rect, V4_F32 color) {
    draw_rect(make_rect(rect.x0, rect.y0, rect_width(rect), 1), color);
    draw_rect(make_rect(rect.x0, rect.y0, 1, rect_height(rect)), color);
    draw_rect(make_rect(rect.x1 - 1, rect.y0, 1, rect_height(rect)), color);
    draw_rect(make_rect(rect.x0, rect.y1 - 1, rect_width(rect), 1), color);
}

internal void draw_3d_mesh_begin(M4_F32 projection, M4_F32 view, R_Handle tex, R_Rasterizer_Kind rasterizer) {
    R_Batch_Node *node = push_array(draw_arena, R_Batch_Node, 1);
    draw_push_batch_node(&draw_bucket->batches, node);
    node->batch.params.kind = R_ParamsKind_Mesh;
    node->batch.params.params_mesh = push_array(draw_arena, R_Params_Mesh, 1);
    R_Params_Mesh *params = node->batch.params.params_mesh;
    params->view = view;
    params->projection = projection;
    params->tex = tex;
    params->rasterizer = rasterizer;
    node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
}

inline internal void draw_3d_vertex(V3_F32 pos, V4_F32 color, V2_F32 tex) {
    R_Batch_Node *node = draw_bucket->batches.last;
    R_3D_Vertex vertex = r_3d_vertex(pos, color, tex);
    draw_batch_push_3d_vertex(&node->batch, vertex);
}

