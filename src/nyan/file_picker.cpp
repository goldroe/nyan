global File_Picker *g_file_picker;

int file_sort_compare__file_name(const void *arg1, const void *arg2) {
    OS_File *a = (OS_File *)arg1;
    OS_File *b = (OS_File *)arg2;
    int size = (int)Min(a->file_name.count, b->file_name.count);
    int result = strncmp((char *)a->file_name.data, (char *)b->file_name.data, size);
    return result;
}

int file_sort_compare__default(const void *arg1, const void *arg2) {
    OS_File *a = (OS_File *)arg1;
    OS_File *b = (OS_File *)arg2;
    int result = 0;
    if (a->flags & OS_FileFlag_Directory && !(b->flags & OS_FileFlag_Directory)) {
        result = -1;
    } else if (b->flags & OS_FileFlag_Directory && !(a->flags & OS_FileFlag_Directory)) {
        result = 1;
    } else {
        result = file_sort_compare__file_name(a, b);
    }
    return result;
}

internal File_Picker *make_file_picker() {
    Arena *arena = arena_alloc(get_virtual_allocator(), KB(128));
    File_Picker *picker = push_array(arena, File_Picker, 1);
    picker->arena = arena;
    picker->cached_files_arena = arena_alloc(get_virtual_allocator(), KB(64));
    picker->keymap = get_picker_keymap();

    Application *app = get_application();
    UI_View *ui_view = (UI_View *)picker;
    DLLPushBack(app->ui_view_list.first, app->ui_view_list.last, ui_view, view_next, view_prev);
    app->ui_view_list.count++;
    return picker;
}

internal void file_picker_prompt(File_Picker *picker, String8 path) {
    ui_txt_buffer(&picker->path_buffer, path);

    Arena *scratch = make_arena(get_malloc_allocator());
    String8 find_path = str8_copy(scratch, str8(picker->path_buffer.data, picker->path_buffer.count));

    OS_File file;
    OS_File_List file_list = {};
    OS_Handle find_handle = os_find_first_file(picker->arena, find_path, &file);
    if (os_valid_handle(find_handle)) {
        do {
            if (str8_match(file.file_name, str8_lit("."), StringMatchFlag_CaseInsensitive) || str8_match(file.file_name, str8_lit(".."), StringMatchFlag_CaseInsensitive)) {
                continue;
            }
            OS_File_Node *file_node = push_array(scratch, OS_File_Node, 1);
            file_node->file = file;
            SLLQueuePush(file_list.first, file_list.last, file_node);
            file_list.count++;
        } while (os_find_next_file(picker->arena, find_handle, &file));
        os_find_close(find_handle);
    }

    arena_clear(picker->cached_files_arena);
    picker->cached_files = push_array(picker->cached_files_arena, OS_File, file_list.count);
    int idx = 0;
    for (OS_File_Node *node = file_list.first; node; node = node->next, idx++) {
        picker->cached_files[idx] = node->file;
    }
    picker->cached_file_count = file_list.count;

    qsort(picker->cached_files, picker->cached_file_count, sizeof(OS_File), file_sort_compare__default);

    arena_release(scratch);

    picker->file_index = 0;
}
