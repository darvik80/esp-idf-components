#
# Component Makefile
#

COMPONENT_SRCDIRS := src src/core

COMPONENT_ADD_INCLUDEDIRS := src/core

COMPONENT_PRIV_INCLUDEDIRS := src

COMPONENT_SUBMODULES := src/core

dependencies:
  esp-idf: ">=5.1"