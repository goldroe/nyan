#ifndef UI_CORE_H
#define UI_CORE_H

typedef u64 UI_Key;

enum UI_Size_Kind {
    UI_SizeKind_Nil,
    UI_SizeKind_Pixels,
    UI_SizeKind_ParentPct,
    UI_SizeKind_ChildrenSum,
    UI_SizeKind_TextContent,
};

struct UI_Size {
    UI_Size_Kind kind = UI_SizeKind_Nil;
    f32 value = 0.0f;
    f32 strictness = 1.0f;
};

struct UI_Scroll_Pt {
    s64 idx;
    f32 off;
};

struct UI_Scroll_Pos {
    UI_Scroll_Pt x;
    UI_Scroll_Pt y;
};

enum UI_Text_Align {
    UI_TextAlign_Center,
    UI_TextAlign_Left,
    UI_TextAlign_Right
};

enum UI_Box_Flags {
    UI_BoxFlag_Nil                = 0,
    UI_BoxFlag_Clickable          = (1<<0),
    UI_BoxFlag_KeyboardClickable  = (1<<1),
    UI_BoxFlag_Scroll             = (1<<2),
    UI_BoxFlag_FloatingX          = (1<<3),
    UI_BoxFlag_FloatingY          = (1<<4),
    UI_BoxFlag_FixedWidth         = (1<<5),
    UI_BoxFlag_FixedHeight        = (1<<6),
    UI_BoxFlag_OverflowX          = (1<<7),
    UI_BoxFlag_OverflowY          = (1<<8),
    UI_BoxFlag_ClickToFocus       = (1<<9),
    UI_BoxFlag_DrawBackground     = (1<<10),
    UI_BoxFlag_DrawBorder         = (1<<11),
    UI_BoxFlag_DrawText           = (1<<12),
    UI_BoxFlag_DrawHotEffects     = (1<<13),
    UI_BoxFlag_DrawActiveEffects  = (1<<14),
    UI_BoxFlag_Disabled           = (1<<15),
    UI_BoxFlag_DefaultFocus       = (1<<16),
    UI_BoxFlag_Tooltip            = (1<<17),
};
EnumDefineFlagOperators(UI_Box_Flags);

struct UI_Box;
#define UI_BOX_CUSTOM_DRAW_PROC(name) void name(UI_Box *box, void *user_data)
typedef UI_BOX_CUSTOM_DRAW_PROC(UI_Box_Draw_Proc);

struct UI_Box {
    UI_Box *hash_prev = nullptr;
    UI_Box *hash_next = nullptr;

    UI_Box *parent = nullptr;
    UI_Box *prev = nullptr;
    UI_Box *next = nullptr;
    UI_Box *first = nullptr;
    UI_Box *last = nullptr;
    int child_count = 0;

    UI_Key key = 0;
    UI_Box_Flags flags = UI_BoxFlag_Nil;
    int box_id = 0 ; //@Note Shows order of creation

    V2_F32 fixed_position = V2_Zero; // computed relative to parent
    V2_F32 fixed_size = V2_Zero; // computed on requested size
    Rect rect;
    UI_Size pref_size[Axis_COUNT];
    Axis2 child_layout_axis = Axis_Y;
    UI_Text_Align text_alignment = UI_TextAlign_Center;
    Font *font;
    V4_F32 background_color = V4_One;
    V4_F32 text_color = V4_Zero;
    V4_F32 border_color = V4_Zero;
    V4_F32 hover_color = V4_Zero;
    f32 border_thickness;
    String8 string = str8_zero();

    UI_Box_Draw_Proc *custom_draw_proc;
    void *draw_data;
    OS_Cursor hover_cursor;

    //@Note Persistent data
    V2_F32 view_offset;
    V2_F32 view_offset_target;
    f32 hot_t = 0.f;
    f32 active_t = 0.f;
    f32 focus_active_t = 0.f;
};

struct UI_Box_Node {
    UI_Box_Node *next;
    UI_Box *box;
};

struct UI_Font_Node                    { UI_Font_Node            *next; Font *v; };
struct UI_Parent_Node                  { UI_Parent_Node          *next; UI_Box *v; };
struct UI_FixedX_Node                  { UI_FixedX_Node          *next; f32 v; };
struct UI_FixedY_Node                  { UI_FixedY_Node          *next; f32 v; };
struct UI_FixedWidth_Node              { UI_FixedWidth_Node      *next; f32 v; };
struct UI_FixedHeight_Node             { UI_FixedHeight_Node     *next; f32 v; };
struct UI_PrefWidth_Node               { UI_PrefWidth_Node       *next; UI_Size v; };
struct UI_PrefHeight_Node              { UI_PrefHeight_Node      *next; UI_Size v; };
struct UI_ChildLayoutAxis_Node         { UI_ChildLayoutAxis_Node *next; Axis2 v; };
struct UI_TextAlignment_Node           { UI_TextAlignment_Node   *next; UI_Text_Align v; };
struct UI_BackgroundColor_Node         { UI_BackgroundColor_Node *next; V4_F32 v; };
struct UI_BorderColor_Node             { UI_BorderColor_Node     *next; V4_F32 v; };
struct UI_TextColor_Node               { UI_TextColor_Node       *next; V4_F32 v; };
struct UI_HoverColor_Node              { UI_HoverColor_Node      *next; V4_F32 v; };
struct UI_BorderThickness_Node         { UI_BorderThickness_Node *next; f32 v; };
struct UI_Cursor_Node                  { UI_Cursor_Node          *next; OS_Cursor v; };
struct UI_BoxFlags_Node                { UI_BoxFlags_Node        *next; UI_Box_Flags v; };
struct UI_ClipBox_Node                 { UI_ClipBox_Node         *next; Rect v; };

#define UI_StackDecls \
    struct {                                                            \
    struct { UI_Font_Node            *top; UI_Font_Node            *first_free; b32 auto_pop; } font_stack; \
    struct { UI_Parent_Node          *top; UI_Parent_Node          *first_free; b32 auto_pop; } parent_stack; \
    struct { UI_FixedX_Node          *top; UI_FixedX_Node          *first_free; b32 auto_pop; } fixed_x_stack; \
    struct { UI_FixedY_Node          *top; UI_FixedY_Node          *first_free; b32 auto_pop; } fixed_y_stack; \
    struct { UI_FixedWidth_Node      *top; UI_FixedWidth_Node      *first_free; b32 auto_pop; } fixed_width_stack; \
    struct { UI_FixedHeight_Node     *top; UI_FixedHeight_Node     *first_free; b32 auto_pop; } fixed_height_stack; \
    struct { UI_PrefWidth_Node       *top; UI_PrefWidth_Node       *first_free; b32 auto_pop; } pref_width_stack; \
    struct { UI_PrefHeight_Node      *top; UI_PrefHeight_Node      *first_free; b32 auto_pop; } pref_height_stack; \
    struct { UI_ChildLayoutAxis_Node *top; UI_ChildLayoutAxis_Node *first_free; b32 auto_pop; } child_layout_axis_stack; \
    struct { UI_TextAlignment_Node   *top; UI_TextAlignment_Node   *first_free; b32 auto_pop; } text_alignment_stack; \
    struct { UI_BackgroundColor_Node *top; UI_BackgroundColor_Node *first_free; b32 auto_pop; } background_color_stack; \
    struct { UI_BorderColor_Node     *top; UI_BorderColor_Node     *first_free; b32 auto_pop; } border_color_stack; \
    struct { UI_TextColor_Node       *top; UI_TextColor_Node       *first_free; b32 auto_pop; } text_color_stack; \
    struct { UI_HoverColor_Node      *top; UI_HoverColor_Node      *first_free; b32 auto_pop; } hover_color_stack; \
    struct { UI_BorderThickness_Node *top; UI_BorderThickness_Node *first_free; b32 auto_pop; } border_thickness_stack; \
    struct { UI_Cursor_Node          *top; UI_Cursor_Node          *first_free; b32 auto_pop; } hover_cursor_stack; \
    struct { UI_BoxFlags_Node        *top; UI_BoxFlags_Node        *first_free; b32 auto_pop; } box_flags_stack; \
    struct { UI_ClipBox_Node         *top; UI_ClipBox_Node         *first_free; b32 auto_pop; } clip_box_stack; \
}

#define UI_StackPush(state,upper,lower,type,value)                  \
    UI_##upper##_Node *node = state->lower##_stack.first_free;      \
    if (node != NULL) {                                             \
        SLLStackPop(state->lower##_stack.first_free);               \
    } else {                                                        \
        node = push_array(ui_build_arena(), UI_##upper##_Node, 1);   \
    }                                                               \
    node->v = value;                                                \
    SLLStackPush(state->lower##_stack.top, node);                   \
    state->lower##_stack.auto_pop = false;                          \

#define UI_StackPop(state,upper,lower)                          \
    UI_##upper##_Node *node = state->lower##_stack.top;         \
    if (node != NULL) {                                         \
        SLLStackPop(state->lower##_stack.top);                  \
        SLLStackPush(state->lower##_stack.first_free, node);    \
        state->lower##_stack.auto_pop = false;                  \
    }                                                           \
    return node->v;                                             \

#define UI_StackSetNext(state,upper,lower,type,value)               \
    UI_##upper##_Node *node = state->lower##_stack.first_free;      \
    if (node != NULL) {                                            \
        SLLStackPop(state->lower##_stack.first_free);               \
    } else {                                                        \
        node = push_array(ui_build_arena(), UI_##upper##_Node, 1);   \
    }                                                               \
    node->v = value;                                                \
    SLLStackPush(state->lower##_stack.top, node);                   \
    state->lower##_stack.auto_pop = true;                           \

enum UI_Event_Kind {
    UI_EventKind_Error,
    UI_EventKind_MousePress,
    UI_EventKind_MouseRelease,
    UI_EventKind_Press,
    UI_EventKind_Release,
    UI_EventKind_Scroll,
    UI_EventKind_Text,
    UI_EventKind_COUNT,
};

struct UI_Event {
    UI_Event *prev;
    UI_Event *next;
    UI_Event_Kind kind;
    OS_Key key;
    OS_Event_Flags key_modifiers;
    V2_S32 pos;
    V2_S32 delta;
    String8 text;
};

struct UI_Event_List {
    UI_Event *first;
    UI_Event *last;
    int count;
};

struct UI_Hash_Bucket {
    UI_Box *first;
    UI_Box *last;
};

struct UI_State {
    Arena *arena;
    Arena *build_arenas[2];
    u64 build_index = 0;

    int build_counter = 0;
    Auto_Array<UI_Box> last_build_collection;
    UI_Box *root = nullptr;

    UI_Key focus_active_box_key = 0;
    UI_Key active_box_key = 0;
    UI_Key hot_box_key = 0;

    V2_F32 mouse;
    V2_S32 mouse_drag_start;
    b32 mouse_dragging;

    b32 keyboard_captured;
    b32 mouse_captured;

    u64 box_table_size;
    UI_Hash_Bucket *box_table;

    UI_Event_List events;
    
    f32 animation_dt;

    UI_StackDecls;
};

enum UI_Signal_Flags {
    UI_SignalFlag_Clicked  = (1<<0),
    UI_SignalFlag_Pressed  = (1<<1),
    UI_SignalFlag_Released = (1<<2),
    UI_SignalFlag_Hover    = (1<<3),
    UI_SignalFlag_Scroll   = (1<<4),
    UI_SignalFlag_Dragging = (1<<5),
};
EnumDefineFlagOperators(UI_Signal_Flags);

struct UI_Signal {
    UI_Signal_Flags flags;
    UI_Box *box;
    OS_Key key;
    OS_Event_Flags key_modifiers;
    String8 text;
    V2_S32 scroll;
};

struct UI_Txt_Buffer {
    u8 data[1024];
    u64 count;
    u64 pos;
};

enum UI_Icon_Kind {
    UI_IconKind_Warning,
    UI_IconKind_Cancel,
    UI_IconKind_CheckEmpty,
    UI_IconKind_Check,
    UI_IconKind_ArrowUp,
    UI_IconKind_ArrowDown,
    UI_IconKind_ArrowLeft,
    UI_IconKind_ArrowRight,
    UI_IconKind_ZoomPlus,
    UI_IconKind_ZoomMinus,
    UI_IconKind_Folder,
    UI_IconKind_Document,
    UI_IconKind_Trash,
    UI_IconKind_COUNT
};

internal V2_F32 ui_drag_delta();

#define ui_clicked(sig)      ((sig).flags & UI_SignalFlag_Clicked)
#define ui_hover(sig)        ((sig).flags & UI_SignalFlag_Hover)
#define ui_pressed(sig)      ((sig).flags & UI_SignalFlag_Pressed)
#define ui_scroll(sig)       ((sig).flags & UI_SignalFlag_Scroll)
#define ui_dragging(sig)     ((sig).flags & UI_SignalFlag_Dragging)

internal UI_Box *ui_top_parent();

internal void ui_set_focus_active_key(UI_Key key);

internal void ui_set_next_font(Font *v);
internal void ui_set_next_parent(UI_Box *v);
internal void ui_set_next_fixed_x(f32 v);
internal void ui_set_next_fixed_y(f32 v);
internal void ui_set_next_fixed_width(f32 v);
internal void ui_set_next_fixed_height(f32 v);
internal void ui_set_next_pref_width(UI_Size v);
internal void ui_set_next_pref_height(UI_Size v);
internal void ui_set_next_pref_size(Axis2 axis, UI_Size size);
internal void ui_set_next_child_layout_axis(Axis2 v);
internal void ui_set_next_text_alignment(UI_Text_Align v);
internal void ui_set_next_background_color(V4_F32 v);
internal void ui_set_next_border_color(V4_F32 v);
internal void ui_set_next_text_color(V4_F32 v);
internal void ui_set_next_hover_color(V4_F32 v);
internal void ui_set_next_border_thickness(f32 v);
internal void ui_set_next_hover_cursor(OS_Cursor v);
internal void ui_set_next_box_flags(UI_Box_Flags v);

internal void ui_push_font(Font *v);
internal void ui_push_parent(UI_Box *v);
internal void ui_push_fixed_x(f32 v);
internal void ui_push_fixed_y(f32 v);
internal void ui_push_fixed_width(f32 v);
internal void ui_push_fixed_height(f32 v);
internal void ui_push_pref_width(UI_Size v);
internal void ui_push_pref_height(UI_Size v);
internal void ui_push_pref_size(Axis2 axis, UI_Size size);
internal void ui_push_child_layout_axis(Axis2 v);
internal void ui_push_text_alignment(UI_Text_Align v);
internal void ui_push_background_color(V4_F32 v);
internal void ui_push_border_color(V4_F32 v);
internal void ui_push_text_color(V4_F32 v);
internal void ui_push_hover_color(V4_F32 v);
internal void ui_push_border_thickness(f32 v);
internal void ui_push_hover_cursor(OS_Cursor v);
internal void ui_push_box_flags(UI_Box_Flags v);
internal void ui_push_clip_box(Rect v);

internal Font *ui_pop_font();
internal UI_Box *ui_pop_parent();
internal f32 ui_pop_fixed_x();
internal f32 ui_pop_fixed_y();
internal f32 ui_pop_fixed_width();
internal f32 ui_pop_fixed_height();
internal UI_Size ui_pop_pref_width();
internal UI_Size ui_pop_pref_height();
internal UI_Size ui_pop_pref_size(Axis2 axis, UI_Size size);
internal Axis2 ui_pop_child_layout_axis();
internal UI_Text_Align ui_pop_text_alignment();
internal V4_F32 ui_pop_background_color();
internal V4_F32 ui_pop_border_color();
internal V4_F32 ui_pop_text_color();
internal V4_F32 ui_pop_hover_color();
internal f32 ui_pop_border_thickness();
internal OS_Cursor ui_pop_hover_cursor();
internal UI_Box_Flags ui_pop_box_flags();
internal Rect ui_pop_clip_box();

internal Arena *ui_build_arena();

internal UI_Box *ui_make_box_from_key(UI_Box *box, UI_Key key);
internal UI_Box *ui_make_box_from_string(UI_Box_Flags flags, String8 string);
internal UI_Box *ui_box_from_key(UI_Key key);
internal UI_Signal ui_signal_from_box(UI_Box *box);

internal UI_Box *ui_root();

internal V2_F32 ui_text_position(UI_Box *box);

internal UI_Size ui_size(UI_Size_Kind kind, f32 value, f32 strictness);
#define ui_px(value, strictness)  (ui_size(UI_SizeKind_Pixels, value, strictness))
#define ui_pct(value, strictness) (ui_size(UI_SizeKind_ParentPct, value, strictness))
#define ui_children_sum(strictness) (ui_size(UI_SizeKind_ChildrenSum, 0.0f, strictness))
#define ui_text_dim(padding, strictness) (ui_size(UI_SizeKind_TextContent, padding, strictness))

#define UI_Font(font)     DeferLoop(ui_push_font(font), ui_pop_font())
#define UI_Parent(parent) DeferLoop(ui_push_parent(parent), ui_pop_parent())
#define UI_ChildLayoutAxis(axis) DeferLoop(ui_push_child_layout_axis(axis), ui_pop_child_layout_axis())
#define UI_TextAlignment(align) DeferLoop(ui_push_text_alignment(align), ui_pop_text_alignment())
#define UI_PrefWidth(pref_width) DeferLoop(ui_push_pref_width(pref_width), ui_pop_pref_width())
#define UI_PrefHeight(pref_height) DeferLoop(ui_push_pref_height(pref_height), ui_pop_pref_height())
#define UI_PrefSize(axis,pref_size) DeferLoop(ui_push_pref_size(axis,pref_size), ui_pop_pref_size(axis))
#define UI_FixedWidth(fixed_width) DeferLoop(ui_push_fixed_width(fixed_width), ui_pop_fixed_width())
#define UI_FixedHeight(fixed_height) DeferLoop(ui_push_fixed_height(fixed_height), ui_pop_fixed_height())
#define UI_FixedX(fixed_x) DeferLoop(ui_push_fixed_x(fixed_x), ui_pop_fixed_x())
#define UI_FixedY(fixed_y) DeferLoop(ui_push_fixed_y(fixed_y), ui_pop_fixed_y())
#define UI_BackgroundColor(color) DeferLoop(ui_push_background_color(color), ui_pop_background_color())
#define UI_BorderColor(color) DeferLoop(ui_push_border_color(color), ui_pop_border_color())
#define UI_TextColor(color) DeferLoop(ui_push_text_color(color), ui_pop_text_color())
#define UI_HoverColor(color) DeferLoop(ui_push_hover_color(color), ui_pop_text_color())
#define UI_ClipBox(clip_box) DeferLoop(ui_push_clip_box(clip_box), ui_pop_clip_box())

#define UI_Column DeferLoop(ui_column_begin(), ui_pop_parent())
#define UI_Row DeferLoop(ui_row_begin(), ui_pop_parent())

#endif // UI_CORE_H
