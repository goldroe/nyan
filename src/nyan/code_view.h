#ifndef CODE_VIEW_H
#define CODE_VIEW_H

typedef u64 Code_View_ID;

struct Code_View;

enum Modal_State {
    ModalState_Normal,
    ModalState_Insert,
    ModalState_Select,
    ModalState_COUNT
};

struct Pane {
    Pane *parent;
    Pane *tl;
    Pane *br;

    Axis2 axis;
    Code_View *view;
    V2_F32 pct_dim;

    V2_F32 fixed_pos;
    V2_F32 fixed_dim;
};

struct Code_View : UI_View {
    Code_View_ID id;

    Code_View *next;
    Code_View *prev;

    Buffer_ID buffer;
    Buffer_Cursor cursor;
    s64 pref_col;

    Buffer_Cursor marker;
    b32 marker_active;
    b32 persist_selection;
    
    Modal_State modal_state;

    Pane *pane;
    UI_Box *box;
    UI_Scroll_Pt scroll_pt;
    Font *font;

    Auto_Array<Rng_S64> search_matches;

    //@Note Buffer data
    Arena *buffer_arena;
    String8 buffer_string;
    Auto_Array<u64> line_bounds;
};

#endif // CODE_VIEW_H
