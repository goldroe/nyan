#ifndef NYAN_H
#define NYAN_H

struct UI_View_List {
    UI_View *first;
    UI_View *last;
    int count;
};

struct Code_View_List {
    Code_View *first;
    Code_View *last;
    int count;
};

struct Buffer_List {
    Buffer *first;
    Buffer *last;
    int count;
};

struct Pane_List {
    Pane *first;
    Pane *last;
    int count;
};

struct Application {
    Code_View_ID active_code_view;
    UI_Key active_view_key;

    Buffer_List buffer_list;
    UI_View_List ui_view_list;
    Code_View_List code_view_list;

    Pane *root_pane;
};

internal Application *get_application();

#endif // NYAN_H
