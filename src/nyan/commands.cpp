
internal Code_View *get_active_code_view();
internal Buffer *find_text_buffer(Buffer id);
internal void set_root_pane(Pane *pane);

COMMAND(nil_command) {
}

COMMAND(kill_nyan) {
    os_post_quit_message(0);
}

COMMAND(nyan_normal_state) {
    Code_View *code_view = get_active_code_view();

    view_set_normal_state(code_view);
}

COMMAND(nyan_force_normal_state) {
    Code_View *code_view = get_active_code_view();

    view_set_normal_state(code_view);
}

COMMAND(nyan_insert_state) {
    Code_View *code_view = get_active_code_view();

    view_set_insert_state(code_view);
}

COMMAND(nyan_select_state) {
    Code_View *code_view = get_active_code_view();

    Buffer_Cursor cursor = code_view->cursor;
    view_set_select_state(code_view);
    code_view->marker = cursor;
    code_view->marker_active = true;
}

COMMAND(nyan_goto_state) {
    Code_View *code_view = get_active_code_view();

    view_set_goto_state(code_view);
}

COMMAND(nyan_space_state) {
    Code_View *code_view = get_active_code_view();

    view_set_space_state(code_view);
}

COMMAND(self_insert) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    buffer_insert_string(buffer, code_view->cursor.pos, code_view->text_input);
    Buffer_Cursor cursor = get_cursor_from_position(buffer, code_view->cursor.pos + code_view->text_input.count);
    view_set_cursor(code_view, cursor);
}

COMMAND(newline) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    buffer_insert_single(buffer, code_view->cursor.pos, '\n');
    Buffer_Cursor cursor = get_cursor_from_position(buffer, code_view->cursor.pos + 1);
    view_set_cursor(code_view, cursor);
}

COMMAND(open_line_up) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    s64 line = code_view->cursor.line;
    Buffer_Pos position = get_position_from_line(buffer, line);
    buffer_insert_single(buffer, position, '\n');
    position = get_position_from_line(buffer, line);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, position);
    view_set_cursor(code_view, new_cursor);
    view_set_insert_state(code_view);
}

COMMAND(open_line_down) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    s64 line = code_view->cursor.line;
    Buffer_Pos position = get_position_from_line(buffer, line + 1);
    buffer_insert_single(buffer, position, '\n');
    position = get_position_from_line(buffer, line + 1);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, position);
    view_set_cursor(code_view, new_cursor);
    view_set_insert_state(code_view);
}

COMMAND(insert_line) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Pos start = buffer_get_start_of_line(buffer, code_view->cursor.pos);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, start);
    view_set_cursor(code_view, new_cursor);
    view_set_insert_state(code_view);
}

COMMAND(append_line) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Pos eol = buffer_get_end_of_line(buffer, code_view->cursor.pos);
    Buffer_Cursor cursor = get_cursor_from_position(buffer, eol);
    view_set_cursor(code_view, cursor);
    view_set_insert_state(code_view);
}

COMMAND(find_char_backward) {
}

COMMAND(find_char) {
    
}

COMMAND(backward_char) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    view_move_char(code_view, -1);
    code_view->pref_col = code_view->cursor.col;
}

COMMAND(forward_char) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    view_move_char(code_view, 1);
    code_view->pref_col = code_view->cursor.col;
}

COMMAND(backward_word) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Cursor cursor = code_view->cursor;
    Buffer_Pos pos = buffer_seek_word_begin(buffer, cursor.pos - 1, SeekDir_Backward);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, pos);
    view_set_cursor(code_view, new_cursor);
    code_view->pref_col = code_view->cursor.col;

    if (code_view->marker_active && !code_view->persist_selection) {
        code_view->marker_active = false;
    }

    if (!code_view->marker_active) {
        code_view->persist_selection = false;
        code_view->marker_active = true;
        code_view->marker = cursor;
    }
}

COMMAND(forward_word) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Cursor cursor = code_view->cursor;
    Buffer_Pos pos = buffer_seek_word_end(buffer, code_view->cursor.pos + 1, SeekDir_Forward);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, pos);
    view_set_cursor(code_view, new_cursor);
    code_view->pref_col = code_view->cursor.col;

    if (code_view->marker_active && !code_view->persist_selection) {
        code_view->marker_active = false;
    }

    if (!code_view->marker_active) {
        code_view->persist_selection = false;
        code_view->marker_active = true;
        code_view->marker = cursor;
    }
}

COMMAND(prev_line) {
    Code_View *code_view = get_active_code_view();

    view_move_lines(code_view, -1);
}

COMMAND(next_line) {
    Code_View *code_view = get_active_code_view();

    view_move_lines(code_view, 1);
}

COMMAND(page_up) {
    Code_View *code_view = get_active_code_view();

    int page_lines = (int)round_f32(rect_height(code_view->box->rect) / code_view->font->glyph_height);
    view_move_lines(code_view, -page_lines);
}

COMMAND(page_down) {
    Code_View *code_view = get_active_code_view();

    int page_lines = (int)ceil_f32(rect_height(code_view->box->rect) / code_view->font->glyph_height);
    view_move_lines(code_view, page_lines);
}

COMMAND(backspace) {
    Code_View *code_view = get_active_code_view();

    if (code_view->cursor.pos > 0) {
        view_delete(code_view, SeekDir_Backward);
    }
}

COMMAND(del) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    if (code_view->cursor.pos < buffer_get_length(buffer) - 1) {
        view_delete(code_view, SeekDir_Forward);
    }
}

COMMAND(goto_file_start) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, 0);
    view_set_cursor(code_view, new_cursor);
    view_set_normal_state(code_view);
}

COMMAND(goto_file_end) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, buffer_get_length(buffer) - 1);
    view_set_cursor(code_view, new_cursor);
    view_set_normal_state(code_view);
}

COMMAND(goto_line_end) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Pos line_position = get_position_from_line(buffer, code_view->cursor.line);
    s64 length = buffer_get_line_length(buffer, code_view->cursor.line);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, line_position + length);
    view_set_cursor(code_view, new_cursor);
    view_set_normal_state(code_view);
}

COMMAND(goto_line_start) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    Buffer_Pos line_position = get_position_from_line(buffer, code_view->cursor.line);
    Buffer_Cursor new_cursor = get_cursor_from_position(buffer, line_position);
    view_set_cursor(code_view, new_cursor);
    view_set_normal_state(code_view);
}

COMMAND(file_picker_open) {
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);

    String8 path = buffer->file_path;
    file_picker_prompt(g_file_picker, path);
    g_file_picker->active = true;
}

COMMAND(file_picker_close) {
    Code_View *code_view = get_active_code_view();

    g_file_picker->active = false;
    view_set_normal_state(code_view);
    ui_set_focus_active_key(code_view->box->key);
}

COMMAND(hsplit) {
    Code_View *code_view = get_active_code_view();

    view_split_pane(code_view, Axis_Y);
}

COMMAND(vsplit) {
    Code_View *code_view = get_active_code_view();

    view_split_pane(code_view, Axis_X);
}

COMMAND(search) {
    Code_View *code_view = get_active_code_view();
    ui_set_focus_active_key(g_search_box->key);

    g_search_box->active = true;
    g_search_box->dir = SeekDir_Forward;
    g_search_box->match_index = 0;
}
