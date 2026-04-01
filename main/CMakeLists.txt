idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES i2c_handle
                    REQUIRES sps30
                    REQUIRES ms5611
                    REQUIRES ze27o3
                    REQUIRES sdCard
                    REQUIRES driver
                    )
