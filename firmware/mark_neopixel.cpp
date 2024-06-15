#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/dma.h"
#include "neopixel.pio.h"
#include <string.h>

#include "NeoPixelBuffer.h"
#include <stdlib.h>

#define PIXEL_COUNT 70

#define DMA_CHANNEL 0
#define PIXEL_PIN 2


static uint32_t ring_spin(uint32_t frame, NeoPixelFrame *neopixels)
{
    static uint8_t intensity = 255;
    static uint8_t colIndex = 0;

    neopixel col;
    col.colour = 0;
    switch(colIndex)
    {
        case 0:
            col.red = intensity;
            break;
        case 1:
            col.green = intensity;
            break;
        case 2:
            col.blue = intensity;
            break;
    }
    intensity-=4;
    if(intensity == 255)
    {
        colIndex = (colIndex + 1) % 3;
    }
    auto idx = frame % 12;
    for(auto x = 0; x < PIXEL_COUNT; x++)
    {
        if(x == idx)
            neopixels->SetPixel(x, col);
        else
            neopixels->SetPixel(x, 0);
    }

    return 20;
}

uint32_t blurDrops(uint32_t frame, NeoPixelFrame *neopixels)
{
    auto input = neopixels->GetLastBuffer();
    auto output = neopixels->GetBuffer();


    for(auto x = 0; x < PIXEL_COUNT; x++)
    {
        auto left = (x - 1) % PIXEL_COUNT;
        auto right = (x + 1) % PIXEL_COUNT;
        output[x].red = ((int)input[left].red + input[x].red + input[x].red + input[right].red) / 4;
        output[x].green = ((int)input[left].green + input[x].green + input[x].green + input[right].green) / 4;
        output[x].blue = ((int)input[left].blue + input[x].blue + input[x].blue + input[right].blue) / 4;
        /*if(output[x].red < 1)
            output[x].red = 0;
        else
            output[x].red -= 1;
        if(output[x].green < 1)
            output[x].green = 0;
        else
            output[x].green -= 1;
        if(output[x].blue < 1)
            output[x].blue = 0;
        else
            output[x].blue -= 1;*/

        if((rand() & 1023) == 100)
        {
            puts("drip");
            output[x].colour = 0;
            switch(rand() % 3)
            {
                case 0:
                    output[x].red = 127;
                    break;
                case 1:
                    output[x].green = 127;
                    break;
                case 2:
                    output[x].blue = 127;
                    output[x].red = 127;
                    output[x].green = 127;
                    break;
            }
            output[x].red += rand() & 127;
            output[x].green += rand() & 127;
            output[x].blue += rand() & 127;           
        }        
    }



    return 20;
}

int main()
{
    stdio_init_all();

    //sleep_ms(8000);

    puts("Hello, world!");

    NeoPixelBuffer neopixels(DMA_CHANNEL, DMA_IRQ_0, pio0, 0, PIXEL_PIN, PIXEL_COUNT);

    puts("Program Init");

    uint32_t frameCounter = 0;
    while(true)
    {
        auto frameStart = get_absolute_time();
        auto frameData = neopixels.Swap();

        auto frameTime = blurDrops(frameCounter, &frameData);
        sleep_until(delayed_by_ms(frameStart, frameTime));
        frameCounter++;
    }

    return 0;
}
