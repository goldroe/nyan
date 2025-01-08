internal UI_Signal ui_label(String8 string) {
    UI_Box *box = ui_make_box_from_key(UI_BoxFlag_DrawText, 0);
    ui_box_set_string(box, string);
    return ui_signal_from_box(box);
}

internal UI_Signal ui_labelf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    return ui_label(string);
}

internal UI_Signal ui_button(String8 string) {
    ui_set_next_hover_cursor(OS_Cursor_Hand);
    UI_Box *box = ui_make_box_from_string(UI_BoxFlag_Clickable | UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText | UI_BoxFlag_DrawHotEffects | UI_BoxFlag_DrawActiveEffects, string);
    return ui_signal_from_box(box);
}

internal UI_Signal ui_buttonf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    return ui_button(string);
}

internal UI_Signal ui_checkbox(b32 *value, String8 string) {
    UI_Box *check_box = NULL;
    ui_set_next_child_layout_axis(Axis_X);
    ui_set_next_pref_width(ui_children_sum(1.0f));
    ui_set_next_pref_height(ui_children_sum(1.0f));
    UI_Box *container = ui_make_box_from_string(UI_BoxFlag_Nil, string);
    UI_Parent(container) {
        ui_set_next_font(default_fonts[FONT_ICON]);
        UI_Icon_Kind icon = *value ? UI_IconKind_Check : UI_IconKind_CheckEmpty;
        String8 icon_string = ui_string_from_icon_kind(icon, "###_checkbox");
        ui_set_next_pref_width(ui_text_dim(4.0f, 1.0f));
        ui_set_next_pref_height(ui_text_dim(2.0f, 1.0f));
        check_box = ui_make_box_from_string(UI_BoxFlag_DrawText|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable, icon_string);
        ui_label(string);
    }
    UI_Signal sig = ui_signal_from_box(check_box);
    if (sig.flags & UI_SignalFlag_Clicked) {
        *value = !*value;
    }

    return sig;
}

struct UI_Graph_Draw_Data {
    f32 *values;
    int values_count;
    V4_F32 color;
};

internal UI_BOX_CUSTOM_DRAW_PROC(ui_draw_graph) {
    UI_Graph_Draw_Data *draw_data = (UI_Graph_Draw_Data *)user_data;
    V2_F32 box_dim = rect_dim(box->rect);
    f32 line_width = floor_f32((box_dim.x / (f32)draw_data->values_count));
    f32 line_height = box_dim.y;
    V2_F32 start = v2_f32(box->rect.x0, box->rect.y1);
    draw_ui_graph(draw_data->values, draw_data->values_count, line_width, line_height, start, draw_data->color);
}

internal UI_Signal ui_bar_graph(f32 *values, int values_count, V4_F32 line_color, String8 string) {
    UI_Box *box = ui_make_box_from_string(UI_BoxFlag_DrawBackground, string);

    UI_Graph_Draw_Data *draw_data = push_array(ui_build_arena(), UI_Graph_Draw_Data, 1);
    draw_data->values = values;
    draw_data->values_count = values_count;
    draw_data->color = line_color;
    ui_set_custom_draw(box, ui_draw_graph, draw_data);

    UI_Signal signal = ui_signal_from_box(box);
    return signal;
}

struct UI_Image_Draw_Data {
    R_Handle tex;
    Rect src;
};

internal UI_BOX_CUSTOM_DRAW_PROC(ui_draw_img) {
    UI_Image_Draw_Data *draw_data = (UI_Image_Draw_Data *)user_data;
    V2_F32 box_dim = rect_dim(box->rect);
    V2_F32 img_dim = box_dim * 0.9f;
    V2_S32 tex_size = r_texture_size(draw_data->tex);
    f32 img_ar = (f32)tex_size.x / (f32)tex_size.y;
    f32 box_ar = img_dim.x / img_dim.y;
    V2_F32 adjusted = img_dim;
    if (img_ar > box_ar) {
        adjusted.y = img_dim.x / img_ar;
    } else if (img_ar < box_ar) {
        adjusted.x = img_dim.y * img_ar;
    }
    Rect dst = make_rect(box->rect.x0, box->rect.y0, adjusted.x, adjusted.y);
    shift_rect(&dst, (box_dim.x - adjusted.x)/2.f, (box_dim.y - adjusted.y)/2.f);
    draw_ui_rect(box->rect, v4_f32(0.2f, 0.2f, 0.2f, 1.f), box->border_thickness);
    draw_ui_img(draw_data->tex, dst, draw_data->src, v4_f32(1.f, 1.f, 1.f, 1.f));
}

internal UI_Signal ui_image(R_Handle img, String8 string) {
    UI_Box *box = ui_make_box_from_string(UI_BoxFlag_Nil, string);

    UI_Image_Draw_Data *draw_data = push_array(ui_build_arena(), UI_Image_Draw_Data, 1);
    draw_data->tex = img;
    draw_data->src = make_rect(0.f, 0.f, 1.f, 1.f);
    ui_set_custom_draw(box, ui_draw_img, draw_data);

    UI_Signal signal = ui_signal_from_box(box);
    return signal;
}

internal UI_Signal ui_imagef(R_Handle img, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    UI_Signal signal = ui_image(img, string);
    return signal;
}

struct UI_Line_Edit_Draw_Data {
    String8 edit_string;
    s64 cursor;
};

internal UI_BOX_CUSTOM_DRAW_PROC(ui_draw_line_edit) {
    UI_Line_Edit_Draw_Data *draw_data = (UI_Line_Edit_Draw_Data *)user_data;
    String8 edit_string = draw_data->edit_string;
    box->string = edit_string;
    V2_F32 text_position = ui_text_position(box);
    text_position += box->view_offset;
    draw_text(edit_string, box->font, box->text_color, text_position);

    String8 string_before_cursor = edit_string;
    string_before_cursor.count = (u64)draw_data->cursor;
    V2_F32 c_pos = text_position + measure_string_size(string_before_cursor, box->font);
    Rect c_rect = make_rect(c_pos.x, c_pos.y, 2.f, box->font->glyph_height);
    if (ui_key_match(box->key, ui_state->focus_active_box_key)) {
        draw_ui_rect(c_rect, box->text_color, 1.f);
    }
}

internal void ui_text_edit_insert(void *buffer, u64 max_buffer_capacity, u64 *buffer_pos,  u64 *buffer_count, String8 insertion) {
    u64 count = insertion.count;
    count = ClampTop(count, max_buffer_capacity);
    if (insertion.count > 0) {
        MemoryCopy(buffer, insertion.data, count);
    }
    *buffer_count = count;
    *buffer_pos = count;
}

internal void ui_text_edit_op(void *buffer, u64 max_buffer_capacity, u64 *buffer_pos,  u64 *buffer_count) {
    u64 pos = *buffer_pos;
    u64 count = *buffer_count;
    if (ui_key_press(OS_KEY_LEFT)) {
        if (pos > 0) {
            MemoryCopy((u8*)buffer + pos - 1, (u8*)buffer + pos, count - pos);
            pos -= 1;
            count -= 1;
        }
    }
    if (ui_key_press(OS_KEY_RIGHT)) {
        if (pos > 0) {
            pos -= 1;
        }
    }
    if (ui_key_press(OS_KEY_BACKSPACE)) {
        if (pos < count) {
            pos += 1;
        }
    }
    *buffer_pos = pos;
    *buffer_count = count;
}

internal UI_Signal ui_line_edit(String8 name, void *buffer, u64 max_buffer_capacity, u64 *buffer_pos,  u64 *buffer_count) {
    ui_set_next_hover_cursor(OS_Cursor_Ibeam);
    UI_Box *box = ui_make_box_from_string(UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable | UI_BoxFlag_ClickToFocus | UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawHotEffects | UI_BoxFlag_DrawActiveEffects, name);
    UI_Signal signal = ui_signal_from_box(box);

    u64 pos = *buffer_pos;
    u64 count = *buffer_count;

    if (signal.text.data) {
        u64 text_count = Min(signal.text.count, max_buffer_capacity - count);
        if (pos == text_count) {
            MemoryCopy((u8*)buffer + pos, signal.text.data, text_count);
        } else {
            MemoryCopy((u8*)buffer + pos + text_count, (u8*)buffer + pos, text_count);
            MemoryCopy((u8*)buffer + pos, signal.text.data, text_count);
        }
        pos += text_count;
        count += text_count;
    }

    if (ui_pressed(signal)) {
        switch (signal.key) {
        case OS_KEY_BACKSPACE:
            if (pos > 0) {
                MemoryCopy((u8*)buffer + pos - 1, (u8*)buffer + pos, count - pos);
                pos -= 1;
                count -= 1;
            }
            break;
        case OS_KEY_LEFT:
            if (pos > 0) {
                pos -= 1;
            }
            break; 
        case OS_KEY_RIGHT:
            if (pos < count) {
                pos += 1;
            }
            break;
        }
    }
 
    *buffer_count = count;
    *buffer_pos = pos;

    UI_Line_Edit_Draw_Data *draw_data = push_array(ui_build_arena(), UI_Line_Edit_Draw_Data, 1);
    draw_data->edit_string = str8((u8 *)buffer, *buffer_count);
    draw_data->cursor = *buffer_pos;
    ui_set_custom_draw(box, ui_draw_line_edit, draw_data);
    return signal;
}

internal UI_Scroll_Pt ui_scroll_bar(String8 name, Axis2 axis, UI_Size flip_axis_size, UI_Scroll_Pt scroll_pt, Rng_S64 view_rng, s64 view_indices) {
    UI_Scroll_Pt new_pt = scroll_pt;
    Axis2 flip_axis = axis_flip(axis);

    s64 scroll_indices = view_indices + rng_s64_len(view_rng);
    f32 scroll_ratio = (f32)rng_s64_len(view_rng) / (f32)scroll_indices;
    
    ui_set_next_pref_size(axis, ui_pct(1.f, 0.f));
    ui_set_next_pref_size(flip_axis, flip_axis_size);
    ui_set_next_child_layout_axis(axis);
    ui_set_next_hover_cursor(OS_Cursor_Hand);
    UI_Box *container = ui_make_box_from_stringf(UI_BoxFlag_DrawBackground, "###container_%S", name);
    UI_Parent(container)
        UI_PrefSize(flip_axis, ui_pct(1.f, 0.f)) {
        ui_set_next_pref_size(axis, ui_text_dim(0.f, 1.f));
        ui_set_next_font(default_fonts[FONT_ICON]);
        String8 tl_arrow_string = ui_string_from_icon_kind(axis==Axis_X ? UI_IconKind_ArrowLeft : UI_IconKind_ArrowUp, "###tl_arrow");
        UI_Signal top_sig = ui_button(tl_arrow_string);
        if (ui_clicked(top_sig) || ui_dragging(top_sig)) {
            new_pt.idx -= 1;
        }

        ui_set_next_pref_size(axis, ui_pct(1.f, 0.f));
        ui_set_next_background_color(v4_f32(.24f, .25f, .25f, 1.f));
        ui_set_next_hover_cursor(OS_Cursor_Hand);
        ui_set_next_border_thickness(8.f);
        UI_Box *thumb_container = ui_make_box_from_stringf(UI_BoxFlag_Clickable | UI_BoxFlag_DrawBackground, "###thumb_container", name.data);
        V2_F32 thumb_container_dim = rect_dim(thumb_container->rect);
            
        UI_Signal scroll_sig = ui_signal_from_box(thumb_container);

        if (ui_clicked(scroll_sig)) {
            V2_F32 scroll_pos = ui_mouse() - thumb_container->rect.p0;
            new_pt.idx = (s64)(scroll_pos[axis] / (thumb_container_dim[axis] / (f32)view_indices));
        }

        f32 thumb_pos = thumb_container_dim[axis] * ((f32)scroll_pt.idx / (f32)scroll_indices);
        ui_set_next_parent(thumb_container);
        ui_set_next_fixed_xy(axis, thumb_pos);
        ui_set_next_fixed_xy(flip_axis, 0.f);
        ui_set_next_pref_size(axis, ui_pct(scroll_ratio, 0.f));
        ui_set_next_background_color(v4_f32(.4f, .4f, .4f, 1.f));
        ui_set_next_hover_cursor(OS_Cursor_Hand);
        ui_set_next_border_thickness(8.f);
        UI_Box *thumb_box = ui_make_box_from_stringf(UI_BoxFlag_Clickable | UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawHotEffects | UI_BoxFlag_DrawActiveEffects, "###thumb", name.data);
        UI_Signal thumb_sig = ui_signal_from_box(thumb_box);
        if (ui_dragging(thumb_sig)) {
            V2_F32 scroll_pos = ui_mouse() - thumb_container->rect.p0;
            new_pt.idx = (s64)(scroll_pos[axis] / (thumb_container_dim[axis] / (f32)view_indices));
        }

        ui_set_next_pref_size(axis, ui_text_dim(0.f, 1.f));
        String8 br_arrow_string = ui_string_from_icon_kind(axis==Axis_X ? UI_IconKind_ArrowRight : UI_IconKind_ArrowDown, "###br_arrow");
        ui_set_next_font(default_fonts[FONT_ICON]);
        UI_Signal bottom_sig = ui_button(br_arrow_string);
        if (ui_clicked(bottom_sig) || ui_dragging(bottom_sig)) {
            new_pt.idx += 1;
        }
    }

    new_pt.idx = Clamp(new_pt.idx, 0, view_indices - 1);

    return new_pt;
}


#if 0
internal void ui_slider(String8 name, f32 *value, f32 min, f32 max) {
    V2_F32 mouse = ui_mouse();
    UI_Box *found = ui_find_box(hash);
    bool hot = false;
    if (found) {
        hot = point_in_rect(mouse, found->rect);
    }
    UI_Box *box = ui_make_box_from_string(name, 0);
    if (found) ui_copy_box(box, found);

    if (ui_active_id() == hash) {
        if (!input_mouse(MOUSE_LEFT)) {
            ui_set_active(0);
        }
    } else {
        if (hot && input_mouse_down(MOUSE_LEFT)) {
            ui_set_active(hash);
        }
    }

    if (ui_active_id() == hash) {
        f32 ratio = (CLAMP(mouse.x, box->rect.x0, box->rect.x1) - box->rect.x0) / rect_width(box->rect);
        f32 c = min + (max - min) * ratio;
        *value = c;
    }

    int name_count = (int)strlen(name);
    f32 width = 2.0f * get_string_width(name, name_count, ui_face);
    f32 height = ui_face->glyph_height;

    ui_advance();

    box->rect = make_rect(ui_state.cursor, V2(width, height));

    *value = CLAMP(*value, min, max);
    
    f32 ratio = (*value - min) / (max - min);
    f32 overlay_width = ratio * rect_width(box->rect);

    Rect overlay_rect = { box->rect.x0, box->rect.y0 };
    overlay_rect.x1 = overlay_rect.x0 + overlay_width;
    overlay_rect.y1 = box->rect.y1;

    V4_F32 bg_color = V4(1.0f, 1.0f, 1.0f, 1.0f);
    V4_F32 overlay_color = V4(0.1f, 0.1f, 0.1f, 0.2f);
    V4_F32 fg_color = V4(0.0f, 0.0f, 0.0f, 1.0f);
    V4_F32 border_color = V4(0.0f, 0.0f, 0.0f, 1.0f);

    draw_ui_rect(box->rect, bg_color);
    draw_ui_rect_outline(box->rect, border_color);
    draw_text(name, name_count, ui_state.cursor, fg_color, ui_face);
    draw_ui_rect(overlay_rect, overlay_color);

    ui_state.cursor.y = box->rect.y1;
}
#endif

internal UI_Signal ui_directory_list(String8 directory) {
    ui_set_next_child_layout_axis(Axis_Y);
    UI_Signal signal = ui_button(directory);
    UI_Parent(signal.box)
    UI_PrefWidth(ui_pct(1.0f, 1.0f)) UI_PrefHeight(ui_text_dim(0.f, 1.f)) {
        OS_File file;
        OS_Handle find_handle = os_find_first_file(ui_build_arena(), directory, &file);
        if (find_handle) {
            do {
                UI_Signal file_signal = ui_label(file.file_name);
                if (ui_clicked(file_signal)) {
                }
            } while (os_find_next_file(ui_build_arena(), find_handle, &file));
            os_find_close(find_handle);
        }
    }
    return signal;
}

internal void ui_spacer(Axis2 axis, UI_Size size) {
    ui_set_next_pref_size(axis, size);
    UI_Box *box = ui_make_box_from_key(UI_BoxFlag_Nil, 0);
}

internal void ui_column_begin() {
    ui_set_next_child_layout_axis(Axis_Y);
    UI_Box *column = ui_make_box_from_key(UI_BoxFlag_Nil, 0);
    ui_push_parent(column);
}

internal UI_Box *ui_row_begin() {
    ui_set_next_child_layout_axis(Axis_X);
    UI_Box *row = ui_make_box_from_key(UI_BoxFlag_DrawBackground, 0);
    ui_push_parent(row);
    return row;
}

internal UI_Signal ui_row_end() {
    UI_Box *box = ui_pop_parent();
    UI_Signal signal = ui_signal_from_box(box);
    return signal;
}
