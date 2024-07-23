#ifndef RMII_PHY_H
#define RMII_PHY_H

#include "stdlib.h"
#include "string.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"
#include "hardware/dma.h"
#include "hardware/pio.h"

#include "rmii_phy_read.pio.h" 
#include "rmii_phy_write.pio.h" 

#define PREAMBLE_SIZE 8
#define MIN_LENGTH_PACKET 64
#define MAX_LENGTH_PACKET 1518

#define MAX_INTERFACE_NAME 5

typedef struct rmii_phy_config {
    PIO pio; 
    uint sm;
    uint mdc;
    uint mdio;
    uint clk;
    uint rx0;
    uint tx0; 
    uint8_t phy_addr;
    uint8_t* mac_addr; 
    char* phy_name; 
    bool status_link; 
} rmii_phy_config_t;  

uint crs_pin; 
PIO pio;
bool status_crs;

uint8_t packet_recv[MAX_LENGTH_PACKET]; 
uint8_t packet_send[MAX_LENGTH_PACKET]; 

uint recv_sm_offset;
uint send_sm_offset; 

int recv_dma_chan; 
int send_dma_chan; 

dma_channel_config recv_dma_channel_config; 
dma_channel_config send_dma_channel_config; 

static void mdio_clock_write(rmii_phy_config_t* config, int bit);

static uint mdio_clock_read(rmii_phy_config_t* config);

static uint16_t rmii_phy_read_cycle(rmii_phy_config_t* config, uint8_t reg);

static void rmii_phy_write_cycle(rmii_phy_config_t* config, uint8_t reg, uint16_t data);

void rmii_phy_init(rmii_phy_config_t* config);

void receive_packet(rmii_phy_config_t* config);

#endif /* RMII_PHY_H */


