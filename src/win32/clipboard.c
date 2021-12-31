#include "../clipboard.h"
#include <stdlib.h>
#include <stdio.h>

/* REMINDER: 
    Please be civil in C memory usage
    Check the exit code of each method that can go wrong 
    Using unintended memory is the main cause of all bugs in C ever 
    See pie chart at this blog
    https://daniel.haxx.se/blog/2021/03/09/half-of-curls-vulnerabilities-are-c-mistakes/ 
    
    Also, we always return from a function only in the end so as to free up any memory that
    we may have initialized on the way */

// Clipboard format explanation at https://docs.microsoft.com/en-us/windows/win32/dataxchg/html-clipboard-format
// Example code
// For HTML: https://docs.microsoft.com/en-za/troubleshoot/cpp/add-html-code-clipboard
// For plaintext: https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard#copying-information-to-the-clipboard

char *prepareHTMLDescriptor(const char *html)
{
    // Create temporary buffer for HTML header...
    size_t bufferSize = 400 + strlen(html);

    char *buf = (char *)malloc(bufferSize * sizeof(char));
    if (!buf)
        return NULL;

    // Create a template string for the HTML header...
    strcpy(buf,
           "Version:0.9\r\n"
           "StartHTML:00000000\r\n"
           "EndHTML:00000000\r\n"
           "StartFragment:00000000\r\n"
           "EndFragment:00000000\r\n"
           "<html><body>\r\n"
           "<!--StartFragment -->\r\n");

    // Append the HTML...
    strcat(buf, html);
    strcat(buf, "\r\n");
    // Finish up the HTML format...
    strcat(buf,
           "<!--EndFragment-->\r\n"
           "</body>\r\n"
           "</html>");

    // Now go back, calculate all the lengths, and write out the
    // necessary header information. Note, wsprintf() truncates the
    // string when you overwrite it so you follow up with code to replace
    // the 0 appended at the end with a '\r'...
    char *ptr = strstr(buf, "StartHTML");
    wsprintf(ptr + 10, "%08u", strstr(buf, "<html>") - buf);
    *(ptr + 10 + 8) = '\r';

    ptr = strstr(buf, "EndHTML");
    wsprintf(ptr + 8, "%08u", strlen(buf));
    *(ptr + 8 + 8) = '\r';

    ptr = strstr(buf, "StartFragment");
    wsprintf(ptr + 14, "%08u", strstr(buf, "<!--StartFrag") - buf);
    *(ptr + 14 + 8) = '\r';

    ptr = strstr(buf, "EndFragment");
    wsprintf(ptr + 12, "%08u", strstr(buf, "<!--EndFrag") - buf);
    *(ptr + 12 + 8) = '\r';

    return buf;
}

int setClipboardData(int clipboardDescriptor, char* buf, size_t allocSize) {
    int exitCode = 0;

    printf("Clipboard contents: %s\n", buf);

    // Allocate global memory for transfer to clipboard context
    // TODO: is this susceptible to memory leaks? Should we do +1 to buffer size?
    HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, allocSize);

    if (hText)
    {
        char *ptr = (char *)GlobalLock(hText);
        if (ptr)
        {
            strcpy(ptr, buf);

            // TODO: why are we unlocking the handle before setting clipboard data?
            // Would it allow the memeory to be modified by some other program?
            if (!GlobalUnlock(hText))
            {
                if (GetLastError() == NO_ERROR)
                {
                    if (!SetClipboardData(clipboardDescriptor, hText))
                    {
                        exitCode = 5;
                    }
                }
                else
                {
                    exitCode = 4;
                }
            }
            else
            {
                exitCode = 4;
            }
        }
        else
        {
            exitCode = 3;
        }
    }
    else
    {
        exitCode = 2;
    }

    GlobalFree(hText);

    return exitCode;
}

/* 
 * Convenient API references:
 * API:
 * https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
 * Global memory:
 * https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-globalalloc
 * https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-globallock
 * https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-globalfree
 * Clipboard:
 * https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-openclipboard
 * https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-emptyclipboard
 * https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclipboarddata
 * https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-closeclipboard
*/

int setClipboardHTML(const char *html, const char *fallbackPlaintext)
{
    // Get clipboard id for HTML format...
    static int cfid = 0;
    if (!cfid)
        cfid = RegisterClipboardFormat("HTML Format");

    char *buf = prepareHTMLDescriptor(html);
    int exitCode = 0;

    if (OpenClipboard(NULL))
    {
        if (!EmptyClipboard()) {
            exitCode = -2;
        } else {
            // html text
            int htmlExitCode = setClipboardData(cfid, buf, strlen(buf));

            if (!htmlExitCode) {
                // Set fall back text if available
                size_t fallbackPlaintextLen = fallbackPlaintext ? wcslen(fallbackPlaintext) + 1 : 0;
                if (fallbackPlaintext) {
                    // TODO: we should use CF_UNICODETEXT here instead?
                    exitCode = setClipboardData(CF_TEXT, fallbackPlaintext, fallbackPlaintextLen * sizeof(wchar_t));
                }
            } else {
                exitCode = htmlExitCode;
            }

            if (!CloseClipboard())
            {
                exitCode = -3;
            }
        }
    }
    else
    {
        exitCode = -1;
    }

    free(buf);

    return exitCode;
}