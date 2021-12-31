#pragma once
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "os.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" 
{
#endif

int setClipboardHTML(const char *html, const char* fallbackPlaintext);

#ifdef __cplusplus
}
#endif

#endif /* CLIPBOARD_H */
