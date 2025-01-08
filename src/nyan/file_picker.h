#ifndef FILE_PICKER_H
#define FILE_PICKER_H

struct File_Picker : UI_View {
    Arena *arena;
    UI_Txt_Buffer path_buffer;

    int file_index;
    Arena *cached_files_arena;
    OS_File *cached_files;
    int cached_file_count;

    UI_Box *box;
};

#endif // PICKER_H
