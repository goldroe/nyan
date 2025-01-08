#ifndef SEARCH_H
#define SEARCH_H

struct Search_Box : UI_View {
    UI_Txt_Buffer prompt;
    int match_index;
    Seek_Dir dir;
};

internal Search_Box *get_search_box();

#endif // SEARCH_H
