#ifndef DISPLAY_H
#define DISPLAY_H

#include "stack.h"
#include "palette.h"

int   display_init(int width, int height);
void  display_show(const Elem_t* vram, int width, int height);
int   display_should_close(void);                                         // 1, если пользователь закрыл окно (в консоли всегда 0)
void  display_wait_close(const Elem_t* vram, int width, int height);      // держит последний кадр, пока окно не закроют (в консоли сразу возвращается)
void  display_close(void);

#endif // DIDPLAY_H

