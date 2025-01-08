internal u64 cstr8_length(const char *c) {
    if (c == nullptr) return 0;
    u64 result = 0;
    while (*c++) {
        result++;
    }
    return result;
}

internal String8 str8_zero() {
    String8 result = {0, 0};
    return result;
}

internal String8 str8(u8 *c, u64 count) {
    String8 result = {(u8 *)c, count};
    return result;
}

internal String8 str8_cstring(const char *c) {
    String8 result;
    result.count = cstr8_length(c);
    result.data = (u8 *)c;
    return result;
}

internal String8 str8_rng(String8 string, Rng_U64 rng) {
    String8 result;
    result.data = string.data + rng.min;
    result.count = rng.max - rng.min;
    return result;
}

internal String8 str8_copy(Arena *arena, String8 string) {
    String8 result;
    result.count = string.count;
    result.data = (u8 *)arena_push(arena, result.count + 1);
    MemoryCopy(result.data, string.data, string.count);
    result.data[result.count] = 0;
    return result;
}

internal String8 str8_concat(Arena *arena, String8 first, String8 second) {
    String8 result;
    result.count = first.count + second.count;
    result.data = push_array(arena, u8, result.count + 1);
    MemoryCopy(result.data, first.data, first.count);
    MemoryCopy(result.data + first.count, second.data, second.count);
    result.data[result.count] = 0;
    return result;
}

internal bool str8_match(String8 first, String8 second, String_Match_Flags flags) {
    if (first.count != second.count) return false;
    u8 a, b;
    for (u64 i = 0; i < first.count; i++) {
        a = first.data[i];
        b = second.data[i];
        if (flags & StringMatchFlag_CaseInsensitive) {
            a = (u8)tolower(a);
            b = (u8)tolower(b);
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

internal String8 str8_pushfv(Arena *arena, const char *fmt, va_list args) {
    va_list args_;
    va_copy(args_, args);
    String8 result;
    int bytes = base_vsnprintf(NULL, NULL, fmt, args_) + 1;
    result.data = push_array(arena, u8, bytes);
    result.count = base_vsnprintf((char *)result.data, bytes, fmt, args_);
    result.data[result.count] = 0;
    va_end(args_);
    return result;
}

internal String8 str8_pushf(Arena *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 result;
    int bytes = base_vsnprintf(NULL, NULL, fmt, args) + 1;
    result.data = push_array(arena, u8, bytes);
    result.count = base_vsnprintf((char *)result.data, bytes, fmt, args);
    result.data[result.count] = 0;
    va_end(args);
    return result;
}

internal String8 str8_jump(String8 string, u64 count) {
    String8 result;
    result.data = string.data + count;
    result.count = string.count - count;
    return result;
}

internal u64 str8_find_substr(String8 string, String8 substring) {
    if (substring.count > string.count) {
        return string.count;
    }

    u64 result = string.count;
    for (u64 string_cursor = 0; string_cursor < string.count; string_cursor++) {
        u64 rem = string.count - string_cursor;
        if (rem < substring.count) {
            break;
        }
        
        if (string.data[string_cursor] == substring.data[0]) {
            int cmp = memcmp(string.data + string_cursor, substring.data, substring.count);
            if (cmp == 0) {
                result = string_cursor;
                break;
            }
        }
    }
    return result;
}

internal u64 djb2_hash_string(String8 string) {
    u64 result = 5381;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}
