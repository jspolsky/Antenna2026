#include <Arduino.h>
#include "SPI.h"
#include <SD.h>

#include "Util.h"
#include "Gif.h"
#include "DipSwitch.h"

namespace Gif
{
    CRGB rgbFrames[MAX_FRAMES][NUM_LEDS]; // the buffer that LoadGif() will load into
    uint32_t cFrames = 0;                 // the number of frames currently loaded

    AnimatedGIF gif;
    File f;

    void setup()
    {
        dbgprintf("Gif::setup()\n");
        gif.begin(GIF_PALETTE_RGB888);
    }

    void loop()
    {
    }

    // called by DOM to learn about a GIF
    // returns true if it exists, false if it doesn't
    // sets *piDelay to the delay speed to use
    bool GetGifInfo(uint16_t ixGifNumber, int &iDelay)
    {
        char rgchFileName[12];  // "/65535.gif" + null = 11 chars max
        sprintf(rgchFileName, "/%03d.gif", ixGifNumber);

        uint32_t timeStart = millis();
        if (gif.open(rgchFileName, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
        {
            GIFINFO gi;

            dbgprintf("Successfully opened GIF %s; Canvas size = %d x %d\n", rgchFileName, gif.getCanvasWidth(), gif.getCanvasHeight());

            // The getInfo() method can be slow since it walks through the entire GIF file to count the frames
            // and gather info about total play time. Comment out this section if you don't need this info
            if (gif.getInfo(&gi))
            {
                dbgprintf("frame count: %d\n", gi.iFrameCount);
                dbgprintf("duration: %d ms\n", gi.iDuration);
                dbgprintf("max delay: %d ms\n", gi.iMaxDelay);
                dbgprintf("min delay: %d ms\n", gi.iMinDelay);

                iDelay = gi.iMinDelay;
            }

            gif.close();
            dbgprintf("Reading GIF took %d millis\n", millis() - timeStart);
            return true;
        }
        else
        {
            return false;
        }
    }

    void LoadGif(uint16_t ixGifNumber)
    {
        char rgchFileName[12];  // "/65535.gif" + null = 11 chars max
        sprintf(rgchFileName, "/%03d.gif", ixGifNumber);

        uint32_t timeStart = millis();
        if (gif.open(rgchFileName, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
        {
            dbgprintf("Successfully opened GIF %s; Canvas size = %d x %d\n", rgchFileName, gif.getCanvasWidth(), gif.getCanvasHeight());

            if (gif.allocFrameBuf(GIFAlloc) == GIF_SUCCESS)
            {
                gif.setDrawType(GIF_DRAW_COOKED);

                int32_t iFrame = 0;
                while (gif.playFrame(false, NULL, &iFrame))
                {
                    iFrame++;
                }
                gif.freeFrameBuf(GIFFree);
            }
            else
            {
                dbgprintf("Insufficient memory\n");
            }

            gif.close();
            dbgprintf("Reading GIF took %d millis\n", millis() - timeStart);
        }
        else
        {
            dbgprintf("Error opening file = %d\n", gif.getLastError());
        }
    }

    void GetFrame(uint32_t frame, CRGB *leds)
    {
        memcpy(leds, rgbFrames[frame % cFrames], NUM_LEDS * 3);
    }

    void *GIFOpenFile(const char *fname, int32_t *pSize)
    {
        f = SD.open(fname);
        if (f)
        {
            *pSize = f.size();
            return (void *)&f;
        }
        return NULL;
    }

    void GIFCloseFile(void *pHandle)
    {
        File *f = static_cast<File *>(pHandle);
        if (f != NULL)
            f->close();
    }

    int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
    {
        int32_t iBytesRead;
        iBytesRead = iLen;
        File *f = static_cast<File *>(pFile->fHandle);
        // Note: If you read a file all the way to the last byte, seek() stops working
        if ((pFile->iSize - pFile->iPos) < iLen)
            iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
        if (iBytesRead <= 0)
            return 0;
        iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
        pFile->iPos = f->position();
        return iBytesRead;
    }

    int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
    {
        File *f = static_cast<File *>(pFile->fHandle);
        f->seek(iPosition);
        pFile->iPos = (int32_t)f->position();
        return pFile->iPos;
    }

    void GIFDraw(GIFDRAW *pDraw)
    {
        int line = pDraw->y;
        if (line == DipSwitch::getWhipNumber())
        {
            int32_t frame = *(int32_t *)(pDraw->pUser);
            memcpy(rgbFrames[frame], pDraw->pPixels, NUM_LEDS * 3);
            cFrames = frame + 1;
        }
    }

    void *GIFAlloc(uint32_t u32Size)
    {
        dbgprintf("Allocating %d for a frame\n", u32Size);
        return malloc(u32Size);
    }
    void GIFFree(void *p)
    {
        dbgprintf("Free\n");
        free(p);
    }
}
