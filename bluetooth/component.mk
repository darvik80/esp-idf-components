#
# Component Makefile
#

COMPONENT_SRCDIRS := src src/bluetooth src/beacon

COMPONENT_ADD_INCLUDEDIRS := src/bluetooth src/beacon

COMPONENT_PRIV_INCLUDEDIRS := src

COMPONENT_SUBMODULES := src/bluetooth src/beacon

dependencies:
  esp-idf: ">=5.0"