idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES i2c_handle
                    REQUIRES spi_handle
                    REQUIRES sps30
                    REQUIRES ms5611
                    REQUIRES ze27o3
                    REQUIRES sdCard
                    REQUIRES max31856
                    REQUIRES driver
                    )
