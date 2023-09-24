#
# Component Makefile
#

COMPONENT_SRCDIRS := src src/bluetooth

COMPONENT_ADD_INCLUDEDIRS := src/bluetooth

COMPONENT_PRIV_INCLUDEDIRS := src

COMPONENT_SUBMODULES := src/bluetooth

dependencies:
  esp-idf: ">=5.0"