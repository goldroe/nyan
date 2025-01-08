
internal Buffer_Cursor buffer_cursor_zero() {
    Buffer_Cursor cursor = {};
    return cursor;
}

internal void buffer_init_contents(Buffer *buffer, String8 file_name, String8 string) {
    Base_Allocator *base_allocator = get_malloc_allocator();
    buffer->file_path = path_strip_dir_name(arena_alloc(base_allocator, file_name.count + 1), file_name);
    buffer->file_name = path_strip_file_name(arena_alloc(base_allocator, file_name.count + 1), file_name);
    buffer->text = string.data;
    buffer->gap_start = 0;
    buffer->gap_end = 0;
    buffer->end = string.count;
    buffer->line_ending = LineEnding_LF;
    buffer_update_line_starts(buffer);
}

internal Buffer *make_buffer(String8 file_name) {
    Buffer *buffer = new Buffer();
    OS_Handle handle = os_open_file(file_name, OS_AccessFlag_Read);
    if (os_valid_handle(handle)) {
        u8 *file_data = nullptr;
        u64 file_size = os_read_entire_file(handle, (void **)&file_data);
        os_close_handle(handle);
        Assert(file_data);

        String8 buffer_string = {file_data, file_size};
        Line_Ending line_ending = detect_line_ending(buffer_string);
        if (line_ending != LineEnding_LF) {
            String8 adjusted = {};
            remove_crlf(buffer_string.data, buffer_string.count, &adjusted.data, &adjusted.count);
            free(file_data);
            buffer_string = adjusted;
        }
        buffer_init_contents(buffer, file_name, buffer_string);
        buffer->line_ending = line_ending;
    } else {
        String8 string = str8_zero();
        buffer_init_contents(buffer, file_name, string);
        buffer->line_ending = LineEnding_LF;
        #if OS_WINDOWS
        buffer->line_ending = LineEnding_CRLF;
        #endif
    }

    Application *app = get_application();
    buffer->id = app->buffer_list.count;
    DLLPushBack(app->buffer_list.first, app->buffer_list.last, buffer, next, prev);
    app->buffer_list.count += 1;
    return buffer;
}

internal inline s64 buffer_size(Buffer *buffer) {
    return buffer->end - buffer_gap_size(buffer);
}

internal s64 buffer_get_line_length(Buffer *buffer, s64 line) {
    s64 length = buffer->line_starts[line + 1] - buffer->line_starts[line] - 1;
    return length;
}

internal s64 buffer_get_line_count(Buffer *buffer) {
    s64 result = buffer->line_starts.count - 1;
    return result;
}

internal s64 buffer_get_length(Buffer *buffer) {
    s64 result = buffer_size(buffer);
    return result;
}

internal inline s64 buffer_gap_size(Buffer *buffer) {
    return buffer->gap_end - buffer->gap_start;
}

internal inline b32 ptr_in_gap(Buffer *buffer, u8 *ptr) {
    return (ptr >= buffer->text + buffer->gap_start && ptr < buffer->text + buffer->gap_end);
}

internal void write_buffer(Buffer *buffer) {
    Arena *scratch = make_arena(get_virtual_allocator());
    String8 full_path = path_join(scratch, buffer->file_path, buffer->file_name);
    String8 buffer_string = string_from_buffer(scratch, buffer, true);
    OS_Handle file_handle = os_open_file(full_path, OS_AccessFlag_Write);
    if (os_valid_handle(file_handle)) {
        os_write_file(file_handle, buffer_string.data, buffer_string.count);
        os_close_handle(file_handle);
        buffer->modified = false;
    }
    arena_release(scratch);
}

internal String8 string_from_buffer(Arena *arena, Buffer *buffer, bool apply_line_ends) {
    s64 line_count = buffer_get_line_count(buffer);
    s64 buffer_length = buffer_get_length(buffer);
    if (apply_line_ends) {
        buffer_length += (line_count + 1)*(buffer->line_ending == LineEnding_CRLF); 
    }

    String8 result{};
    result.data = push_array_no_zero(arena, u8, buffer_length + 1);
    result.count = buffer_length;
    s64 str_idx = 0;
    for (s64 line = 0; line < line_count; line += 1) {
        s64 start = get_position_from_line(buffer, line);
        s64 line_length = buffer_get_line_length(buffer, line);
        s64 end = start + line_length;
        for (s64 i = start; i < end; i += 1) {
            result.data[str_idx++] = buffer_at(buffer, i);
        }
        if (apply_line_ends) {
            if (buffer->line_ending == LineEnding_LF) {
                result.data[str_idx++] = '\n';
            } else if (buffer->line_ending == LineEnding_CRLF) {
                result.data[str_idx++] = '\r';
                result.data[str_idx++] = '\n';
            }
        } else {
            result.data[str_idx++] = '\n';
        }
    }
    result.data[str_idx] = 0;
    return result;
}

internal String8 string_range_from_buffer(Arena *arena, Buffer *buffer, Rng_S64 rng) {
    String8 result = str8_zero();
    s64 count = rng_s64_len(rng);
    result.data = push_array(arena, u8, count + 1);
    result.count = count;
    for (s64 i = 0, rng_idx = rng.min; rng_idx < rng.max; i += 1, rng_idx += 1) {
        result.data[i] = buffer_at(buffer, rng_idx);
    }
    result.data[count] = 0;
    return result;
}

internal void remove_crlf(u8 *data, s64 count, u8 **out_data, u64 *out_count) {
    u8 *result = (u8 *)malloc(count);
    u8 *src = data;
    u8 *src_end = data + count;
    u8 *dest = result;
    while (src < data + count) {
        switch (*src) {
        default:
            *dest++ = *src++;
            break;
        case '\r':
            src++;
            if (src < src_end && *src == '\n') src++;
            *dest++ = '\n';
            break;
        case '\n':
            src++;
            if (src < src_end && *src == '\r') src++;
            *dest++ = '\n';
            break;
        }
    }

    u64 new_count = dest - result;
    result = (u8 *)realloc(result, new_count);
    if (out_data) *out_data = result;
    if (out_count) *out_count = new_count;
}

internal Line_Ending detect_line_ending(String8 string) {
    for (u64 i = 0; i < string.count; i++) {
        switch (string.data[i]) {
        case '\r':
            if (i + 1 < string.count && string.data[i + 1] == '\n') {
                return LineEnding_CRLF;
            } else {
                return LineEnding_Unknown;
            }
        case '\n':
            return LineEnding_LF;
        }
    }
    return LineEnding_LF;
}

internal s64 buffer_position_logical(Buffer *buffer, s64 position) {
    if (position > buffer->gap_start) {
        position -= buffer_gap_size(buffer);
    }
    return position;
}

internal u8 buffer_at(Buffer *buffer, s64 position) {
    s64 index = position;
    if (index >= buffer->gap_start) {
        index += buffer_gap_size(buffer);
    }
    u8 c = 0;
    if (buffer_get_length(buffer) > 0) {
       c = buffer->text[index]; 
    }
    return c;
}

internal void buffer_update_line_starts(Buffer *buffer) {
    buffer->line_starts.reset_count();
    buffer->line_starts.push(0);
    
    u8 *text = buffer->text;
    for (;;) {
        if (text >= buffer->text + buffer->end) break;
        if (ptr_in_gap(buffer, text)) {
            text = buffer->text + buffer->gap_end;
            continue;
        }

        bool newline = false;
        switch (*text) {
        default:
            text++;
            break;
        case '\r':
            text++;
            if (!ptr_in_gap(buffer, text) && (text < buffer->text + buffer->end)) {
                if (*text == '\n') text++;
            }
            newline = true;
            break;
        case '\n':
            text++;
            if (!ptr_in_gap(buffer, text) && (text < buffer->text + buffer->end)) {
                if (*text == '\r') text++;
            }
            newline = true;
            break;
        }
        if (newline) {
            s64 position = text - buffer->text;
            position = buffer_position_logical(buffer, position);
            buffer->line_starts.push(position);
        }
    }
    buffer->line_starts.push(buffer_size(buffer) + 1);
}

internal void buffer_grow(Buffer *buffer, s64 gap_size) {
    s64 size1 = buffer->gap_start;
    s64 size2 = buffer->end - buffer->gap_end;
    u8 *data = (u8 *)calloc(buffer->end + gap_size, 1);
    memcpy(data, buffer->text, buffer->gap_start);
    memset(data + size1, '_', gap_size);
    memcpy(data + buffer->gap_start + gap_size, buffer->text + buffer->gap_end, buffer->end - buffer->gap_end);
    free(buffer->text);
    buffer->text = data;
    buffer->gap_end += gap_size;
    buffer->end += gap_size;
}

internal void buffer_shift_gap(Buffer *buffer, s64 new_gap) {
    u8 *temp = (u8 *)malloc(buffer->end);
    s64 gap_start = buffer->gap_start;
    s64 gap_end = buffer->gap_end;
    s64 gap_size = gap_end - gap_start;
    s64 size = buffer->end;
    if (new_gap > gap_start) {
        memcpy(temp, buffer->text, new_gap);
        memcpy(temp + gap_start, buffer->text + gap_end, new_gap - gap_start);
        memcpy(temp + new_gap + gap_size, buffer->text + new_gap + gap_size, size - (new_gap + gap_size));
    } else {
        memcpy(temp, buffer->text, new_gap);
        memcpy(temp + new_gap + gap_size, buffer->text + new_gap, gap_start - new_gap);
        memcpy(temp + new_gap + gap_size + (gap_start - new_gap), buffer->text + gap_end, size - gap_end);
    }
    free(buffer->text);
    buffer->text = temp;
    buffer->gap_start = new_gap;
    buffer->gap_end = new_gap + gap_size;
}

internal void buffer_ensure_gap(Buffer *buffer) {
    if (buffer->gap_end - buffer->gap_start == 0) {
        buffer_grow(buffer, DEFAULT_GAP_SIZE);
    }
}

internal void buffer_delete_region(Buffer *buffer, s64 start, s64 end) {
    Assert(start < end);
    if (buffer->gap_start != start) {
        buffer_shift_gap(buffer, start);
    }
    buffer->gap_end += (end - start);
    buffer_update_line_starts(buffer);
    buffer->modified = true;
}

internal void buffer_delete_single(Buffer *buffer, s64 position) {
    buffer_delete_region(buffer, position - 1, position);
}

internal void buffer_insert_single(Buffer *buffer, s64 position, u8 c) {
    buffer_ensure_gap(buffer);
    if (buffer->gap_start != position) {
        buffer_shift_gap(buffer, position);
    }
    buffer->text[position] = c;
    buffer->gap_start++;
    buffer_update_line_starts(buffer);
    buffer->modified = true;
}

internal void buffer_insert_string(Buffer *buffer, s64 position, String8 string) {
    if (buffer_gap_size(buffer) < (s64)string.count) {
        buffer_grow(buffer, string.count);
    }
    if (buffer->gap_start != position) {
        buffer_shift_gap(buffer, position);
    }
    MemoryCopy(buffer->text + position, string.data, string.count);
    buffer->gap_start += string.count;
    buffer_update_line_starts(buffer);
    buffer->modified = true;
}

internal void buffer_replace_region(Buffer *buffer, String8 string, s64 start, s64 end) {
    s64 region_size = end - start;
    buffer_delete_region(buffer, start, end);
    if (buffer_gap_size(buffer) < (s64)string.count) {
        buffer_grow(buffer, string.count);
    }
    memcpy(buffer->text + buffer->gap_start, string.data, string.count);
    buffer->gap_start += string.count;
    buffer_update_line_starts(buffer);
    buffer->modified = true;
}

internal void buffer_clear(Buffer *buffer) {
    buffer->gap_start = 0;
    buffer->gap_end = buffer->end;
}

internal Buffer_Cursor get_cursor_from_position(Buffer *buffer, s64 position) {
    Buffer_Cursor cursor = {};
    for (int line = 0; line < buffer->line_starts.count - 1; line++) {
        s64 start = buffer->line_starts[line];
        s64 end = buffer->line_starts[line + 1];
        if (start <= position && position < end) {
            cursor.line = line;
            cursor.col = position - start;
            break;
        }
    }
    cursor.pos = position;
    return cursor;
}

internal s64 get_position_from_line(Buffer *buffer, s64 line) {
    s64 position = buffer->line_starts[line];
    return position;
}

internal Buffer_Cursor get_cursor_from_line(Buffer *buffer, s64 line) {
    s64 position = get_position_from_line(buffer, line);
    Buffer_Cursor cursor = get_cursor_from_position(buffer, position);
    return cursor;
}

internal s64 buffer_indentation_from_line(Buffer *buffer, s64 line) {
    s64 result = 0;
    s64 start = get_position_from_line(buffer, line);
    for (s64 i = start, end = start + buffer_get_line_length(buffer, line); i < end; i += 1) {
        if (buffer_at(buffer, i) != ' ') {
            break;
        }
        result += 1;
    }
    return result;
}

internal Auto_Array<Rng_S64> buffer_find_text_matches(Buffer *buffer, String8 string) {
    Auto_Array<Rng_S64> match_ranges;
    for (s64 i = 0, length = buffer_get_length(buffer); i < length; i++) {
        u64 rem = length - i;
        if (rem < string.count) {
            break;
        }
        
        if (buffer_at(buffer, i) == string.data[0]) {
            bool matches = true;
            for (u64 j = 0; j < string.count; j++) {
                if (buffer_at(buffer, i + j) != string.data[j]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                Rng_S64 rng = rng_s64(i, i + string.count);
                match_ranges.push(rng);
            }
        }
    }
    return match_ranges;
}

internal Buffer_Pos buffer_skip_whitespace(Buffer *buffer, Buffer_Pos pos, Seek_Dir dir) {
    s64 result = pos;
    Assert(isspace(buffer_at(buffer, pos)));

    if (dir == SeekDir_Backward) {
        Buffer_Pos end = 0;
        while (pos > end) {
            u8 c = buffer_at(buffer, pos);
            if (!isspace(c)) {
                break;
            }
            pos--;
        }
        result = pos;
    } else if (dir == SeekDir_Forward) {
        Buffer_Pos end = buffer_get_length(buffer);
        while (pos < end) {
            u8 c = buffer_at(buffer, pos);
            if (!isspace(c)) {
                break;
            }
            pos++;
        }
        result = pos;
    }
    return result;
}

internal Buffer_Pos buffer_seek_word_end(Buffer *buffer, Buffer_Pos start, Seek_Dir dir) {
    Buffer_Pos result = start;
    u8 start_c = buffer_at(buffer, start);

    if (dir == SeekDir_Forward) {
        s64 end = buffer_get_length(buffer);
        Buffer_Pos pos = start;

        if (isspace(start_c)) {
            pos = buffer_skip_whitespace(buffer, pos, dir);
        }

        while (pos < end) {
            u8 c = buffer_at(buffer, pos);
            if (isspace(c)) {
                pos--;
                break;
            }
            pos++;
        }
        result = pos;
    } else if (dir == SeekDir_Backward) {
    }

    return result;
}

internal Buffer_Pos buffer_seek_word_begin(Buffer *buffer, Buffer_Pos start, Seek_Dir dir) {
    Buffer_Pos result = start;
    u8 start_c = buffer_at(buffer, start);

    if (dir == SeekDir_Forward) {
        s64 end = buffer_get_length(buffer);
        Buffer_Pos pos = start;
        if (isspace(start_c)) {
            buffer_skip_whitespace(buffer, pos, dir);
        }

        result = pos;
    } else if (dir == SeekDir_Backward) {
        s64 end = 0;
        Buffer_Pos pos = start;
        if (isspace(start_c)) {
            pos = buffer_skip_whitespace(buffer, pos, dir);
        }

        while (pos > end) {
            u8 c = buffer_at(buffer, pos);
            if (isspace(c)) {
                pos = pos + 1;
                break;
            }
            pos--;
        }
        result = pos;
    }

    return result;
}

internal Buffer_Pos buffer_get_start_of_line(Buffer *buffer, Buffer_Pos start) {
    Buffer_Pos pos = start;

    while (pos > 0) {
        if (buffer_at(buffer, pos) == '\n') {
            pos++;
            break;
        }
        pos--;
    }
    return pos;
}

internal Buffer_Pos buffer_get_end_of_line(Buffer *buffer, Buffer_Pos start) {
    Buffer_Pos pos = start;
    Buffer_Pos end = buffer_get_length(buffer);
    pos = ClampTop(pos, end - 1);
    while (pos < end - 1) {
        if (buffer_at(buffer, pos) == '\n') {
            break;
        }
        pos++;
    }
    return pos;
}
