# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
idf_component_register(SRCS "thermometer_direct_main.c" "time_sync.c"
                    INCLUDE_DIRS "include")
