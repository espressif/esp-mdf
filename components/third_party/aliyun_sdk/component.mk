
ifdef CONFIG_ALIYUN_IOT_NEW_SDK

# Enable aliyun sdk support
COMPONENT_EMBED_TXTFILES := certs/aliyun_cert.pem
COMPONENT_PRIV_INCLUDEDIRS := coap/include message/include mqtt/include ota/include platform/include sign/include

COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_SRCDIRS := . coap message mqtt ota platform sign

ifdef CONFIG_ALIYUN_PLATFORM_MDF
COMPONENT_PRIV_INCLUDEDIRS += platform/mdf
COMPONENT_SRCDIRS += platform/mdf
else
COMPONENT_PRIV_INCLUDEDIRS += platform/idf
COMPONENT_SRCDIRS += platform/idf
endif

else

# Disable aliyun sdk support
COMPONENT_ADD_INCLUDEDIRS :=
COMPONENT_ADD_LDFLAGS :=
COMPONENT_SRCDIRS :=
endif
