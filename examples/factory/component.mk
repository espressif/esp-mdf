#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCDIRS += espnow event_loop http_parse info_store json openssl socket upgrade
COMPONENT_ADD_INCLUDEDIRS := espnow/include event_loop/include http_parse/include info_store/include json/include openssl/include socket/include upgrade/include
