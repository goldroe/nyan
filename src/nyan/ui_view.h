#ifndef UI_VIEW_H
#define UI_VIEW_H

typedef u64 UI_View_ID;

struct UI_View {
    UI_Key key;
    UI_View *view_next;
    UI_View *view_prev;

    b32 active;
    UI_Signal signal;
    String8 text_input;
    Keymap *keymap;
};

#endif // UI_VIEW_H
