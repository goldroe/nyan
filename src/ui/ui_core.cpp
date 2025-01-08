global UI_State *ui_state;

global char *ui_g_icon_kind_strings[] = {
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z"
};
internal String8 ui_string_from_icon_kind(UI_Icon_Kind kind, const char *end) {
    char *icon_string = ui_g_icon_kind_strings[kind];
    String8 result = str8_pushf(ui_build_arena(), "%s%s", icon_string, end);
    return result;
}

internal UI_Scroll_Pt ui_scroll_pt(s64 idx, f32 off) {
    UI_Scroll_Pt result = {idx, off};
    return result;
}

internal UI_Box *ui_root() {
    return ui_state->root;
}

internal f32 ui_animation_dt() {
    return ui_state->animation_dt;
}

internal bool ui_key_match(UI_Key a, UI_Key b) {
    return a == b;
}

internal void ui_set_state(UI_State *state) {
    ui_state = state;
}

internal void ui_set_mouse_captured() {
    ui_state->mouse_captured = true;
}

internal void ui_set_keyboard_captured() {
    ui_state->keyboard_captured = true;
}

internal UI_State *ui_state_new() {
    Arena *arena = make_arena(get_virtual_allocator());
    UI_State *ui = push_array(arena, UI_State, 1);
    ui->arena = arena;
    ui->build_arenas[0] = arena_alloc(get_virtual_allocator(), MB(1));
    ui->build_arenas[1] = arena_alloc(get_virtual_allocator(), MB(1));
    ui->box_table_size = 4096;
    ui->box_table = push_array(arena, UI_Hash_Bucket, ui->box_table_size);
    return ui;
}

internal UI_Event *ui_push_event(UI_Event_Kind kind) {
    UI_Event *result = push_array(ui_build_arena(), UI_Event, 1);
    result->kind = kind;
    result->next = nullptr;
    result->prev = nullptr;

    DLLPushBack(ui_state->events.first, ui_state->events.last, result, next, prev);
    ui_state->events.count += 1;
    return result;
}

internal void ui_pop_event(UI_Event *event) {
    DLLRemove(ui_state->events.first, ui_state->events.last, event, next, prev);
    ui_state->events.count -= 1;
}

internal V2_F32 ui_mouse() {return ui_state->mouse;}
internal V2_F32 ui_drag_delta() {return ui_mouse() - v2_f32_from_v2_s32(ui_state->mouse_drag_start);}

internal bool ui_mouse_press() {
    bool result = false;
    for (UI_Event *event = ui_state->events.first; event; event = event->next) {
        if (event->kind == UI_EventKind_MousePress) {
            result = true;
            break;
        }
    }
    return result;
}

internal bool ui_mouse_release() {
    bool result = false;
    for (UI_Event *event = ui_state->events.first; event; event = event->next) {
        if (event->kind == UI_EventKind_MouseRelease) {
            result = true;
            break;
        }
    }
    return result;
}

internal bool ui_key_press(OS_Key key) {
    bool result = false;
    for (UI_Event *event = ui_state->events.first; event; event = event->next) {
        if (event->kind == UI_EventKind_Press) {
            if (event->key == key) {
                result = true;
                break;
            }
        }
    }
    return result;
}

internal bool ui_key_release(OS_Key key) {
    bool result = false;
    for (UI_Event *event = ui_state->events.first; event; event = event->next) {
        if (event->kind == UI_EventKind_Release) {
            if (event->key == key) {
                ui_pop_event(event);
                result = true;
                ui_set_keyboard_captured();
                break;
            }
        }
    }
    return result;
}

internal V2_F32 measure_string_size(String8 string, Font *font) {
    V2_F32 result = V2_Zero;
    for (u64 i = 0; i < string.count; i++) {
        u8 c = string.data[i];
        if (c == '\n') {
            result.x = 0.f;
            result.y += font->glyph_height;
            continue;
        }
        Glyph g = font->glyphs[c];
        result.x += g.ax;
    }
    return result;
}

internal f32 get_string_width(String8 text, Font *font) {
    f32 result = 0.0f;
    for (u64 i = 0; i < text.count; i++) {
        u8 c = text.data[i];
        Glyph g = font->glyphs[c];
        result += g.ax;
    }
    return result;
}

internal void ui_set_custom_draw(UI_Box *box, UI_Box_Draw_Proc *draw_proc, void *user_data) {
    box->custom_draw_proc = draw_proc;
    box->draw_data = user_data;
}

internal void ui_box_set_string(UI_Box *box, String8 string) {
    String8 display_string = string;
    u64 hash_start = str8_find_substr(string, str8_lit("###"));
    if (hash_start < string.count) {
        display_string.count = hash_start;
    }
    box->string = str8_copy(ui_build_arena(), display_string);
}

internal UI_Size ui_size(UI_Size_Kind kind, f32 value, f32 strictness) {
    UI_Size size = { kind, value, strictness };
    return size;
}

internal UI_Box *ui_box_from_key(UI_Key key) {
    UI_Hash_Bucket *hash_bucket = &ui_state->box_table[key % ui_state->box_table_size];
    for (UI_Box *box = hash_bucket->first; box; box = box->hash_next) {
        if (box->key == key) {
            return box;
        }
    }
    return nullptr;
}

internal void ui_push_box(UI_Box *box, UI_Box *parent) {
    box->parent = parent;
    if (parent) {
        DLLPushBack(parent->first, parent->last, box, next, prev);
        parent->child_count += 1;
    }
}

internal Arena *ui_build_arena() {
    Arena *result = ui_state->build_arenas[ui_state->build_index%ArrayCount(ui_state->build_arenas)];
    return result;
}

internal UI_Box *ui_make_box_from_key(UI_Box_Flags flags, UI_Key key) {
    UI_Box *parent = ui_top_parent();
    UI_Box *box = ui_box_from_key(key);
    bool box_first_frame = box == nullptr;
    bool box_is_transient = key == 0;

    if (ui_state->box_flags_stack.top) {
        flags |= ui_state->box_flags_stack.top->v;
    }
    bool box_is_disabled = flags & UI_BoxFlag_Disabled;

    if (box_first_frame) {
        box = push_array(ui_state->arena, UI_Box, 1);
        *box = UI_Box();

        if (!box_is_transient) {
            UI_Hash_Bucket *hash_bucket = &ui_state->box_table[key % ui_state->box_table_size];
            DLLPushBack(hash_bucket->first, hash_bucket->last, box, hash_next, hash_prev);
        }

        //@Note Set default focus on newly created box
        if (!box_is_disabled && (flags & UI_BoxFlag_DefaultFocus)) {
            ui_set_focus_active_key(key);
        }
    }

    //@Note Set active focus on re-enable
    bool re_enable = !box_is_disabled && (box->flags & UI_BoxFlag_Disabled);
    if (re_enable) {
        if (flags & UI_BoxFlag_DefaultFocus) {
            ui_set_focus_active_key(key);
        }
    }

    box->box_id = ui_state->build_counter;
    ui_state->build_counter += 1;

    //@Note Reset per-frame builder options
    box->first = box->last = box->prev = box->next = box->parent = nullptr;
    box->child_count = 0;
    box->key = key;
    box->flags = flags;
    box->pref_size[0] = {};
    box->pref_size[1] = {};
    box->fixed_size = V2_Zero;
    box->custom_draw_proc = NULL;
    box->draw_data = NULL;

    if (parent == nullptr) {
        return box;
    }

    ui_push_box(box, parent);

  
    if (ui_state->fixed_x_stack.top) {
        box->flags |= UI_BoxFlag_FloatingX;
        box->fixed_position[Axis_X] = ui_state->fixed_x_stack.top->v;
    }
    if (ui_state->fixed_y_stack.top) {
        box->flags |= UI_BoxFlag_FloatingY;
        box->fixed_position[Axis_Y] = ui_state->fixed_y_stack.top->v;
    }

    if (ui_state->fixed_width_stack.top) {
        box->flags |= UI_BoxFlag_FixedWidth;
        box->fixed_size[Axis_X] = ui_state->fixed_width_stack.top->v;
    } else {
        box->pref_size[Axis_X] = ui_state->pref_width_stack.top->v;
    }
    if (ui_state->fixed_height_stack.top) {
        box->flags |= UI_BoxFlag_FixedHeight;
        box->fixed_size[Axis_Y] = ui_state->fixed_height_stack.top->v;
    }
    else {
        box->pref_size[Axis_Y] = ui_state->pref_height_stack.top->v;
    }

    if (ui_state->border_thickness_stack.top) {
        box->border_thickness = ui_state->border_thickness_stack.top->v;
    }

    box->font = ui_state->font_stack.top->v;
    box->text_alignment = ui_state->text_alignment_stack.top->v;
    box->child_layout_axis = ui_state->child_layout_axis_stack.top->v;
    box->background_color = ui_state->background_color_stack.top->v;
    box->border_color = ui_state->border_color_stack.top->v;
    box->text_color = ui_state->text_color_stack.top->v;
    box->hover_color = ui_state->hover_color_stack.top->v;
    box->hover_cursor = ui_state->hover_cursor_stack.top->v;

    f32 animation_dt = ui_state->animation_dt;

    f32 fast_rate = animation_dt / 50.f;
    f32 hot_rate =  fast_rate;
    f32 active_rate =  fast_rate;
    box->view_offset.x += fast_rate * (box->view_offset_target.x - box->view_offset.x);
    box->view_offset.y += fast_rate * (box->view_offset_target.y - box->view_offset.y);

    bool is_hot = ui_state->hot_box_key == box->key;
    bool is_active = ui_state->active_box_key == box->key;
    bool is_focus_active = ui_state->focus_active_box_key == box->key;

    box->hot_t          += hot_rate    * ((f32)is_hot - box->hot_t);
    box->active_t       += active_rate * ((f32)is_active - box->active_t);
    box->focus_active_t += active_rate * ((f32)is_focus_active - box->focus_active_t);
    
    //@Note Auto pop UI stacks
    {
        if (ui_state->font_stack.auto_pop) { ui_pop_font(); }
        if (ui_state->parent_stack.auto_pop) { ui_pop_parent(); }
        if (ui_state->fixed_x_stack.auto_pop) { ui_pop_fixed_x(); }
        if (ui_state->fixed_y_stack.auto_pop) { ui_pop_fixed_y(); }
        if (ui_state->fixed_width_stack.auto_pop) { ui_pop_fixed_width(); }
        if (ui_state->fixed_height_stack.auto_pop) { ui_pop_fixed_height(); }
        if (ui_state->pref_width_stack.auto_pop) { ui_pop_pref_width(); }
        if (ui_state->pref_height_stack.auto_pop) { ui_pop_pref_height(); }
        if (ui_state->child_layout_axis_stack.auto_pop) { ui_pop_child_layout_axis(); }
        if (ui_state->text_alignment_stack.auto_pop) { ui_pop_text_alignment(); }
        if (ui_state->background_color_stack.auto_pop) { ui_pop_background_color(); }
        if (ui_state->border_color_stack.auto_pop) { ui_pop_border_color(); }
        if (ui_state->text_color_stack.auto_pop) { ui_pop_text_color(); }
        if (ui_state->hover_color_stack.auto_pop) { ui_pop_hover_color(); }
        if (ui_state->border_thickness_stack.auto_pop) { ui_pop_border_thickness(); }
        if (ui_state->hover_cursor_stack.auto_pop) { ui_pop_hover_cursor(); }
        if (ui_state->box_flags_stack.auto_pop) { ui_pop_box_flags(); }
    }
    return box;
}

//@Note Key-ing functions

//@Note djb2 hash algorithm
internal u64 ui_hash_from_string(u64 seed, String8 string) {
    u64 result = seed;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}

internal String8 hash_part_from_string(String8 string) {
    String8 hash_part = string;
    u64 hash_start = str8_find_substr(string, str8_lit("###"));
    if (hash_start < string.count) {
        hash_part = str8_jump(string, hash_start);
    }
    return hash_part;     
}

internal UI_Key ui_key_from_string(UI_Key seed_key, String8 string) {
    String8 hash_part = hash_part_from_string(string);
    UI_Key key = (UI_Key)ui_hash_from_string((u64)seed_key, hash_part);
    return key;
}

internal UI_Key ui_seed_key(UI_Box *parent) {
    UI_Key seed_key = 0;
    UI_Box *inherited_seed_parent = parent;
    while (inherited_seed_parent) {
        if (inherited_seed_parent->key != 0) {
            seed_key = inherited_seed_parent->key;
            break;
        }
        inherited_seed_parent = inherited_seed_parent->parent;
    }
    return seed_key;
}

internal UI_Box *ui_make_box_from_string(UI_Box_Flags flags, String8 string) {
    UI_Box *box = nullptr;
    UI_Box *parent = ui_top_parent();

    UI_Key seed_key = ui_seed_key(parent);
    UI_Key key = ui_key_from_string(seed_key, string);
    box = ui_make_box_from_key(flags, key);

    if (box->flags & UI_BoxFlag_DrawText) {
        ui_box_set_string(box, string);
    }

    return box;
}

internal UI_Box *ui_make_box_from_stringf(UI_Box_Flags flags, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    UI_Box *box = ui_make_box_from_string(flags, string);
    return box;
}

internal void ui_set_focus_active_key(UI_Key key) {
    ui_state->focus_active_box_key = key;
}

internal void ui_set_active_key(UI_Key key) {
    ui_state->active_box_key = key;
}

internal void ui_set_hot_key(UI_Key key) {
    ui_state->hot_box_key = key;
}

internal bool ui_key_focus_active_box_match(UI_Key key) {
    return ui_state->focus_active_box_key == key;
}

internal bool ui_key_active_box_match(UI_Key key) {
    return ui_state->active_box_key == key;
}

internal bool ui_key_hot_box_match(UI_Key key) {
    return ui_state->hot_box_key == key;
}

internal UI_Signal ui_signal_from_box(UI_Box *box) {
    UI_Signal signal = {};
    if (box == NULL || (box->flags & UI_BoxFlag_Disabled)) return signal;

    signal.box = box;
    signal.key_modifiers = os_event_flags();

    bool box_is_transient = (box->key == 0);
    if (box_is_transient) return signal;

    UI_Box *box_last_frame = ui_box_from_key(box->key);
    bool box_first_frame = box_last_frame == nullptr;

    bool event_in_bounds = rect_contains(box->rect, ui_mouse());

    bool do_later = false;
    for (int i = 0; i < ui_state->last_build_collection.count; i++) {
        UI_Box *collect_box = &ui_state->last_build_collection.data[i];
        if (collect_box->flags & UI_BoxFlag_Disabled) continue;

        if (collect_box->box_id > box->box_id && rect_contains(collect_box->rect, ui_mouse())) {
            do_later = true;
            break;
        }
    }

    //@Note Consume UI events for box
    for (UI_Event *event = ui_state->events.first, *next = nullptr; event; event = next) {
        next = event->next;
        bool taken = false;

        if (event->kind == UI_EventKind_MouseRelease) {
            ui_state->mouse_dragging = false;
            ui_set_mouse_captured();
        }

        if (box->flags & UI_BoxFlag_Clickable &&
            event->kind == UI_EventKind_MousePress &&
            event_in_bounds && !do_later) {
            signal.flags |= UI_SignalFlag_Clicked;
            ui_state->active_box_key = box->key;

            if (box->flags & UI_BoxFlag_ClickToFocus) {
                ui_set_focus_active_key(box->key);
            }

            ui_state->mouse_dragging = true;
            ui_state->mouse_drag_start = event->pos;

            ui_set_mouse_captured();
            taken = true;
        }

        if (box->flags & UI_BoxFlag_Clickable
            && event->kind == UI_EventKind_MouseRelease) {
            if (event_in_bounds && !do_later) {
                signal.flags |= UI_SignalFlag_Released;
                ui_set_mouse_captured();
            }
            ui_state->active_box_key = 0;
        }

        if (box->flags & UI_BoxFlag_Scroll &&
            event->kind == UI_EventKind_Scroll &&
            event_in_bounds && !do_later) {
            signal.flags |= UI_SignalFlag_Scroll;
            signal.scroll.x = event->delta.x;
            signal.scroll.y = event->delta.y;
            ui_set_mouse_captured();
            taken = true;
        }

        if (box->flags & UI_BoxFlag_KeyboardClickable &&
            event->kind == UI_EventKind_Press &&
            ui_key_focus_active_box_match(box->key)) {
            signal.flags |= UI_SignalFlag_Pressed;
            signal.key = event->key;
            ui_set_keyboard_captured();
        }

        if (box->flags & UI_BoxFlag_Clickable &&
            event->kind == UI_EventKind_Release &&
            ui_key_active_box_match(box->key)) {
            signal.flags |= UI_SignalFlag_Released;
            ui_set_mouse_captured();
        }

        if (box->flags & UI_BoxFlag_KeyboardClickable &&
            event->kind == UI_EventKind_Text &&
            ui_key_focus_active_box_match(box->key)) {
            signal.text = str8_concat(ui_build_arena(), signal.text, event->text);
            ui_set_keyboard_captured();
        }

        if (taken) {
            ui_pop_event(event);
        }
    }

    if (box->flags & UI_BoxFlag_Clickable &&
        event_in_bounds && !do_later) {
        signal.flags |= UI_SignalFlag_Hover;
        ui_state->hot_box_key = box->key;
        ui_set_mouse_captured();
    }

    //@Note Mouse dragging
    if (ui_state->mouse_dragging &&
        ui_key_active_box_match(box->key)) {
        signal.flags |= UI_SignalFlag_Dragging;
        ui_set_mouse_captured();
    }

    return signal;
}

internal void ui_layout_calc_fixed(UI_Box *root, Axis2 axis) {
    if (!(root->flags & (UI_BoxFlag_FixedWidth << axis))) {
        f32 size = 0.0f;
        switch (root->pref_size[axis].kind) {
        default:
            break;
        case UI_SizeKind_Pixels:
            size = root->pref_size[axis].value;
            break;
        case UI_SizeKind_TextContent:
            f32 padding = root->pref_size[axis].value;
            if (axis == Axis_X) {
                size = get_string_width(root->string, root->font);
            } else {
                size = root->font->glyph_height;
            }
            size += 2.0f * padding;
            break;
        }
        root->fixed_size[axis] += size;
    }

    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_calc_fixed(child, axis);
    }
}

internal void ui_layout_calc_upwards_dependent(UI_Box *root, Axis2 axis) {
    if (root->pref_size[axis].kind == UI_SizeKind_ParentPct) {
        for (UI_Box *parent = root->parent; parent != nullptr; parent = parent->parent) {
            bool found = false;
            if (parent->flags & (UI_BoxFlag_FixedWidth<<axis)) found = true;
            switch (parent->pref_size[axis].kind) {
            case UI_SizeKind_Pixels:
            case UI_SizeKind_TextContent:
                found = true;
                break;
            }

            if (found) {
                root->fixed_size[axis] = root->pref_size[axis].value * parent->fixed_size[axis];
                break; 
            }
        }
    }

    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_calc_upwards_dependent(child, axis);
    }
}

internal void ui_layout_calc_downwards_dependent(UI_Box *root, Axis2 axis) {
    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_calc_downwards_dependent(child, axis);
    }

    if (root->pref_size[axis].kind == UI_SizeKind_ChildrenSum) {
        if (root->child_layout_axis == axis) {
            //@Note Add sizes of children if on the layout axis
            for (UI_Box *child = root->first; child != nullptr; child = child->next) {
                root->fixed_size[axis] += child->fixed_size[axis];
            }
        } else {
            //@Note Get max children size if not on layout axis
            f32 max = 0.0f;
            for (UI_Box *child = root->first; child != nullptr; child = child->next) {
                if (child->fixed_size[axis] > max) max = child->fixed_size[axis];
            }
            root->fixed_size[axis] = max;
        }
    }
}

internal void ui_layout_resolve_violations(UI_Box *root, Axis2 axis) {
    //@Note Resolve by basic clipping if not on the layout axis
    if (axis != root->child_layout_axis && !(root->flags & (UI_BoxFlag_OverflowX << axis))) {
        f32 max_size = root->fixed_size[axis];
        for (UI_Box *child = root->first; child != nullptr; child = child->next) {
            f32 child_size = child->fixed_size[axis];
            f32 violation = child_size - max_size;
            if (violation > 0) {
                child_size -= violation;
            }
            child->fixed_size[axis] = child_size;
        }
    }

    //@Note Resolve by doing percent fixup if on the layout axis
    if (axis == root->child_layout_axis && !(root->flags & (UI_BoxFlag_OverflowX << axis))) {
        f32 max_size = root->fixed_size[axis];
        f32 total_size = 0.f;
        f32 total_weighted_size = 0.f;
        for (UI_Box *child = root->first; child != nullptr; child = child->next) {
            if (!(child->flags & UI_BoxFlag_FloatingX<<axis)) {
                total_size += child->fixed_size[axis];
                total_weighted_size += child->fixed_size[axis] * (1 - child->pref_size[axis].strictness);
            }
        }

        f32 violation = total_size - max_size;
        if (violation > 0) {
            for (UI_Box *child = root->first; child != nullptr; child = child->next) {
                if (!(child->flags & UI_BoxFlag_FloatingX<<axis)) {
                    f32 fixup = child->fixed_size[axis] * (1 - child->pref_size[axis].strictness);
                    fixup = ClampBot(fixup, 0);
                    f32 fix_pct = violation / total_weighted_size;
                    fix_pct = Clamp(fix_pct, 0, 1);
                    child->fixed_size[axis] -= fixup * fix_pct;
                }
            }
        }
    }

    //@Note Fixup upwards-relative sizes
    if (root->flags & (UI_BoxFlag_OverflowX << axis)) {
        for (UI_Box *child = root->first; child != nullptr; child = child->next) {
            if (child->pref_size[axis].kind == UI_SizeKind_ParentPct) {
                child->fixed_size[axis] = root->fixed_size[axis] * child->pref_size[axis].value;
            }
        }
    }
    
    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_resolve_violations(child, axis);
    }
}

internal void ui_layout_place_boxes(UI_Box *root, Axis2 axis) {
    f32 layout_position = 0.f;
    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        if (!(child->flags & UI_BoxFlag_FloatingX<<axis)) {
            child->fixed_position[axis] = layout_position;
            if (root->child_layout_axis == axis) {
                layout_position += child->fixed_size[axis];
            }
        }

        child->rect.p0[axis] = root->rect.p0[axis] + child->fixed_position[axis];
        child->rect.p1[axis] = child->rect.p0[axis] + child->fixed_size[axis];
    }

    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_place_boxes(child, axis);
    }
}

internal void ui_layout_apply(UI_Box *root) {
    for (Axis2 axis = Axis_X; axis < Axis_COUNT; axis = (Axis2)(axis + 1)) {
        ui_layout_calc_fixed(root, axis);
        ui_layout_calc_upwards_dependent(root, axis);
        ui_layout_calc_downwards_dependent(root, axis);
        ui_layout_resolve_violations(root, axis);
        ui_layout_place_boxes(root, axis);
    }
}

internal V2_F32 ui_text_position(UI_Box *box) {
    // V2_F32 text_position = VECTOR2(0.f, box->rect.y0 + .25f * rect_height(box->rect));
    V2_F32 text_position = v2_f32(0.f, box->rect.y0);
    switch (box->text_alignment) {
    case UI_TextAlign_Center:
        text_position.x = box->rect.x0 + 0.5f * rect_width(box->rect);
        text_position.x -= 0.5f * get_string_width(box->string, box->font);
        text_position.x = ClampBot(text_position.x, box->rect.x0);
        break;
    case UI_TextAlign_Left:
        text_position.x = box->rect.x0;
        break;
    case UI_TextAlign_Right:
        text_position.x = box->rect.x1 - get_string_width(box->string, box->font);
        text_position.x = ClampBot(text_position.x, box->rect.x0);
        break;
    }
    return text_position;
}

internal void ui_begin_build(f32 animation_dt, OS_Handle window_handle, OS_Event_List *events) {
    V2_F32 window_dim = os_get_window_dim(window_handle);

    ui_state->animation_dt = animation_dt;
    ui_state->mouse = os_window_is_focused(window_handle) ? os_mouse_from_window(window_handle) : v2_f32(-100.f, -100.f);

    ui_state->mouse_captured = false;
    ui_state->keyboard_captured = false;

    UI_Event *ui_event = nullptr;
    for (OS_Event *event = events->first; event; event = event->next) {
        switch (event->kind) {
        case OS_EventKind_MouseMove:
            ui_state->mouse.x = (f32)event->pos.x;
            ui_state->mouse.y = (f32)event->pos.y;
            break;
        case OS_EventKind_MouseDown:
            ui_event = ui_push_event(UI_EventKind_MousePress);
            ui_event->pos = event->pos;
            ui_event->key = event->key;
            break;
        case OS_EventKind_MouseUp:
            ui_event = ui_push_event(UI_EventKind_MouseRelease);
            ui_event->pos = event->pos;
            break;
        case OS_EventKind_Press:
            ui_event = ui_push_event(UI_EventKind_Press);
            ui_event->key = event->key;
            break;
        case OS_EventKind_Release:
            ui_event = ui_push_event(UI_EventKind_Release);
            ui_event->key = event->key;
            break;
        case OS_EventKind_Scroll:
            ui_event = ui_push_event(UI_EventKind_Scroll);
            ui_event->delta = event->delta;
            break;
        case OS_EventKind_Text:
            ui_event = ui_push_event(UI_EventKind_Text);
            ui_event->text = str8_copy(ui_build_arena(), event->text);
            break;
        }
    }

    ui_state->font_stack.top = NULL; ui_state->font_stack.first_free = NULL; ui_state->font_stack.auto_pop = false;
    ui_state->parent_stack.top = NULL; ui_state->parent_stack.first_free = NULL; ui_state->parent_stack.auto_pop = false;
    ui_state->fixed_x_stack.top = NULL; ui_state->fixed_x_stack.first_free = NULL; ui_state->fixed_x_stack.auto_pop = false;
    ui_state->fixed_y_stack.top = NULL; ui_state->fixed_y_stack.first_free = NULL; ui_state->fixed_y_stack.auto_pop = false;
    ui_state->fixed_width_stack.top = NULL; ui_state->fixed_width_stack.first_free = NULL; ui_state->fixed_width_stack.auto_pop = false;
    ui_state->fixed_height_stack.top = NULL; ui_state->fixed_height_stack.first_free = NULL; ui_state->fixed_height_stack.auto_pop = false;
    ui_state->pref_width_stack.top = NULL; ui_state->pref_width_stack.first_free = NULL; ui_state->pref_width_stack.auto_pop = false;
    ui_state->pref_height_stack.top = NULL; ui_state->pref_height_stack.first_free = NULL; ui_state->pref_height_stack.auto_pop = false;
    ui_state->child_layout_axis_stack.top = NULL; ui_state->child_layout_axis_stack.first_free = NULL; ui_state->child_layout_axis_stack.auto_pop = false;
    ui_state->text_alignment_stack.top = NULL; ui_state->text_alignment_stack.first_free = NULL; ui_state->text_alignment_stack.auto_pop = false;
    ui_state->background_color_stack.top = NULL; ui_state->background_color_stack.first_free = NULL; ui_state->background_color_stack.auto_pop = false;
    ui_state->border_color_stack.top = NULL; ui_state->border_color_stack.first_free = NULL; ui_state->border_color_stack.auto_pop = false;
    ui_state->text_color_stack.top = NULL; ui_state->text_color_stack.first_free = NULL; ui_state->text_color_stack.auto_pop = false;
    ui_state->hover_color_stack.top = NULL; ui_state->hover_color_stack.first_free = NULL; ui_state->hover_color_stack.auto_pop = false;
    ui_state->hover_cursor_stack.top = NULL; ui_state->hover_cursor_stack.first_free = NULL; ui_state->hover_cursor_stack.auto_pop = false;
    ui_state->border_thickness_stack.top = NULL; ui_state->border_thickness_stack.first_free = NULL; ui_state->border_thickness_stack.auto_pop = false;
    ui_state->box_flags_stack.top = NULL; ui_state->box_flags_stack.first_free = NULL; ui_state->box_flags_stack.auto_pop = false;
    ui_state->clip_box_stack.top = NULL; ui_state->clip_box_stack.first_free = NULL; ui_state->clip_box_stack.auto_pop = false;

    ui_push_font(default_fonts[FONT_DEFAULT]);
    ui_push_text_alignment(UI_TextAlign_Center);
    ui_push_child_layout_axis(Axis_Y);
    ui_push_background_color(v4_f32(.15f, .15f, .15f, 1.f));
    ui_push_text_color(v4_f32(.92f, .86f, .7f, 1.f));
    ui_push_border_color(v4_f32(.31f, .29f, .27f, 1.f));
    ui_push_hover_color(v4_f32(1.f, 1.f, 1.f, .3f));
    ui_push_border_thickness(0.f);
    ui_push_hover_cursor(OS_Cursor_Arrow);
    ui_push_pref_width(ui_px(100.f, 1.f));
    ui_push_pref_height(ui_px(20.f, 1.f));
    ui_push_box_flags(UI_BoxFlag_Nil);
    ui_push_clip_box(make_rect(0, 0, window_dim.x, window_dim.y));

    ui_state->build_counter = 0;

    UI_Box *root = ui_make_box_from_string(UI_BoxFlag_Nil, str8_lit("~Root"));
    root->flags = UI_BoxFlag_FloatingX | UI_BoxFlag_FloatingY | UI_BoxFlag_FixedWidth | UI_BoxFlag_FixedHeight;
    root->fixed_position = V2_Zero;
    root->fixed_size = window_dim;
    ui_state->root = root;
    ui_push_parent(root);
}

//@Note Get rect, hash and id of each box for collections
// Used for checking overlapping boxes
internal void ui_collect_build_boxes(UI_Box *root) {
    for (UI_Box *child = root->first; child; child = child->next) {
        ui_collect_build_boxes(child);
    }
    UI_Box box{};
    box.key = root->key;
    box.box_id = root->box_id;
    box.rect = root->rect;
    ui_state->last_build_collection.push(box);
}

internal void ui_end_build() {
    UI_Box *hover_box = ui_box_from_key(ui_state->hot_box_key);
    UI_Box *active_box = ui_box_from_key(ui_state->active_box_key);
    UI_Box *focus_active_box = ui_box_from_key(ui_state->focus_active_box_key);
    if (hover_box) {
        os_set_cursor(hover_box->hover_cursor);
    } else {
        os_set_cursor(OS_Cursor_Arrow);
    }

    if (hover_box && !rect_contains(hover_box->rect, ui_mouse())) {
        ui_state->hot_box_key = 0;
    }

    // if (focus_active_box && !rect_contains(focus_active_box->rect, ui_mouse()) && ui_mouse_press()) {
        // ui_set_focus_active_key(0);
    // }

    ui_state->last_build_collection.reset_count();
    ui_collect_build_boxes(ui_state->root);

    ui_state->build_index += 1;

    ui_state->events.first = nullptr;
    ui_state->events.last = nullptr;

    //@Note Clear build for next frame
    arena_clear(ui_build_arena());
}

internal void ui_txt_buffer(UI_Txt_Buffer *buffer, String8 string) {
    MemoryZero(buffer->data, 1024);
    MemoryCopy(buffer->data, string.data, ClampTop(string.count, 1024));
    buffer->pos = string.count;
    buffer->count = string.count;
}

internal void ui_txt_bufferf(UI_Txt_Buffer *buffer, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    ui_txt_buffer(buffer, string);
}

internal UI_Box *ui_top_parent() { UI_Box *result = nullptr; if (ui_state->parent_stack.top) result = ui_state->parent_stack.top->v; return result; }

internal void ui_set_next_font(Font *v) { UI_StackSetNext(ui_state, Font, font, Font, v); }
internal void ui_set_next_parent(UI_Box *v) { UI_StackSetNext(ui_state, Parent, parent, UI_Box, v); }
internal void ui_set_next_fixed_x(f32 v) { UI_StackSetNext(ui_state, FixedX, fixed_x, f32, v); }
internal void ui_set_next_fixed_y(f32 v) { UI_StackSetNext(ui_state, FixedY, fixed_y, f32, v); }
internal void ui_set_next_fixed_xy(Axis2 axis,f32 v) { ((axis==Axis_X) ? ui_set_next_fixed_x : ui_set_next_fixed_y)(v); };
internal void ui_set_next_fixed_width(f32 v) { UI_StackSetNext(ui_state, FixedWidth, fixed_width, f32, v); }
internal void ui_set_next_fixed_height(f32 v) { UI_StackSetNext(ui_state, FixedHeight, fixed_height, f32, v); }
internal void ui_set_next_fixed_size(Axis2 axis,f32 v) { ((axis==Axis_X) ? ui_set_next_fixed_width : ui_set_next_fixed_height)(v); };
internal void ui_set_next_pref_width(UI_Size v) { UI_StackSetNext(ui_state, PrefWidth, pref_width, UI_Size, v); }
internal void ui_set_next_pref_height(UI_Size v) { UI_StackSetNext(ui_state, PrefHeight, pref_height, UI_Size, v); }
internal void ui_set_next_pref_size(Axis2 axis, UI_Size size) { ((axis == Axis_X) ? ui_set_next_pref_width : ui_set_next_pref_height)(size); }
internal void ui_set_next_child_layout_axis(Axis2 v) { UI_StackSetNext(ui_state, ChildLayoutAxis, child_layout_axis, Axis2, v); }
internal void ui_set_next_text_alignment(UI_Text_Align v) { UI_StackSetNext(ui_state, TextAlignment, text_alignment, UI_Text_Alignment, v); }
internal void ui_set_next_background_color(V4_F32 v) { UI_StackSetNext(ui_state, BackgroundColor, background_color, V4_F32, v); }
internal void ui_set_next_border_color(V4_F32 v) { UI_StackSetNext(ui_state, BorderColor, border_color, V4_F32, v); }
internal void ui_set_next_text_color(V4_F32 v) { UI_StackSetNext(ui_state, TextColor, text_color, V4_F32, v); }
internal void ui_set_next_hover_color(V4_F32 v) { UI_StackSetNext(ui_state, HoverColor, hover_color, V4_F32, v); }
internal void ui_set_next_border_thickness(f32 v) { UI_StackSetNext(ui_state, BorderThickness, border_thickness, f32, v); }
internal void ui_set_next_hover_cursor(OS_Cursor v) { UI_StackSetNext(ui_state, Cursor, hover_cursor, OS_Cursor, v); }
internal void ui_set_next_box_flags(UI_Box_Flags v) { UI_StackSetNext(ui_state, BoxFlags, box_flags, UI_Box_Flags, v); }

internal void ui_push_font(Font *v) { UI_StackPush(ui_state, Font, font, Font, v); }
internal void ui_push_parent(UI_Box *v) { UI_StackPush(ui_state, Parent, parent, UI_Box, v); }
internal void ui_push_fixed_x(f32 v) { UI_StackPush(ui_state, FixedX, fixed_x, f32, v); }
internal void ui_push_fixed_y(f32 v) { UI_StackPush(ui_state, FixedY, fixed_y, f32, v); } 
internal void ui_push_fixed_width(f32 v) { UI_StackPush(ui_state, FixedWidth, fixed_width, f32, v); }
internal void ui_push_fixed_height(f32 v) { UI_StackPush(ui_state, FixedHeight, fixed_height, f32, v); }
internal void ui_push_pref_width(UI_Size v) { UI_StackPush(ui_state, PrefWidth, pref_width, UI_Size, v); }
internal void ui_push_pref_height(UI_Size v) { UI_StackPush(ui_state, PrefHeight, pref_height, UI_Size, v); }
internal void ui_push_pref_size(Axis2 axis, UI_Size size) { ((axis == Axis_X) ? ui_push_pref_width : ui_push_pref_height)(size); }
internal void ui_push_child_layout_axis(Axis2 v) { UI_StackPush(ui_state, ChildLayoutAxis, child_layout_axis, Axis2, v); }
internal void ui_push_text_alignment(UI_Text_Align v) { UI_StackPush(ui_state, TextAlignment, text_alignment, UI_Text_Alignment, v); }
internal void ui_push_background_color(V4_F32 v) { UI_StackPush(ui_state, BackgroundColor, background_color, V4_F32, v); }
internal void ui_push_border_color(V4_F32 v) { UI_StackPush(ui_state, BorderColor, border_color, V4_F32, v); }
internal void ui_push_text_color(V4_F32 v) { UI_StackPush(ui_state, TextColor, text_color, V4_F32, v); }
internal void ui_push_hover_color(V4_F32 v) { UI_StackPush(ui_state, HoverColor, hover_color, V4_F32, v); }
internal void ui_push_border_thickness(f32 v) { UI_StackPush(ui_state, BorderThickness, border_thickness, f32, v); }
internal void ui_push_hover_cursor(OS_Cursor v) { UI_StackPush(ui_state, Cursor, hover_cursor, OS_Cursor, v); }
internal void ui_push_box_flags(UI_Box_Flags v) { UI_StackPush(ui_state, BoxFlags, box_flags, UI_Box_Flags, v); }
internal void ui_push_clip_box(Rect v) { UI_StackPush(ui_state, ClipBox, clip_box, Rect, v); }

internal Font *ui_pop_font()  { UI_StackPop(ui_state, Font, font); }
internal UI_Box *ui_pop_parent() { UI_StackPop(ui_state, Parent, parent); }
internal f32 ui_pop_fixed_x() { UI_StackPop(ui_state, FixedX, fixed_x); }
internal f32 ui_pop_fixed_y() { UI_StackPop(ui_state, FixedY, fixed_y); }
internal f32 ui_pop_fixed_width() { UI_StackPop(ui_state, FixedWidth, fixed_width); }
internal f32 ui_pop_fixed_height() { UI_StackPop(ui_state, FixedHeight, fixed_height); }
internal UI_Size ui_pop_pref_width() { UI_StackPop(ui_state, PrefWidth, pref_width); }
internal UI_Size ui_pop_pref_height() { UI_StackPop(ui_state, PrefHeight, pref_height); }
internal UI_Size ui_pop_pref_size(Axis2 axis) { UI_Size result = ((axis == Axis_X) ? ui_pop_pref_width : ui_pop_pref_height)(); return result; }
internal Axis2 ui_pop_child_layout_axis() { UI_StackPop(ui_state, ChildLayoutAxis, child_layout_axis); }
internal UI_Text_Align ui_pop_text_alignment() { UI_StackPop(ui_state, TextAlignment, text_alignment); }
internal V4_F32 ui_pop_background_color() { UI_StackPop(ui_state, BackgroundColor, background_color); }
internal V4_F32 ui_pop_border_color() { UI_StackPop(ui_state, BorderColor, border_color); }
internal V4_F32 ui_pop_text_color() { UI_StackPop(ui_state, TextColor, text_color); }
internal V4_F32 ui_pop_hover_color() { UI_StackPop(ui_state, HoverColor, hover_color); }
internal f32 ui_pop_border_thickness() { UI_StackPop(ui_state, BorderThickness, border_thickness); }
internal OS_Cursor ui_pop_hover_cursor() { UI_StackPop(ui_state, Cursor, hover_cursor); }
internal UI_Box_Flags ui_pop_box_flags() { UI_StackPop(ui_state, BoxFlags, box_flags); }
internal Rect ui_pop_clip_box() { UI_StackPop(ui_state, ClipBox, clip_box); }
