spi_ss = ss.source_set()
spi_ss.add(when: 'CONFIG_SPI', if_true: files('core.c'))
softmmu_ss.add_all(when: 'CONFIG_SPI', if_true: spi_ss)
