#
# Component Makefile
#

COMPONENT_SRCDIRS := src src/sntp

COMPONENT_ADD_INCLUDEDIRS := src/sntp

COMPONENT_PRIV_INCLUDEDIRS := src

COMPONENT_SUBMODULES := src

dependencies:
  esp-idf: ">=5.3"