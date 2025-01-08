#ifndef KEYMAP_H
#define KEYMAP_H

#define COMMAND(name) void name()
typedef COMMAND(Command);

struct Key_Bind {
    char *name;
    Command *command;
};

#define KEYMOD_ALT      (1<<18)
#define KEYMOD_CONTROL  (1<<17)
#define KEYMOD_SHIFT    (1<<16)
#define MAX_KEYBIND_CAP (1 << (16 + 3))

struct Keymap {
    Key_Bind bindings[MAX_KEYBIND_CAP];
};

internal Keymap *get_normal_keymap();
internal Keymap *get_insert_keymap();
internal Keymap *get_select_keymap();
internal Keymap *get_goto_keymap();
internal Keymap *get_space_keymap();
internal Keymap *get_window_keymap();
internal Keymap *get_picker_keymap();

#endif // KEYMAP_H
