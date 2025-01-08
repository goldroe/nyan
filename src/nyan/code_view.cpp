global char *g_modal_state_strings[ModalState_COUNT] = {
    "NOR",
    "INS",
    "SEL"
};

internal Buffer *find_text_buffer(Buffer_ID id);
internal void view_move_lines(Code_View *view, s64 lines);
internal void view_move_char(Code_View *view, s64 move);
internal Application *get_application();
internal void set_root_pane(Pane *pane);

internal void view_set_normal_state(Code_View *view) {
    view->modal_state = ModalState_Normal;
    view->keymap = get_normal_keymap();
    view->marker_active = false;
}

internal void view_set_insert_state(Code_View *view) {
    view->modal_state = ModalState_Insert;
    view->keymap = get_insert_keymap();
    view->marker_active = false;
}

internal void view_set_select_state(Code_View *view) {
    view->modal_state = ModalState_Select;
    view->keymap = get_select_keymap();
    view->persist_selection = true;
}

internal void view_set_goto_state(Code_View *view) {
    view->modal_state = ModalState_Normal;
    view->keymap = get_goto_keymap();
    view->marker_active = false;
}

internal void view_set_space_state(Code_View *view) {
    view->modal_state = ModalState_Normal;
    view->keymap = get_space_keymap();
    view->marker_active = false;
}

internal f32 buffer_string_range_width(Buffer *buffer, Rng_S64 range, Font *font) {
    f32 result = 0;
    for (s64 idx = range.min; idx < range.max; idx++) {
        u8 c = buffer_at(buffer, idx);
        Glyph g = font->glyphs[c];
        result += g.ax;
    }
    return result;
}

internal f32 buffer_line_width_before(Buffer *buffer, Buffer_Cursor cursor, Font *font) {
    f32 result = 0;
    s64 col = cursor.col;
    Buffer_Pos end = cursor.pos;
    for (Buffer_Pos pos = cursor.pos - cursor.col; pos < end; pos++) {
        u8 c = buffer_at(buffer, pos); 
        Glyph g = font->glyphs[c];
        result += g.ax;
    }
    return result;
}

internal f32 buffer_line_width_after(Buffer *buffer, Buffer_Cursor cursor, Font *font) {
    f32 result = 0;
    s64 line_len = buffer_get_line_length(buffer, cursor.line);
    Buffer_Pos end = cursor.pos - cursor.col + line_len - 1;
    for (Buffer_Pos pos = cursor.pos; pos <= end; pos++) {
        u8 c = buffer_at(buffer, pos); 
        Glyph g = font->glyphs[c];
        result += g.ax;
    }
    return result;
}

internal void view_ensure_cursor_in_view(Code_View *view) {
    V2_F32 code_area_dim = rect_dim(view->box->rect);
    int visible_lines = (int)trunc_f32(code_area_dim.y / view->font->glyph_height);
    Rng_S64 view_rng = rng_s64(view->scroll_pt.idx, view->scroll_pt.idx + visible_lines);
    UI_Scroll_Pt new_pt = view->scroll_pt;
    if (view->cursor.line < view_rng.min) {
        new_pt.idx = view->cursor.line;
    } else if (view->cursor.line >= view_rng.max - 1) {
        s64 scroll = view->cursor.line - (view_rng.max - 2); 
        new_pt.idx = view_rng.min + scroll;
    }
    view->scroll_pt = new_pt;
}

internal void view_set_cursor(Code_View *view, Buffer_Cursor cursor) {
    view->cursor = cursor;
    view_ensure_cursor_in_view(view);
}

internal void view_delete(Code_View *view, Seek_Dir dir) {
    Buffer *buffer = find_text_buffer(view->buffer);
    if (view->marker_active) {
        Buffer_Pos start = view->cursor.pos + 1;
        Buffer_Pos end = view->marker.pos;
        if (start > end) Swap(Buffer_Pos, start, end);
        buffer_delete_region(buffer, start, end);
        Buffer_Cursor new_cursor = get_cursor_from_position(buffer, start);
        view_set_cursor(view, new_cursor);
        view->marker_active = false;
    } else {
        if (dir == SeekDir_Forward) {
            buffer_delete_single(buffer, view->cursor.pos + 1);
            Buffer_Cursor new_cursor = get_cursor_from_position(buffer, view->cursor.pos);
            view_set_cursor(view, new_cursor);
        } else if (dir == SeekDir_Backward) {
            buffer_delete_single(buffer, view->cursor.pos);
            Buffer_Cursor new_cursor = get_cursor_from_position(buffer, view->cursor.pos - 1);
            view_set_cursor(view, new_cursor);
        }
    }
}

internal void view_move_lines(Code_View *view, s64 move) {
    Buffer *buffer = find_text_buffer(view->buffer);
    s64 new_line = view->cursor.line + move;
    new_line = Clamp(new_line, 0, buffer_get_line_count(buffer) - 1);

    s64 line_len = buffer_get_line_length(buffer, new_line);
    Buffer_Pos line_pos = get_position_from_line(buffer, new_line);
    Buffer_Pos pos = line_pos + view->pref_col;
    pos = ClampTop(pos, line_pos + line_len);
    Buffer_Cursor cursor = get_cursor_from_position(buffer, pos);
    view_set_cursor(view, cursor);
}

internal void view_move_char(Code_View *view, s64 move) {
    Buffer *buffer = find_text_buffer(view->buffer);
    s64 len = buffer_get_line_length(buffer, view->cursor.line);
    s64 line_pos = get_position_from_line(buffer, view->cursor.line);
    s64 line_end = line_pos + len;

    Buffer_Pos pos = view->cursor.pos + move;
    pos = Clamp(pos, line_pos, line_end);

    Buffer_Cursor cursor = get_cursor_from_position(buffer, pos);
    view_set_cursor(view, cursor);
}

internal Pane *make_pane() {
    Pane *pane = (Pane *)calloc(1, sizeof(Pane));
    return pane;
}

internal Code_View *make_code_view() {
    Code_View *view = (Code_View *)calloc(1, sizeof(Code_View));
    view->buffer_arena = arena_alloc(get_virtual_allocator(), KB(64));
    view->font = default_fonts[FONT_CODE];
    view->keymap = get_normal_keymap();
    view->active = true;

    Application *app = get_application();
    view->id = app->code_view_list.count;
    DLLPushBack(app->code_view_list.first, app->code_view_list.last, view, next, prev);
    app->code_view_list.count++;

    UI_View *ui_view = (UI_View *)view;
    DLLPushBack(app->ui_view_list.first, app->ui_view_list.last, ui_view, view_next, view_prev);
    app->ui_view_list.count++;
    return view;
}

internal u64 get_next_line_index(String8 string, u64 start) {
    u64 idx;
    for (idx = start; idx < string.count; idx++) {
        if (string.data[idx] == '\n') {
            break;
        }
    }
    return idx;
}

internal s64 get_next_word_in_line(String8 string, u64 start) {
    u64 index = start;

    u64 line_end = get_next_line_index(string, start);

    //@Note Skip whitespace at start of line
    for (; index < line_end; index++) {
        if (string.data[index] != ' ') {
            break;
        }
    }

    for (; index < line_end; index++) {
        u8 c = string.data[index];
        if (c == ' ' || c == '\n' || c == '\t') {
            break;
        }
    }
    return index;
}

internal void update_view(Code_View *view) {
    Buffer *buffer = find_text_buffer(view->buffer);
    arena_clear(view->buffer_arena);
    view->buffer_string = string_from_buffer(view->buffer_arena, buffer, false);

    //@Note Update line wrapping
    // Font *font = view->box->font;
    // s64 page_lines = (s64)round_f32(rect_height(view->box->rect) / font->glyph_height);
    // s64 start_line = (s64)floor_f32(view->box->view_offset.y / font->glyph_height + 0.1f);
    // s64 start_idx = get_position_from_line(buffer, start_line);

    // f32 max_height = rect_height(view->box->rect);
    // f32 max_width = rect_width(view->box->rect);
    // V2_F32 cursor = v2_f32(0, 0);

    // String8 string = view->buffer_string;

    // //@Note Update line wrapping bounds
    // view->line_bounds.reset_count();
    // u64 line_count = buffer_get_line_count(buffer);
    // for (s64 line = 0; line < line_count; line++) {
    //     u64 line_start = buffer->line_starts[line];
    //     u64 line_end   = buffer->line_starts[line + 1];
    //     u64 idx = line_start;

    //     for (;;) {
    //         f32 space_left = max_width;

    //         for (;;) {
    //             u8 c  = string.data[idx];
    //             f32 w = get_char_width(font, c);

    //             if (idx == line_end || space_left < w) {
    //                 view->line_bounds.push(idx);
    //                 break;
    //             }

    //             space_left -= w;
    //             idx++;
    //         }
    //     }
    // }
}

internal void ui_draw_code_view_matches(String8 string, Code_View *view, Buffer *buffer) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != r_handle_zero() || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.params_ui = params_ui;
        params_ui->tex = r_handle_zero();
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->tex = r_handle_zero();
        draw_bucket->params_kind = R_ParamsKind_UI;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    Font *font = view->font;
    V4_F32 current_color = v4_f32(1.0f, 0.5f, 0.1f, 1.0f);
    V4_F32 def_color = v4_f32(0.98f, 0.74f, 0.18f, 1.0f);
    V2_F32 offset = view->box->rect.p0;
    offset.y -= view->scroll_pt.idx * font->glyph_height;

    for (int i = 0; i < view->search_matches.count; i++) {
        Rng_S64 rng = view->search_matches[i];
        Buffer_Cursor cursor = get_cursor_from_position(buffer, rng.min);

        V4_F32 color = def_color;
        if (view->cursor.pos == rng.max) color = current_color;

        f32 w = 0;
        f32 h = font->glyph_height;
        f32 x = buffer_line_width_before(buffer, cursor, font);
        f32 y = offset.y + cursor.line * font->glyph_height;

        for (s64 idx = rng.min; idx < rng.max; idx++) {
            u8 c = string.data[idx];
            w += font->glyphs[c].ax;
        }

        R_2D_Rect rect = r_2d_rect(make_rect(x, y, w, h), rect_zero(), color, 0, 0);
        draw_batch_push_rect(&node->batch, rect);
    }
}

internal void ui_draw_code_view_selection(String8 string, Code_View *view, Buffer *buffer) {
    R_Handle tex = draw_bucket->tex;
    R_Params_Kind params_kind = draw_bucket->params_kind;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != r_handle_zero() || params_kind != R_ParamsKind_UI) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        node->batch.params.kind = R_ParamsKind_UI;
        R_Params_UI *params_ui = push_array(draw_arena, R_Params_UI, 1);
        node->batch.params.params_ui = params_ui;
        params_ui->tex = r_handle_zero();
        params_ui->clip = draw_bucket->clip;
        params_ui->xform = draw_bucket->xform;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->tex = r_handle_zero();
        draw_bucket->params_kind = R_ParamsKind_UI;
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    }

    V4_F32 color = v4_f32(0.31f, 0.29f, 0.27f, 1.0f);

    Font *font = view->font;

    Buffer_Cursor start = view->marker;
    Buffer_Cursor end = view->cursor;
    if (start.pos > end.pos) Swap(Buffer_Cursor, start, end);

    V2_F32 code_area_dim = rect_dim(view->box->rect);
    int visible_lines = (int)trunc_f32(code_area_dim.y / view->font->glyph_height);
    Rng_S64 view_rng = rng_s64(view->scroll_pt.idx, view->scroll_pt.idx + visible_lines);

    if (start.line < view_rng.min) start = get_cursor_from_line(buffer, view_rng.min);
    if (end.line > view_rng.max) end = get_cursor_from_line(buffer, view_rng.max);

    V2_F32 offset = v2_f32(view->box->rect.x0, view->box->rect.y0);
    offset.y -= view->scroll_pt.idx * font->glyph_height;

    if (start.line == end.line) {
        f32 x = buffer_line_width_before(buffer, start, font);
        f32 w = buffer_string_range_width(buffer, rng_s64(start.pos, end.pos), font);
        f32 y = offset.y + font->glyph_height * start.line;
        Rect dst = make_rect(x, y, w, font->glyph_height);
        R_2D_Rect rect = r_2d_rect(dst, rect_zero(), color, 0.0f, 0.0f);
        draw_batch_push_rect(&node->batch, rect);
    } else {
        for (s64 line = start.line; line <= end.line; line++) {
            f32 x = offset.x;
            f32 y = offset.y + line * font->glyph_height;
            f32 w = 0;
            if (line == start.line) {
                x += buffer_line_width_before(buffer, start, font);
                w = buffer_line_width_after(buffer, start, font);
            } else if (line == end.line) {
                w = buffer_line_width_before(buffer, end, font);
            } else {
                s64 min = get_position_from_line(buffer, line);
                s64 len = buffer_get_line_length(buffer, line);
                Rng_S64 range = rng_s64(min, min + len);
                w = buffer_string_range_width(buffer, range, font);
            }

            Rect dst = make_rect(x, y, w, font->glyph_height);
            R_2D_Rect rect = r_2d_rect(dst, rect_zero(), color, 0.0f, 0.0f);
            draw_batch_push_rect(&node->batch, rect);
        }
    }
}

internal void ui_draw_code_view_text(String8 string, Code_View *view, Buffer *buffer) {
    R_Handle tex = draw_bucket->tex;
    Font *font = view->font;
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

    V4_F32 color = view->box->text_color;

    s64 page_lines = (s64)round_f32(rect_height(view->box->rect) / font->glyph_height);
    s64 start_line = (s64)floor_f32(view->box->view_offset.y / font->glyph_height + 0.1f);
    s64 start_idx = get_position_from_line(buffer, start_line);

    s64 end_line = ClampTop(start_line + page_lines + 1, (s64)buffer->line_starts.count - 1);
    s64 end_idx = get_position_from_line(buffer, end_line);

    V2_F32 cursor = v2_f32(0, 0);
    for (s64 idx = start_idx; idx < end_idx; idx++) {
        u8 c = string.data[idx];
        Glyph g = font->glyphs[c];

        if (c == '\n') {
            cursor.x = 0;
            cursor.y += font->glyph_height;
            continue;
        }

        Rect dst = make_rect(
            view->box->rect.x0 + cursor.x + g.bl,
            view->box->rect.y0 + cursor.y - g.bt + font->ascend,
            g.bx, g.by);

        Rect src = make_rect(
            g.to, 0.0f,
            g.bx / (f32)font->width, g.by / (f32)font->height);

        R_2D_Rect rect = r_2d_rect(dst, src, color, 0.0f, 0.0f);
        draw_batch_push_rect(&node->batch, rect);
        cursor.x += g.ax;
    }
}

struct UI_Code_View_Draw_Data {
    Code_View *view;
    Buffer *buffer;
};

UI_BOX_CUSTOM_DRAW_PROC(ui_draw_view) {
    UI_Code_View_Draw_Data *draw_data = (UI_Code_View_Draw_Data *)user_data;
    Code_View *view = draw_data->view;
    Buffer *buffer = draw_data->buffer;
    Font *font = view->font;

    draw_ui_rect(box->rect, box->background_color, 0.0f);

    if (view->marker_active) {
        ui_draw_code_view_selection(view->buffer_string, view, buffer);  
    }

    if (get_search_box()->active) ui_draw_code_view_matches(view->buffer_string, view, buffer);

    String8 string = view->buffer_string;
    ui_draw_code_view_text(view->buffer_string, view, buffer);

    //@Note Draw cursor
    {
        Rect cursor_rect = {};
        s64 line_pos = get_position_from_line(buffer, view->cursor.line);
        String8 before_cursor = str8(string.data + line_pos, (u64)view->cursor.col);

        cursor_rect.y0 = box->rect.y0 + box->font->glyph_height * view->cursor.line - view->box->view_offset.y;
        cursor_rect.y1 = cursor_rect.y0 + box->font->glyph_height;
        cursor_rect.x0 = box->rect.x0 + get_string_width(before_cursor, box->font);
        cursor_rect.x1 = cursor_rect.x0 + get_string_width({ string.data + line_pos + view->cursor.col, 1 }, box->font);
        draw_ui_rect(cursor_rect, box->text_color, 0.0f);

        //@Note Draw cursor character
        {
            R_Batch_Node *node = draw_bucket->batches.last;
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

            u8 c = buffer_at(buffer, view->cursor.pos);
            Glyph g = font->glyphs[c];
            Rect dst = make_rect(
                cursor_rect.x0 + g.bl,
                cursor_rect.y0 - g.bt + font->ascend,
                g.bx, g.by);
            Rect src = make_rect(
                g.to, 0.0f,
                g.bx / (f32)font->width, g.by / (f32)font->height);
            R_2D_Rect rect = r_2d_rect(dst, src, v4_f32(0.08f, 0.14f, 0.30f, 1.0f), 0.0f, 0.0f);
            draw_batch_push_rect(&node->batch, rect);
        }
    }
}

internal void view_split_pane(Code_View *view, Axis2 axis) {
    Application *app = get_application();

    Code_View *new_view = make_code_view();
    new_view->buffer = view->buffer;

    Pane *parent = view->pane->parent;
    Pane *pane = view->pane;
    Pane *new_pane = make_pane();
    new_pane->view = new_view;

    Pane *container = make_pane();
    if (parent) {
        if (parent->tl == pane) parent->tl = container;
        else parent->br = container;
    } else {
        set_root_pane(container);
    }

    container->pct_dim = parent ? parent->pct_dim : pane->pct_dim;
    container->axis = axis;
    container->tl = pane;
    container->br = new_pane;

    pane->parent = container;
    new_pane->parent = container;

    V2_F32 new_dim = container->pct_dim;
    new_dim[axis] *= 0.5f;
    pane->pct_dim = new_dim;
    new_pane->pct_dim = new_dim;

    new_view->pane = new_pane;

    app->active_code_view = new_view->id;
}
