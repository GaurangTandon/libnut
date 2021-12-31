#include "../clipboard.h"
#include <stdlib.h>
#include <stdio.h>

// Adapted from https://docs.microsoft.com/en-za/troubleshoot/cpp/add-html-code-clipboard
int setClipBoardHTMLRaw(const char *html)
{
    // Create temporary buffer for HTML header...
    size_t bufferSize = 400 + strlen(html);
    char *buf = (char*)malloc(bufferSize * sizeof(char));
    if (!buf)
        return -1;

    printf("h");
    // Get clipboard id for HTML format...
    static int cfid = 0;
    if (!cfid)
        cfid = RegisterClipboardFormat("HTML Format");
    printf("i");

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
    wsprintf(ptr+10, "%08u", strstr(buf, "<html>") - buf);
    *(ptr+10+8) = '\r';

    ptr = strstr(buf, "EndHTML");
    wsprintf(ptr+8, "%08u", strlen(buf));
    *(ptr+8+8) = '\r';

    ptr = strstr(buf, "StartFragment");
    wsprintf(ptr+14, "%08u", strstr(buf, "<!--StartFrag") - buf);
    *(ptr+14+8) = '\r';

    ptr = strstr(buf, "EndFragment");
    wsprintf(ptr+12, "%08u", strstr(buf, "<!--EndFrag") - buf);
    *(ptr+12+8) = '\r';

    printf("See: %s", buf);

    // Now you have everything in place ready to put on the clipboard.
    // Open the clipboard...
    if (OpenClipboard(0))
    {
        // Empty what's in there...
        EmptyClipboard();

        // Allocate global memory for transfer...
        HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, strlen(buf) + 4);

        // Put your string in the global memory...
        char *ptr = (char *)GlobalLock(hText);
        strcpy(ptr, buf);
        GlobalUnlock(hText);

        SetClipboardData(cfid, hText);

        CloseClipboard();
        // Free memory...
        GlobalFree(hText);
    } else {
        return -2;
    }
    // Clean up...
    free(buf);
}