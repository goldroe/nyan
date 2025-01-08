#ifndef FONT_H
#define FONT_H

struct Glyph {
    f32 ax;
    f32 ay;
    f32 bx;
    f32 by;
    f32 bt;
    f32 bl;
    f32 to;
};

struct Font {
    String8 font_name;
    Glyph glyphs[1024];
    int width;
    int height;
    int max_bmp_height;
    f32 ascend;
    f32 descend;
    int bbox_height;
    f32 glyph_width;
    f32 glyph_height;
    R_Handle texture;
};

enum Font_Type {
    FONT_DEFAULT,
    FONT_ICON,
    FONT_CODE,
    FONT_COUNT,
};

#endif // FONT_H
