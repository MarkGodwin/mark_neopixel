
#include <stdio.h>
#include <string.h>
#include "NeoPixelBuffer.h"
#include "neopixel.pio.h"

static NeoPixelBuffer *_dma0Buffer = nullptr;
static NeoPixelBuffer *_dma1Buffer = nullptr;

void __isr NeoPixelBuffer::Dma0CompleteHandler()
{
    if (dma_hw->ints0 & 1)
    {
        // clear IRQ
        dma_hw->ints0 = 1;
        add_alarm_in_us(400, NeoPixelBuffer::LatchDelayCompleteEntry, _dma0Buffer, true);
    }
}

void __isr NeoPixelBuffer::Dma1CompleteHandler()
{
    if (dma_hw->ints0 & 2)
    {
        // clear IRQ
        dma_hw->ints0 = 2;
        add_alarm_in_us(400, NeoPixelBuffer::LatchDelayCompleteEntry, _dma1Buffer, true);
    }
}


NeoPixelBuffer::NeoPixelBuffer(uint32_t dmaChannel, uint32_t irq, PIO pio, uint32_t stateMachine, uint32_t pixelPin, uint32_t pixelCount)
:   _dmaChannel(dmaChannel),
    _dmaMask(1 << dmaChannel),
    _irq(irq),
    _pio(pio),
    _stateMachine(stateMachine),
    _pixelCount(pixelCount)
{
    if(dmaChannel == 0)
        ::_dma0Buffer = this;
    else
        ::_dma1Buffer = this;

    _frontBuffer = new neopixel[_pixelCount];
    _backBuffer = new neopixel[_pixelCount];
    ::memset(_frontBuffer, 0, sizeof(neopixel) * sizeof(_pixelCount));
    ::memset(_backBuffer, 0, sizeof(neopixel) * sizeof(_pixelCount));

    _programOffset = pio_add_program(_pio, &neopixel_program);    
    neopixel_program_init(_pio, _stateMachine, _programOffset, pixelPin, false);

    sem_init(&_swapReady, 10, 10);

    // Set up the DMA channel to pipe the bit data to the PIO program
    dma_claim_mask(_dmaMask);
    auto config = dma_channel_get_default_config(_dmaChannel);
    channel_config_set_dreq(&config, pio_get_dreq(_pio, _stateMachine, true));
    dma_channel_configure(_dmaChannel,
                          &config,
                          &_pio->txf[_stateMachine],
                          NULL,
                          _pixelCount, // Each pixel value is a DMA_SIZE_32 word
                          false);

    if(_irq == DMA_IRQ_0)
        dma_channel_set_irq0_enabled(_dmaChannel, true);
    else
        dma_channel_set_irq1_enabled(_dmaChannel, true);
    if(_dmaChannel == 0)
        irq_set_exclusive_handler(_irq, Dma0CompleteHandler);
    else if(_dmaChannel == 1)
        irq_set_exclusive_handler(_irq, Dma1CompleteHandler);

    irq_set_enabled(_irq, true);
}

NeoPixelBuffer::~NeoPixelBuffer()
{
    sem_acquire_blocking(&_swapReady);

    irq_set_enabled(_irq, false);
    if(_irq == DMA_IRQ_0)
        dma_channel_set_irq0_enabled(_dmaChannel, false);
    else
        dma_channel_set_irq1_enabled(_dmaChannel, false);
    dma_unclaim_mask(_dmaMask);

    pio_remove_program(_pio, &neopixel_program, _programOffset);
}

int64_t NeoPixelBuffer::LatchDelayCompleteEntry(alarm_id_t id, void *userData)
{
    auto pthis = (NeoPixelBuffer *)userData;
    return pthis->LatchDelayComplete();
}

int64_t NeoPixelBuffer::LatchDelayComplete()
{
    auto x = sem_release(&_swapReady);
    if(!x)
        puts("Unlock fail");
    return 0;
}

