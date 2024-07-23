add_library(rmii_phy STATIC
    rmii_phy.c
)

# Gerar cabeçalhos PIO
pico_generate_pio_header(rmii_phy ${CMAKE_CURRENT_LIST_DIR}/rmii_phy_read.pio)
pico_generate_pio_header(rmii_phy ${CMAKE_CURRENT_LIST_DIR}/rmii_phy_write.pio)

# Incluir diretório para o compilador buscar os headers
target_include_directories(rmii_phy PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Linkar bibliotecas necessárias
target_link_libraries(rmii_phy PRIVATE
    pico_stdlib
    pico_multicore
    hardware_pio
    pico_unique_id
    hardware_dma
)
