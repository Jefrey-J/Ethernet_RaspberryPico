#include "rmii_phy.h"

static void mdio_clock_write(rmii_phy_config_t* config, int bit) {
    gpio_put(config->mdc, 0); 
    sleep_us(1);
    gpio_put(config->mdio, bit); 
    gpio_put(config->mdc, 1);
    sleep_us(1);
}

static uint mdio_clock_read(rmii_phy_config_t* config) {
    gpio_put(config->mdc, 0); 
    sleep_us(1);
    gpio_put(config->mdc, 1); 

    uint bit = gpio_get(config->mdio); 
    sleep_us(1); 

    return bit; 
}

static uint16_t rmii_phy_read_cycle(rmii_phy_config_t* config, uint8_t reg) {

    gpio_init(config->mdio);
    gpio_init(config->mdc);

    gpio_set_dir(config->mdio, GPIO_OUT);     
    gpio_set_dir(config->mdc, GPIO_OUT);

    // ######## Starting Cycle ########

    // Preamble 32 1's 
    for(int b = 0; b < 32; b++) {
        mdio_clock_write(config, 1); 
    }
    
    // Start of Frame 
    mdio_clock_write(config, 0); 
    mdio_clock_write(config, 1); 

    // OP Code
    mdio_clock_write(config, 1); 
    mdio_clock_write(config, 0); 

    // PHY Address 
    for(int b = 0; b < 5; b++) {
        // |8|7|6| 5| 3| 2| 1| 0|
        // |x|x|x|a4|a3|a2|a1|a0|
        mdio_clock_write(config, (config->phy_addr >> (4 - b)) & 0x01); 
    }

    // Register Address 
    for(int b = 0; b < 5; b++) {
        // |8|7|6| 5| 3| 2| 1| 0|
        // |x|x|x|r4|r3|r2|r1|r0|
        mdio_clock_write(config, (reg >> (4 - b)) & 0x01); 
    }

    // Turn Around 
    gpio_set_dir(config->mdio, GPIO_IN); 
    mdio_clock_write(config, 0);
    mdio_clock_write(config, 0); 

    // Data 
    uint16_t data = 0; 
    for(int b = 0; b < 16; b++) {
        data = data << 1; 
        data |= mdio_clock_read(config);
    }

    return data; 
}

static void rmii_phy_write_cycle(rmii_phy_config_t* config, uint8_t reg, uint16_t data) {

    gpio_init(config->mdc);
    gpio_init(config->mdio);

    gpio_set_dir(config->mdc, GPIO_OUT);
    gpio_set_dir(config->mdio, GPIO_OUT); 

    // ######## Starting Cycle ########

    // Preamble 32 1's 
    for(int b = 0; b < 32; b++) {
        mdio_clock_write(config, 1); 
    }
    
    // Start of Frame 
    mdio_clock_write(config, 0); 
    mdio_clock_write(config, 1); 

    // OP Code
    mdio_clock_write(config, 0); 
    mdio_clock_write(config, 1); 

    // PHY Address 
    for(int b = 0; b < 5; b++) {
        // |8|7|6| 5| 3| 2| 1| 0|
        // |x|x|x|a4|a3|a2|a1|a0|
        mdio_clock_write(config, (config->phy_addr >> (4 - b)) & 0x01); 
    }

    // Register Address 
    for(int b = 0; b < 5; b++) {
        // |8|7|6| 5| 3| 2| 1| 0|
        // |x|x|x|r4|r3|r2|r1|r0|
        mdio_clock_write(config, (reg >> (4 - b)) & 0x01); 
    }

    // Turn Around 
    mdio_clock_write(config, 1);
    mdio_clock_write(config, 0); 

    // Data 
    for(int b = 0; b < 16; b++) {
        mdio_clock_write(config, (data >> (15 - b)) & 0x01);
    }

    gpio_set_dir(config->mdio, GPIO_IN); 
}

static void crs_callback_fall(uint gpio, uint32_t events) {
    if(crs_pin == gpio) {
        pio_sm_set_enabled(pio, crs_pin, false);
        dma_channel_abort(recv_dma_chan);
        gpio_set_irq_enabled_with_callback(crs_pin, GPIO_IRQ_EDGE_FALL, false, crs_callback_fall);
    }
}

void rmii_phy_init(rmii_phy_config_t* config) {
    crs_pin = 8;
    pio = pio0;
    /*rmii_phy_config_t config = {
        pio0, // pio0
        27,   // MDC pin 
        28,   // MDIO pin 
        26,   // CLOCK pin
        20,   // RX0, RX1, CRS
        17,   // TX0, TX1, TX-EN
        0,    // PHY ADDRESS
        NULL, // MAC ADDRESS
        NULL  // INTERFACE NAME 
    };*/

    clock_configure_gpin(clk_sys, 20, 50 * MHZ, 50 * MHZ);
    sleep_ms(100);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    pico_unique_board_id_t board_id; 
    pico_get_unique_board_id(&board_id);

    uint8_t mac_addr[6] = {0xb8, 0x27, 0xeb, board_id.id[5], board_id.id[6], board_id.id[7]};

    config->mac_addr = (uint8_t*)calloc(6, sizeof(uint8_t)); 
    memcpy(config->mac_addr, &mac_addr[0], 6);

    config->phy_name = (char*)calloc(MAX_INTERFACE_NAME, sizeof(char));
    strcpy(config->phy_name, "eth01");
    config->phy_name[MAX_INTERFACE_NAME] = '\0';

    recv_sm_offset = pio_add_program(config->pio, &rmii_phy_read_data_program);
    send_sm_offset = pio_add_program(config->pio, &rmii_phy_read_data_program);

    recv_dma_chan = dma_claim_unused_channel(true); 
    send_dma_chan = dma_claim_unused_channel(true); 

    recv_dma_channel_config = dma_channel_get_default_config(recv_dma_chan);

    channel_config_set_read_increment(&recv_dma_channel_config, false); 
    channel_config_set_write_increment(&recv_dma_channel_config, true); 
    channel_config_set_dreq(&recv_dma_channel_config, pio_get_dreq(config->pio, config->rx0, false)); 
    channel_config_set_transfer_data_size(&recv_dma_channel_config, DMA_SIZE_8); 

    send_dma_channel_config = dma_channel_get_default_config(send_dma_chan);

    channel_config_set_read_increment(&send_dma_channel_config, true); 
    channel_config_set_write_increment(&send_dma_channel_config, false); 
    channel_config_set_dreq(&send_dma_channel_config, pio_get_dreq(config->pio, config->tx0, true)); 
    channel_config_set_transfer_data_size(&send_dma_channel_config, DMA_SIZE_8); 

    rmii_phy_read_init(config->pio, recv_sm_offset, recv_sm_offset, config->rx0);
    rmii_phy_write_init(config->pio, send_sm_offset, send_sm_offset, config->tx0);

    for(uint8_t i = 0; i < 32; i++) {
        config->phy_addr = i; 
        if(rmii_phy_read_cycle(config, 0) != 0xFFFF) {
            break; 
        } else {
            config->phy_addr = 0;
        }
    }

    rmii_phy_write_cycle(config, 4, 0x61);   // 10BASE-T Full Duplex | 10BASE-T | IEEE 802.3 
    rmii_phy_write_cycle(config, 0, 0x1000); // Auto-Negotiation Enable - Basic Status Register   
}

void receive_packet(rmii_phy_config_t* config) {

    uint16_t bsr = rmii_phy_read_cycle(config, 1); 
    config->status_link = (((bsr & 0x04) >> 0x02) ? true : false);  
    
    if(config->status_link) {
        printf("Link up!\n"); 
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
    } else {
        printf("Link down!\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
    }

    rmii_phy_read_init(config->pio, config->sm, recv_sm_offset, config->rx0);
    gpio_set_irq_enabled_with_callback(crs_pin, GPIO_IRQ_EDGE_FALL, true, &crs_callback_fall);
}