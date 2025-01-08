#ifndef BUFFER_H
#define BUFFER_H

typedef u32 Buffer_ID;
typedef s64 Buffer_Pos;

enum Seek_Dir {
    SeekDir_Forward,
    SeekDir_Backward,
    SeekDir_COUNT
};

enum Line_Ending {
    LineEnding_LF,
    LineEnding_CRLF,
    LineEnding_Unknown
};

struct Buffer_Cursor {
    Buffer_Pos pos;
    s64 line;
    s64 col;
};

#define DEFAULT_GAP_SIZE 1024

struct Buffer {
    Buffer *next;
    Buffer *prev;

    Buffer_ID id;

    String8 file_path;
    String8 file_name;

    u8 *text;
    s64 gap_start;
    s64 gap_end;
    s64 end;

    b32 modified;

    Line_Ending line_ending;
    Auto_Array<s64> line_starts;
};

internal Buffer *make_buffer(String8 file_name);
internal void buffer_init_contents(Buffer *buffer, String8 file_name, String8 string);

internal Line_Ending detect_line_ending(String8 string);
internal void remove_crlf(u8 *data, s64 count, u8 **out_data, u64 *out_count);

internal inline s64 buffer_gap_size(Buffer *buffer);
internal String8 string_from_buffer(Arena *arena, Buffer *buffer, bool apply_line_ends);
internal s64 get_position_from_line(Buffer *buffer, s64 line);
internal u8 buffer_at(Buffer *buffer, s64 position);
internal void buffer_update_line_starts(Buffer *buffer);

#endif // BUFFER_H
