#include <Arduino.h>
#include "Util.h"

// better debugging. Inspired from https://gist.github.com/asheeshr/9004783 with some modifications

namespace Util
{
    void setup(void)
    {
        while (!Serial)
            ;
    }
} // namespace Util

void binaryPrint(char *rgchDest, uint32_t i)
{
    for (uint32_t mask = 0x8000; mask; mask >>= 1)
    {
        *rgchDest++ = i & mask ? '1' : '0';
    }
    *rgchDest = '\0';
}

void dbgprintf(char const *pszFmt UNUSED_IN_RELEASE, ...)
{

#ifdef DEBUG_SC

    static char rgchBuf[256];
    size_t iBuf = 0;

    char const *pszTmp;
    char rgchTmp[33];

    va_list argv;
    va_start(argv, pszFmt);

    // Helper lambda to append to buffer safely
    auto appendStr = [&](const char *s)
    {
        while (*s && iBuf < sizeof(rgchBuf) - 1)
            rgchBuf[iBuf++] = *s++;
    };

    auto appendChar = [&](char c)
    {
        if (iBuf < sizeof(rgchBuf) - 1)
            rgchBuf[iBuf++] = c;
    };

    pszTmp = pszFmt;
    while (*pszTmp)
    {
        if (*pszTmp == '%')
        {
            pszTmp++;
            switch (*pszTmp)
            {
            case 'd':
                snprintf(rgchTmp, sizeof(rgchTmp), "%d", va_arg(argv, int));
                appendStr(rgchTmp);
                break;

            case 'x':
            case 'X':
                snprintf(rgchTmp, sizeof(rgchTmp), "%X", va_arg(argv, uint32_t));
                appendStr(rgchTmp);
                break;

            case 'b':
                binaryPrint(rgchTmp, va_arg(argv, uint32_t));
                appendStr(rgchTmp);
                break;

            case 'l':
                snprintf(rgchTmp, sizeof(rgchTmp), "%ld", va_arg(argv, long));
                appendStr(rgchTmp);
                break;

            case 'u':
                snprintf(rgchTmp, sizeof(rgchTmp), "%lu", va_arg(argv, unsigned long));
                appendStr(rgchTmp);
                break;

            case 'f':
                snprintf(rgchTmp, sizeof(rgchTmp), "%f", va_arg(argv, double));
                appendStr(rgchTmp);
                break;

            case 'F':
                snprintf(rgchTmp, sizeof(rgchTmp), "%.8f", va_arg(argv, double));
                appendStr(rgchTmp);
                break;

            case 'c':
                appendChar((char)va_arg(argv, int));
                break;

            case 's':
                appendStr(va_arg(argv, char *));
                break;

            case '%':
                appendChar('%');
                break;

            default:
                break;
            }
        }
        else if (*pszTmp == '\n')
        {
            appendStr("\r\n");
        }
        else
        {
            appendChar(*pszTmp);
        }

        pszTmp++;
    }

    va_end(argv);

    rgchBuf[iBuf] = '\0';

    // Send with length prefix: D<length>{<message>}
    Serial.print("D");
    Serial.print(iBuf);
    Serial.print("{");
    Serial.print(rgchBuf);
    Serial.print("}");
#endif
}

void visualize(uint8_t *buf, size_t cb)
{
#ifdef VISUALIZER

    Serial.print("V");
    Serial.print(cb);
    Serial.print("{");
    Serial.write(buf, cb);
    Serial.print("}");

#endif
}