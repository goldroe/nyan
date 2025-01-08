#ifndef MINIBUFFER_H
#define MINIBUFFER_H

struct Minibuffer : UI_View {
    UI_Txt_Buffer text;
    UI_Box *box;
};

#endif // MINIBUFFER_H
