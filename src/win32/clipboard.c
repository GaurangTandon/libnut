#include "../clipboard.h"
#include <stdlib.h>
#include <stdio.h>

// Clipboard format explanation at
// https://docs.microsoft.com/en-us/windows/win32/dataxchg/html-clipboard-format
// Example code
// For HTML: https://docs.microsoft.com/en-za/troubleshoot/cpp/add-html-code-clipboard
// For plaintext: https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard#copying-information-to-the-clipboard

/**
 * @brief Prepare HTML clipboard content that is a valid
 * Windows clipboard format
 * 
 * @param htmlBuffer 
 * @return char*
 */
char *prepareHTMLDescriptor(const char *htmlBuffer)
{
    // Create temporary buffer for HTML header...
    // 400 is roughly the size of strings that we're copying below
    size_t bufferSize = 400 + strlen(htmlBuffer);

    char *buffer = (char *)malloc(bufferSize * sizeof(char));
    if (!buffer)
        return NULL;

    // Create a template string for the HTML header...
    strcpy(buffer,
           "Version:0.9\r\n"
           "StartHTML:00000000\r\n"
           "EndHTML:00000000\r\n"
           "StartFragment:00000000\r\n"
           "EndFragment:00000000\r\n"
           "<html><body>\r\n"
           "<!--StartFragment -->\r\n");

    // Append the HTML...
    strcat(buffer, htmlBuffer);
    strcat(buffer, "\r\n");
    // Finish up the HTML format...
    strcat(buffer,
           "<!--EndFragment-->\r\n"
           "</body>\r\n"
           "</html>");

    // Now go back, calculate all the lengths, and write out the
    // necessary header information. Note, wsprintf() truncates the
    // string when you overwrite it so you follow up with code to replace
    // the 0 appended at the end with a '\r'...
    char *ptr = strstr(buffer, "StartHTML");
    wsprintf(ptr + 10, "%08u", strstr(buffer, "<html>") - buffer);
    *(ptr + 10 + 8) = '\r';

    ptr = strstr(buffer, "EndHTML");
    wsprintf(ptr + 8, "%08u", strlen(buffer));
    *(ptr + 8 + 8) = '\r';

    ptr = strstr(buffer, "StartFragment");
    wsprintf(ptr + 14, "%08u", strstr(buffer, "<!--StartFrag") - buffer);
    *(ptr + 14 + 8) = '\r';

    ptr = strstr(buffer, "EndFragment");
    wsprintf(ptr + 12, "%08u", strstr(buffer, "<!--EndFrag") - buffer);
    *(ptr + 12 + 8) = '\r';

    return buffer;
}

/**
 * @brief Set the clipboard in a particular format
 * 
 * @param clipboardDescriptor  Denote the format of clipboard (plain text/html)
 * @param buffer content to be written into clipboard
 * @param bufferSize size of the buffer
 * @return int 
 */
int setClipboardData(int clipboardDescriptor, char *buffer, size_t bufferSize)
{
    int exitCode = 0;

    // Allocate global memory for transfer to clipboard context
    HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, bufferSize + 1);

    if (hText)
    {
        char *ptr = (char *)GlobalLock(hText);
        if (ptr)
        {
            strcpy(ptr, buffer);

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
    // Get clipboard ID for HTML format...
    static int cfid = 0;
    if (!cfid)
        cfid = RegisterClipboardFormat("HTML Format");

    int exitCode = 0;

    char *htmlBuffer = prepareHTMLDescriptor(html);
    if (!htmlBuffer) 
        return -1;

    if (OpenClipboard(NULL))
    {
        if (!EmptyClipboard())
        {
            exitCode = -2;
        }
        else
        {
            // html text
            int htmlExitCode = setClipboardData(cfid, htmlBuffer, strlen(htmlBuffer));

            if (!htmlExitCode)
            {
                // Set fall back text if available
                size_t fallbackPlaintextLen = fallbackPlaintext ? wcslen(fallbackPlaintext) + 1 : 0;
                if (fallbackPlaintext)
                {
                    // TODO: we should use CF_UNICODETEXT here instead?
                    exitCode = setClipboardData(CF_TEXT, fallbackPlaintext, fallbackPlaintextLen * sizeof(wchar_t));
                }
            }
            else
            {
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

    free(htmlBuffer);

    return exitCode;
}