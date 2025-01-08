global Minibuffer *g_minibuffer;

internal Minibuffer *make_minibuffer() {
    Minibuffer *minibuffer = (Minibuffer *)calloc(1, sizeof(Minibuffer));
    minibuffer->active = true;

    Application *app = get_application();
    UI_View *ui_view = (UI_View *)minibuffer;
    DLLPushBack(app->ui_view_list.first, app->ui_view_list.last, ui_view, view_next, view_prev);
    app->ui_view_list.count++;
    return minibuffer;
}
