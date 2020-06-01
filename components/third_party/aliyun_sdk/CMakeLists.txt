if(CONFIG_ALIYUN_IOT_NEW_SDK)

    set(COMPONENT_EMBED_TXTFILES "certs/aliyun_cert.pem")
    set(COMPONENT_ADD_INCLUDEDIRS "include")

    set(COMPONENT_PRIV_INCLUDEDIRS "coap/include"
        "message/include"
        "mqtt/include"
        "ota/include"
        "platform/include"
        "sign/include")
    set(COMPONENT_SRCS  "aliyun_kv.c"
        "aliyun_subdevice.c"
        "aliyun_gateway.c"
        "platform/aliyun_platform.c"
        "message/aliyun_parse.c"
        "message/aliyun_list.c"
        "message/aliyun_answer.c"
        "message/aliyun_cyclic.c"
        "message/aliyun_pack.c"
        "coap/aliyun_coap.c"
        "coap/aliyun_url.c"
        "sign/aliyun_host.c"
        "sign/aliyun_sign.c"
        "ota/aliyun_ota.c"
        "mqtt/aliyun_mqtt.c"
        "mqtt/aliyun_subscribe.c"
        "mqtt/aliyun_unsubscribe.c"
        "mqtt/aliyun_publish.c"
        "mqtt/aliyun_topic.c")

    if(CONFIG_ALIYUN_PLATFORM_MDF)
        list(APPEND COMPONENT_PRIV_INCLUDEDIRS "platform/mdf")
        list(APPEND COMPONENT_SRCS "platform/mdf/aliyun_mdf.c")
    else()
        list(APPEND COMPONENT_PRIV_INCLUDEDIRS "platform/idf")
        list(APPEND COMPONENT_SRCS "platform/idf/aliyun_idf.c")
    endif()

else()
    message(STATUS "Building empty aliyun_iot component due to configuration")
endif()

set(COMPONENT_PRIV_REQUIRES mcommon mwifi mlink mupgrade)

# requirements can't depend on config
set(COMPONENT_REQUIRES mbedtls app_update nvs_flash esp_http_client json mqtt coap)

idf_component_register(SRCS ${COMPONENT_SRCS}
                    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
                    PRIV_INCLUDE_DIRS ${COMPONENT_PRIV_INCLUDEDIRS}
                    PRIV_REQUIRES ${COMPONENT_PRIV_REQUIRES}
                    REQUIRES ${COMPONENT_REQUIRES}
                    EMBED_TXTFILES ${COMPONENT_EMBED_TXTFILES}
                    )