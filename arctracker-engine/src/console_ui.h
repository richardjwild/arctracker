#ifndef ARCTRACKER_CONSOLE_H
#define ARCTRACKER_CONSOLE_H

#include "lib/libarctracker.h"

void ui_loop(arctracker_t *arctracker_handle, ui_module_info_t module_info);

bool monitor_export(arctracker_t *arctracker_handle);

#endif //ARCTRACKER_CONSOLE_H
