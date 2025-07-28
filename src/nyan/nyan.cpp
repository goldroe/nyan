global Arena *frame_arena;
global Application *g_application;
global String8 file_argument;

internal Application *get_application() {
    return g_application;
}

internal void set_root_pane(Pane *pane) {
    g_application->root_pane = pane;
}

internal Buffer *find_text_buffer(Buffer_ID id) {
    Buffer *result = NULL;
    for (Buffer *buffer = g_application->buffer_list.first; buffer; buffer = buffer->next) {
        if (buffer->id == id) {
            result = buffer;
            break;
        }
    }
    return result;
}

internal Code_View *find_code_view(Code_View_ID id) {
    Code_View *result = NULL;
    for (Code_View *view = g_application->code_view_list.first; view; view = view->next) {
        if (view->id == id) {
            result = view;
            break;
        }
    }
    return result;
}

internal UI_View *get_active_view() {
    UI_View *result = NULL;
    for (UI_View *view = g_application->ui_view_list.first; view; view = view->view_next) {
        if (view->key == ui_state->focus_active_box_key) {
            result = view;
            break;
        }
    }
    return result;
}

internal Code_View *get_active_code_view() {
    Code_View *result = find_code_view(g_application->active_code_view);
    return result;
}

internal void resolve_pane_node(Pane *pane, V2_F32 dim, V2_F32 p) {
    if (pane == NULL) return;
    
    Pane *tl = pane->tl;
    Pane *br = pane->br;
    bool is_parent = tl != NULL && br != NULL;

    pane->fixed_pos = p;
    pane->fixed_dim = dim;

    if (is_parent) {
        V2_F32 tl_dim = v2_f32(tl->pct_dim.x * dim.x, tl->pct_dim.y * dim.y);
        V2_F32 br_dim = v2_f32(br->pct_dim.x * dim.x, br->pct_dim.y * dim.y);
        V2_F32 br_p = p;
        br_p[pane->axis] += tl_dim[pane->axis];

        resolve_pane_node(tl, tl_dim, p);
        resolve_pane_node(br, br_dim, br_p);
    }
}

internal void resolve_pane_layout(Pane *root, V2_F32 root_dim) {
    resolve_pane_node(root, root_dim, v2_f32(0, 0));
}

internal void update_and_render(OS_Event_List *events, OS_Handle window_handle, f32 dt) {
    local_persist b32 first_call = true;
    if (first_call) {
        first_call = false;

        frame_arena = arena_alloc(get_virtual_allocator(), MB(1));

        Arena *font_arena = arena_alloc(get_virtual_allocator(), MB(4));
        default_fonts[FONT_DEFAULT] = load_font(font_arena, str8_lit("data/assets/fonts/RobotoMono.ttf"), 14);

        default_fonts[FONT_CODE] = load_font(font_arena, str8_lit("data/assets/fonts/RobotoMono.ttf"), 14);

        u32 icon_font_glyphs[] = { 87, 120, 33, 49, 85, 68, 76, 82, 123, 125, 67, 70, 35 };
        default_fonts[FONT_ICON] = load_icon_font(font_arena, str8_lit("data/assets/fonts/icons.ttf"), 16, icon_font_glyphs, ArrayCount(icon_font_glyphs));

        ui_set_state(ui_state_new());

        g_application = (Application *)calloc(1, sizeof(Application));

        g_keymap_insert = make_insert_keymap();
        g_keymap_normal = make_normal_keymap();
        g_keymap_select = make_select_keymap();
        g_keymap_goto = make_goto_keymap();
        g_keymap_space = make_space_keymap();
        g_keymap_picker = make_picker_keymap();
        g_keymap_search_box = make_search_box_keymap();

        String8 file_path = file_argument;
        if (path_is_relative(file_argument)) {
            String8 current_dir = os_current_dir(frame_arena);
            file_path = path_join(frame_arena, current_dir, file_argument);
        }

        g_file_picker = make_file_picker();
        g_minibuffer = make_minibuffer();
        g_search_box = make_search_box();
        g_search_box->keymap = g_keymap_search_box;
        
        Buffer *default_buffer = make_buffer(file_path);
        Code_View *default_view = make_code_view();
        default_view->id = 0;
        default_view->buffer = default_buffer->id;
        default_view->font = default_fonts[FONT_CODE];
        default_view->keymap = g_keymap_normal;

        Pane *default_pane = make_pane();
        default_pane->parent = NULL;
        default_pane->view = default_view;
        default_pane->pct_dim = fill_v2_f32(1.0f);

        default_view->pane = default_pane;

        g_application->active_code_view = default_view->id;

        g_application->root_pane = default_pane;
    }

    V2_F32 window_dim = os_get_window_dim(window_handle);

    draw_begin(window_handle);

    ui_begin_build(dt * 1000, window_handle, events);

    V4_F32 default_fg_color = v4_f32(0.92f, 0.86f, 0.7f, 1.0f);
    V4_F32 default_bg_color = v4_f32(0.20f, 0.19f, 0.18f, 1.0f);

    //@Note GUI Editor
    V2_F32 editor_area_pos = v2_f32(0, 0);
    V2_F32 editor_area_dim = window_dim;
    resolve_pane_layout(g_application->root_pane, editor_area_dim);

    ui_set_next_pref_width(ui_px(editor_area_dim.x, 0.0f));
    ui_set_next_pref_height(ui_px(editor_area_dim.y, 0.0f));
    UI_Box *editor_container = ui_make_box_from_string(UI_BoxFlag_Nil, str8_lit("###editor_cont"));
    for (Code_View *view = g_application->code_view_list.first; view; view = view->next) {
        Pane *pane = view->pane;
        Buffer *buffer = find_text_buffer(view->buffer);
        V2_F32 view_pos = pane->fixed_pos + editor_area_pos;
        V2_F32 view_dim = pane->fixed_dim;
        Rect view_rect = make_rect(view_pos.x, view_pos.y, view_dim.x, view_dim.y);
        ui_set_next_fixed_x(view_pos.x);
        ui_set_next_fixed_y(view_pos.y);
        ui_set_next_pref_width(ui_px(view_dim.x, 0.0f));
        ui_set_next_pref_height(ui_px(view_dim.y, 0.0f));
        ui_set_next_background_color(v4_f32(0.97f, 0.95f, 0.93f, 1.0f));
        UI_Box *container = ui_make_box_from_stringf(UI_BoxFlag_DrawBackground, "###view_%d", view->id);
        UI_Parent(container)
            UI_TextColor(default_fg_color)
            UI_BackgroundColor(default_bg_color)
            UI_ClipBox(view_rect)
        {
            ui_set_next_pref_width(ui_pct(1.0f, 1.0f));
            ui_set_next_pref_height(ui_text_dim(1.0f, 1.0f));
            ui_set_next_text_alignment(UI_TextAlign_Left);
            ui_set_next_child_layout_axis(Axis_X);
            UI_Box *file_bar_box = ui_make_box_from_stringf(UI_BoxFlag_DrawBackground, "###file_bar_%d", view->id);

            UI_Parent(file_bar_box)
                UI_PrefWidth(ui_text_dim(2.0f, 1.0f))
                UI_PrefHeight(ui_text_dim(2.0f, 1.0f))
                UI_TextColor(v4_f32(1.0f, 1.0f, 1.0f, 1.0f))
            {
                V4_F32 modal_color = view->modal_state == ModalState_Normal ? v4_f32(0.92f, 0.8f, 0.55f, 1.0f) : v4_f32(0.8f, 0.8f, 0.8f, 1.0f);
                char *modal_string = g_modal_state_strings[view->modal_state];
                ui_set_next_pref_width(ui_text_dim(4.0f, 1.0f));
                ui_set_next_pref_height(ui_text_dim(2.0f, 1.0f));
                ui_set_next_background_color(modal_color);
                ui_make_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawText, modal_string);

                ui_spacer(Axis_X, ui_px(view->font->glyph_width, 1.0f));

                ui_labelf("%s", buffer->file_name.data);

                ui_spacer(Axis_X, ui_pct(1.0f, 0.0f));

                ui_set_next_pref_width(ui_text_dim(4.0f, 1.0f));
                ui_set_next_pref_height(ui_text_dim(2.0f, 1.0f));
                ui_set_next_background_color(v4_f32(0.69f, 0.75f, 0.77f, 1.0f));
                ui_make_box_from_stringf(UI_BoxFlag_DrawText, "%lld:%lld", view->cursor.line + 1, view->cursor.col);
            }

            ui_set_next_font(view->font);
            ui_set_next_text_alignment(UI_TextAlign_Left);
            ui_set_next_pref_width(ui_pct(1.0f, 0.0f));
            ui_set_next_pref_height(ui_pct(1.0f, 0.0f));
            view->box = ui_make_box_from_stringf(UI_BoxFlag_DefaultFocus|UI_BoxFlag_ClickToFocus|UI_BoxFlag_Clickable|UI_BoxFlag_KeyboardClickable, "###buffer_%d", view->id);
            view->signal = ui_signal_from_box(view->box);

            view->key = view->box->key;

            update_view(view);

            view->box->view_offset_target.y = view->box->view_offset.y = (f32)view->scroll_pt.idx * view->font->glyph_height;

            UI_Code_View_Draw_Data *draw_data = push_array(ui_build_arena(), UI_Code_View_Draw_Data, 1);
            draw_data->view = view;
            draw_data->buffer = find_text_buffer(view->buffer);
            ui_set_custom_draw(view->box, ui_draw_view, (void *)draw_data);
        }
    }

    //@Note Minibuffer
    if (0) {
        Minibuffer *minibuffer = g_minibuffer;
        ui_txt_buffer(&minibuffer->text, str8_lit("minibuffer"));

        ui_set_next_pref_width(ui_pct(1.0f, 0.0f));
        ui_set_next_pref_height(ui_text_dim(1.0f, 1.0f));
        ui_set_next_text_alignment(UI_TextAlign_Left);
        ui_set_next_background_color(default_bg_color);
        ui_set_next_text_color(default_fg_color);
        minibuffer->box = ui_make_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawText, "%s###minibuffer", minibuffer->text.data);
        minibuffer->signal = ui_signal_from_box(minibuffer->box);

        minibuffer->key = minibuffer->signal.box->key;
    }

    //@Note File Picker
    UI_TextColor(default_fg_color)
    UI_BackgroundColor(default_bg_color)
    {
        File_Picker *picker = g_file_picker;

        V2_F32 root_dim = window_dim;
        V2_F32 dim = 0.9f * root_dim;
        V2_F32 p = v2_f32(0.5f * root_dim.x - 0.5f * dim.x, 0.5f * root_dim.y - 0.5f * dim.y);

        ui_set_next_fixed_x(p.x);
        ui_set_next_fixed_y(p.y);
        ui_set_next_fixed_width(dim.x);
        ui_set_next_fixed_height(dim.y);
        ui_set_next_child_layout_axis(Axis_Y);
        ui_set_next_border_color(v4_f32(0.38f, 0.35f, 0.32f, 1.0f));
        if (!picker->active) ui_set_next_box_flags(UI_BoxFlag_Disabled);
        picker->box = ui_make_box_from_string(UI_BoxFlag_ClickToFocus|UI_BoxFlag_DefaultFocus|UI_BoxFlag_KeyboardClickable|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, str8_lit("###file_picker"));

        picker->signal = ui_signal_from_box(picker->box);

        if (ui_pressed(picker->signal)) {
            picker->keymap->bindings[picker->signal.key].command();
        }

        ui_set_next_parent(picker->box);
        ui_set_next_pref_width(ui_pct(1.0f, 0.0f));
        ui_set_next_pref_height(ui_text_dim(1.0f, 1.0f));
        UI_Box *prompt_bar = ui_make_box_from_stringf(UI_BoxFlag_DrawText, "%s###file_prompt", picker->path_buffer.data);

        if (ui_pressed(picker->signal) && picker->signal.key == OS_KEY_ENTER) {
            OS_File file = picker->cached_files[picker->file_index];
            String8 picker_path = str8(picker->path_buffer.data, picker->path_buffer.count);
            String8 full_path = path_join(frame_arena, picker_path, file.file_name);
            if (file.flags & OS_FileFlag_Directory) {
                file_picker_prompt(picker, full_path);
            } else {
                Buffer *buffer = make_buffer(full_path);

                Code_View *view = get_active_code_view();
                view->buffer = buffer->id;
                view->cursor = buffer_cursor_zero();
                view->scroll_pt = ui_scroll_pt(0, 0);
                view->keymap = g_keymap_normal;
                picker->active = false;
                ui_set_focus_active_key(view->box->key);
            }
        }

        if (ui_pressed(picker->signal) && picker->signal.key == OS_KEY_BACKSPACE) {
            String8 path = str8(picker->path_buffer.data, picker->path_buffer.count);
            String8 parent = path_strip_dir_name(frame_arena, path);
            OS_File_Flags parent_flags = os_file_attributes(parent);
            if (parent_flags & OS_FileFlag_Directory) {
                file_picker_prompt(picker, parent);
            }
        }

        if (ui_pressed(picker->signal)) {
            if (picker->signal.key == OS_KEY_UP) picker->file_index--;
            if (picker->signal.key == OS_KEY_DOWN) picker->file_index++;
        }
        if (picker->file_index < 0) picker->file_index = picker->cached_file_count - 1;
        if (picker->file_index >= picker->cached_file_count) picker->file_index = 0;

        UI_Parent(picker->box)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_PrefWidth(ui_text_dim(2.0f, 1.0f))
            UI_PrefHeight(ui_text_dim(2.0f, 1.0f))
        {
            String8 arrow_string = ui_string_from_icon_kind(UI_IconKind_ArrowRight, "###right_arrow");

            for (int idx = 0; idx < picker->cached_file_count; idx++) {
                OS_File file = picker->cached_files[idx];
                ui_set_next_child_layout_axis(Axis_X);
                UI_Box *row = ui_make_box_from_stringf(UI_BoxFlag_Nil, "###row_%d", idx);
                UI_Parent(row) {
                    if (idx == picker->file_index) {
                        ui_set_next_font(default_fonts[FONT_ICON]);
                        ui_label(arrow_string);
                    } else {
                        f32 w = get_string_width(arrow_string, default_fonts[FONT_ICON]);
                        ui_spacer(Axis_X, ui_px(w, 1.0f));
                    }
                    ui_label(file.file_name);
                }
            }
        }

        picker->key = picker->box->key;
    }

    //@Note Search
    UI_TextColor(default_fg_color)
        UI_BackgroundColor(default_bg_color)
    {
        Code_View *code_view = get_active_code_view();
        Buffer *buffer = find_text_buffer(code_view->buffer);

        Search_Box *search_box = g_search_box;
        ui_set_next_fixed_x(400);
        ui_set_next_fixed_y(0);
        ui_set_next_pref_width(ui_children_sum(1.0f));
        ui_set_next_pref_height(ui_children_sum(1.0f));
        ui_set_next_background_color(v4_f32(0.17f, 0.16f, 0.16f, 1.0f));
        if (!search_box->active) ui_set_next_box_flags(UI_BoxFlag_Disabled);
        ui_set_next_child_layout_axis(Axis_X);
        UI_Box *container = ui_make_box_from_string(UI_BoxFlag_DrawBackground, str8_lit("###search_box"));
        UI_Parent(container) {
            ui_label(str8_lit("Text"));
            ui_set_next_border_color(v4_f32(0.31f, 0.29f, 0.27f, 1.0f));
            search_box->signal = ui_line_edit(str8_lit("search_cont"), &search_box->prompt.data, ArrayCount(search_box->prompt.data), &search_box->prompt.pos, &search_box->prompt.count);
            search_box->key = search_box->signal.box->key;
        }
    }

    //@Note Dialog
#if 0
    UI_BackgroundColor(v4_f32(0.0f, 0.0f, 0.0f, 1.0f))
    UI_BackgroundColor(v4_f32(0.81f, 0.75f, 0.66f, 1.0f))
        UI_TextColor(v4_f32(0.1f, 0.1f, 0.1f, 1.0f))
    {
        V2_F32 dim = v2_f32(360.0f, 360.0f);
        V2_F32 pos = window_dim - dim;
        ui_set_next_fixed_width(dim.x);
        ui_set_next_fixed_height(dim.y);
        ui_set_next_fixed_x(pos.x);
        ui_set_next_fixed_y(pos.y);
        ui_set_next_child_layout_axis(Axis_Y);
        UI_Box *container = ui_make_box_from_string(UI_BoxFlag_DrawBackground, str8_lit("###modal_dialog"));
        UI_Parent(container) {
            ui_label(str8_lit("C-W h move_left"));
            ui_label(str8_lit("C-W l move_right"));
        }
    }
#endif

    {
        UI_View *view = get_active_view();
        if (view) {
            view->text_input = view->signal.text;
            if (ui_pressed(view->signal)) {
                u32 key = (u16)view->signal.key;
                if (view->signal.key_modifiers & OS_EventFlag_Alt)     key |= KEYMOD_ALT;
                if (view->signal.key_modifiers & OS_EventFlag_Control) key |= KEYMOD_CONTROL;
                if (view->signal.key_modifiers & OS_EventFlag_Shift)   key |= KEYMOD_SHIFT;
                view->keymap->bindings[key].command();
            }            
        }
    }

    ui_end_build();

    ui_layout_apply(ui_root());
    draw_ui_layout(ui_root());

    d3d11_render(window_handle, draw_bucket);

    draw_end();

    arena_clear(frame_arena);

    r_d3d11_state->swap_chain->Present(1, 0);
}
