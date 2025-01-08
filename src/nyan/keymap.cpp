global Keymap *g_keymap_normal;
global Keymap *g_keymap_insert;
global Keymap *g_keymap_select;
global Keymap *g_keymap_goto;
global Keymap *g_keymap_space;
global Keymap *g_keymap_window;
global Keymap *g_keymap_picker;
global Keymap *g_keymap_search_box;

global Key_Bind g_nil_cmd = { "nil", nil_command };
global Key_Bind g_self_insert_cmd = { "self_insert", self_insert };
global Key_Bind g_normal_cmd = { "nyan_normal_state", nyan_normal_state };

internal Keymap *get_normal_keymap() {return g_keymap_normal;}
internal Keymap *get_insert_keymap() {return g_keymap_insert;}
internal Keymap *get_select_keymap() {return g_keymap_select;}
internal Keymap *get_goto_keymap() {return g_keymap_goto;}
internal Keymap *get_space_keymap() {return g_keymap_space;}
internal Keymap *get_window_keymap() {return g_keymap_window;}
internal Keymap *get_picker_keymap() {return g_keymap_picker;}

internal void keymap_common(Keymap *keymap) {
    for (int i = 0; i < MAX_KEYBIND_CAP; i++) {
        keymap->bindings[i] = g_nil_cmd;
    }
    keymap->bindings[KEYMOD_ALT|OS_KEY_F4] = { "kill_nyan", kill_nyan };
}

internal Keymap *make_goto_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);

    keymap->bindings[OS_KEY_ESCAPE] = { "nyan_normal_state", nyan_normal_state };
    keymap->bindings[OS_KEY_G] = { "goto_file_start", goto_file_start };
    keymap->bindings[OS_KEY_G] = { "goto_file_start", goto_file_start };
    keymap->bindings[OS_KEY_E] = { "goto_file_end",  goto_file_end };
    keymap->bindings[OS_KEY_H] = { "goto_line_start", goto_line_start };
    keymap->bindings[OS_KEY_L] = { "goto_line_end", goto_line_end };
    return keymap;
}

internal Keymap *make_insert_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);

    for (OS_Key key = OS_KEY_0; key <= OS_KEY_9; key = (OS_Key)(key + 1)) {
        keymap->bindings[key] = g_self_insert_cmd;
        keymap->bindings[KEYMOD_SHIFT|key] = g_self_insert_cmd;
    }
    for (OS_Key key = OS_KEY_A; key <= OS_KEY_Z; key = (OS_Key)(key + 1)) {
        keymap->bindings[key] = g_self_insert_cmd;
        keymap->bindings[KEYMOD_SHIFT|key] = g_self_insert_cmd;
    }
    for (OS_Key key = OS_KEY_SPACE; key <= OS_KEY_PLUS; key = (OS_Key)(key + 1)) {
        keymap->bindings[key] = g_self_insert_cmd;
        keymap->bindings[KEYMOD_SHIFT|key] = g_self_insert_cmd;
    }
    keymap->bindings[OS_KEY_TICK] = g_self_insert_cmd;
    keymap->bindings[KEYMOD_SHIFT|OS_KEY_TICK] = g_self_insert_cmd;

    keymap->bindings[OS_KEY_ENTER] = { "newline", newline };

    keymap->bindings[OS_KEY_LEFT]  = { "backward_char", backward_char };
    keymap->bindings[OS_KEY_RIGHT] = { "forward_char", forward_char };
    keymap->bindings[OS_KEY_UP]    = { "prev_line", prev_line};
    keymap->bindings[OS_KEY_DOWN]  = { "next_line", next_line};

    keymap->bindings[OS_KEY_BACKSPACE] = { "backspace", backspace };
    keymap->bindings[OS_KEY_DELETE] = { "del", del };

    keymap->bindings[OS_KEY_PAGEDOWN] = { "page_down", page_down };
    keymap->bindings[KEYMOD_CONTROL|OS_KEY_F] = { "page_down", page_down };

    keymap->bindings[OS_KEY_PAGEUP]   = { "page_up", page_up };
    keymap->bindings[KEYMOD_CONTROL|OS_KEY_B] = { "page_up", page_up};

    keymap->bindings[OS_KEY_ESCAPE] = { "nyan_normal_state", nyan_normal_state };
    return keymap;
}

internal Keymap *make_normal_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);

    keymap->bindings[OS_KEY_ESCAPE] = { "nyan_force_normal_state", nyan_force_normal_state };

    keymap->bindings[OS_KEY_I] = { "nyan_insert_state", nyan_insert_state };
    keymap->bindings[OS_KEY_G] = { "nyan_goto_state", nyan_goto_state };
    keymap->bindings[OS_KEY_V] = { "nyan_select_state", nyan_select_state };

    keymap->bindings[OS_KEY_H] = { "backward_char", backward_char };
    keymap->bindings[OS_KEY_LEFT] = { "backward_char", backward_char };
    keymap->bindings[OS_KEY_L] = { "forward_char", forward_char };
    keymap->bindings[OS_KEY_RIGHT] = { "forward_char", forward_char };
    keymap->bindings[OS_KEY_K] = { "prev_line", prev_line};
    keymap->bindings[OS_KEY_UP] = { "prev_line", prev_line};
    keymap->bindings[OS_KEY_J] = { "next_line", next_line};
    keymap->bindings[OS_KEY_DOWN] = { "next_line", next_line};

    keymap->bindings[OS_KEY_E] = { "forward_word", forward_word };
    keymap->bindings[OS_KEY_B] = { "backward_word", backward_word };

    keymap->bindings[OS_KEY_PAGEDOWN] = { "page_down", page_down };
    keymap->bindings[KEYMOD_CONTROL|OS_KEY_F] = { "page_down", page_down };
    keymap->bindings[OS_KEY_PAGEUP]   = { "page_up", page_up };
    keymap->bindings[KEYMOD_CONTROL|OS_KEY_B] = { "page_up", page_up};

    keymap->bindings[OS_KEY_SPACE] = { "nyan_space_state", nyan_space_state };

    keymap->bindings[KEYMOD_SHIFT|OS_KEY_O] = { "open_line_up", open_line_up };
    keymap->bindings[OS_KEY_O] = { "open_line_down", open_line_down };

    keymap->bindings[OS_KEY_BACKSPACE] = { "backward_char", backward_char };
    keymap->bindings[OS_KEY_D] = { "del", del };

    keymap->bindings[KEYMOD_CONTROL|OS_KEY_P] = { "vsplit", vsplit };
    keymap->bindings[KEYMOD_CONTROL|KEYMOD_SHIFT|OS_KEY_P] = { "hsplit", hsplit };

    keymap->bindings[KEYMOD_SHIFT|OS_KEY_A] = { "append_line", append_line };
    keymap->bindings[KEYMOD_SHIFT|OS_KEY_I] = { "insert_line", insert_line };

    keymap->bindings[OS_KEY_SLASH] = { "search", search };

    return keymap;
}

internal Keymap *make_select_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);

    keymap->bindings[OS_KEY_ESCAPE] = { "nyan_normal_state", nyan_normal_state };

    keymap->bindings[OS_KEY_H] = { "backward_char", backward_char };
    keymap->bindings[OS_KEY_LEFT] = { "backward_char", backward_char };
    keymap->bindings[OS_KEY_L] = { "forward_char", forward_char };
    keymap->bindings[OS_KEY_RIGHT] = { "forward_char", forward_char };
    keymap->bindings[OS_KEY_K] = { "prev_line", prev_line};
    keymap->bindings[OS_KEY_UP] = { "prev_line", prev_line};
    keymap->bindings[OS_KEY_J] = { "next_line", next_line};
    keymap->bindings[OS_KEY_DOWN] = { "next_line", next_line};

    keymap->bindings[OS_KEY_PAGEDOWN] = { "page_down", page_down };
    keymap->bindings[KEYMOD_CONTROL|OS_KEY_F] = { "page_down", page_down };
    keymap->bindings[OS_KEY_PAGEUP]   = { "page_up", page_up };
    keymap->bindings[KEYMOD_CONTROL|OS_KEY_B] = { "page_up", page_up};

    keymap->bindings[OS_KEY_DELETE] = { "del", del };
    keymap->bindings[OS_KEY_D] = { "del", del };

    return keymap;
}

internal Keymap *make_space_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);
    keymap->bindings[OS_KEY_ESCAPE] = { "nyan_normal_state", nyan_normal_state };
    keymap->bindings[OS_KEY_F] = { "file_picker_open", file_picker_open };
    return keymap;
}

internal Keymap *make_picker_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);
    keymap->bindings[OS_KEY_ESCAPE] = { "file_picker_close", file_picker_close };
    return keymap;
}

internal Keymap *make_search_box_keymap() {
    Keymap *keymap = new Keymap();
    keymap_common(keymap);

    Key_Bind search_insert_cmd = { "search_box_insert", search_box_insert };
    for (OS_Key key = OS_KEY_0; key <= OS_KEY_9; key = (OS_Key)(key + 1)) {
        keymap->bindings[key] = search_insert_cmd;
        keymap->bindings[KEYMOD_SHIFT|key] = search_insert_cmd;
    }
    for (OS_Key key = OS_KEY_A; key <= OS_KEY_Z; key = (OS_Key)(key + 1)) {
        keymap->bindings[key] = search_insert_cmd;
        keymap->bindings[KEYMOD_SHIFT|key] = search_insert_cmd;
    }
    for (OS_Key key = OS_KEY_SPACE; key <= OS_KEY_PLUS; key = (OS_Key)(key + 1)) {
        keymap->bindings[key] = search_insert_cmd;
        keymap->bindings[KEYMOD_SHIFT|key] = search_insert_cmd;
    }
    keymap->bindings[OS_KEY_TICK] = search_insert_cmd;
    keymap->bindings[KEYMOD_SHIFT|OS_KEY_TICK] = search_insert_cmd;

    keymap->bindings[OS_KEY_BACKSPACE] = { "search_box_backspace", search_box_backspace };

    keymap->bindings[OS_KEY_UP]    = { "search_backward", search_backward };
    keymap->bindings[OS_KEY_DOWN]  = { "search_forward", search_forward };
    keymap->bindings[OS_KEY_ENTER] = { "search_forward", search_forward };

    keymap->bindings[OS_KEY_ESCAPE] = { "searc_close", search_close };

    return keymap;
}
