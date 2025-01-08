#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

internal UI_Box *ui_row();
internal UI_Signal ui_row_end();
internal UI_Signal ui_label(String8 name);
internal UI_Signal ui_labelf(const char *fmt, ...);
internal UI_Signal ui_button(String8 name);
internal UI_Signal ui_buttonf(const char *fmt, ...);

#endif // UI_WIDGETS_H
