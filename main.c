#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"
#include "rmii_phy/rmii_phy.h"

int main() {
    stdio_init_all();
    rmii_phy_config_t config = {
        pio0, // pio0
        0,    // SM 
        11,   // MDC pin 
        10,   // MDIO pin 
        20,   // CLOCK pin
        6,   // RX0, RX1, CRS
        2,   // TX0, TX1, TX-EN
        0,    // PHY ADDRESS
        NULL, // MAC ADDRESS
        NULL, // INTERFACE NAME 
        false // STATUS LINK 
    };  
    rmii_phy_init(&config);

    while (true) {
        receive_packet(&config);
    }
}
