global Search_Box *g_search_box;

internal Code_View *get_active_code_view();
internal Buffer *find_text_buffer(Buffer_ID id);

internal Search_Box *get_search_box() {
    return g_search_box;
}

internal Search_Box *make_search_box() {
    Search_Box *search_box = (Search_Box *)calloc(1, sizeof(Search_Box));

    Application *app = get_application();
    UI_View *ui_view = (UI_View *)search_box;
    DLLPushBack(app->ui_view_list.first, app->ui_view_list.last, ui_view, view_next, view_prev);
    app->ui_view_list.count++;
    return search_box;
}

internal void view_set_next_match(Code_View *code_view, Search_Box *search_box) {
    Buffer *buffer = find_text_buffer(code_view->buffer);
    search_box->match_index += (search_box->dir == SeekDir_Forward) ? 1 : - 1;

    if (search_box->match_index < 0) search_box->match_index = (int)code_view->search_matches.count - 1;
    if (search_box->match_index > code_view->search_matches.count - 1) search_box->match_index = 0;
    search_box->match_index = Clamp(search_box->match_index, 0, (int)code_view->search_matches.count);

    view_set_cursor(code_view, get_cursor_from_position(buffer, code_view->search_matches[search_box->match_index].max));
}

internal int get_closest_search_match(Code_View *view) {
    int index = 0;
    s64 min_dist = INT_MAX;
    for (int i = 0; i < view->search_matches.count; i++) {
        Rng_S64 rng = view->search_matches[i];
        s64 dist = rng.min - view->cursor.pos;
        if (dist < min_dist) {
            index = i;
            min_dist = dist;
        }
    }
    return index;
}

COMMAND(search_close) {
    Code_View *code_view = get_active_code_view();
    g_search_box->active = false;
    ui_set_focus_active_key(code_view->box->key);
}

COMMAND(search_box_insert) {
    Search_Box *search_box = g_search_box;
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);
    String8 search_string = str8(search_box->prompt.data, search_box->prompt.count);
    code_view->search_matches = buffer_find_text_matches(buffer, search_string);
    g_search_box->match_index = get_closest_search_match(code_view);
}

COMMAND(search_box_backspace) {
    Search_Box *search_box = g_search_box;
    Code_View *code_view = get_active_code_view();
    Buffer *buffer = find_text_buffer(code_view->buffer);
    String8 search_string = str8(search_box->prompt.data, search_box->prompt.count);
    code_view->search_matches = buffer_find_text_matches(buffer, search_string);
    g_search_box->match_index = get_closest_search_match(code_view);
}

COMMAND(search_forward) {
    Code_View *code_view = get_active_code_view();
    g_search_box->dir = SeekDir_Forward;

    view_set_next_match(code_view, g_search_box);
}

COMMAND(search_backward) {
    Code_View *code_view = get_active_code_view();
    g_search_box->dir = SeekDir_Backward;

    view_set_next_match(code_view, g_search_box);
}
