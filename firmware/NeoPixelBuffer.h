#pragma once

#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/dma.h"
#include "hardware/pio.h"

union neopixel
{
    uint32_t colour;
    struct
    {
        uint32_t white : 8;
        uint32_t blue : 8;
        uint32_t red : 8;
        uint32_t green : 8;
    };
};

class NeoPixelFrame
{
    public:
        NeoPixelFrame(neopixel *backBuffer, const neopixel *frontBuffer) :
        _backBuffer(backBuffer),
        _frontBuffer(frontBuffer)
        {
        }

        void SetPixel(uint32_t x, uint32_t colour)
        {
            _backBuffer[x].colour = colour;
        }

        void SetPixel(uint32_t x, neopixel colour)
        {
            _backBuffer[x] = colour;
        }

        neopixel *GetBuffer() {
            return _backBuffer;
        }

        const neopixel *GetLastBuffer() {
            return _frontBuffer;
        }

    private:
        neopixel *_backBuffer;
        const neopixel *_frontBuffer;
};

class NeoPixelBuffer
{
    public:
        NeoPixelBuffer(uint32_t dmaChannel, uint32_t irq, PIO pio, uint32_t stateMachine, uint32_t pixelPin, uint32_t pixelCount);
        ~NeoPixelBuffer();

        NeoPixelFrame Swap()
        {
            auto temp = _frontBuffer;
            _frontBuffer = _backBuffer;
            _backBuffer = temp;

            sem_acquire_blocking(&_swapReady);
            dma_channel_set_read_addr(_dmaChannel, _frontBuffer, true);
            return NeoPixelFrame(_backBuffer, _frontBuffer);
        }


    private:

        static void __isr Dma0CompleteHandler();
        static void __isr Dma1CompleteHandler();

        static int64_t LatchDelayCompleteEntry(alarm_id_t id, void *userData);
        int64_t LatchDelayComplete();

        uint32_t _dmaChannel;
        uint32_t _dmaMask;
        uint32_t _irq;
        PIO _pio;
        uint32_t _stateMachine;
        uint32_t _programOffset;

        semaphore _swapReady;

        uint32_t _pixelCount;
        neopixel *_frontBuffer;
        neopixel *_backBuffer;
};
