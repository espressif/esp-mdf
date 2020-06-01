
set(COM_RQUIRES mcommon mwifi mlink mdebug light_driver light_handle mesh_utils aliyun_sdk)

if(CONFIG_LIGHT_EXAMPLE_ALIYUN)
set(COM_SRCS "aliyun_light_example.c")
else()
set(COM_SRCS "light_example.c")
endif()


idf_component_register(SRCS ${COM_SRCS}
                    INCLUDE_DIRS ""
                    REQUIRES ${COM_REQUIRES}
)
