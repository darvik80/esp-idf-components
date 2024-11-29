#
# Component Makefile
#

COMPONENT_SRCDIRS := src src/led

COMPONENT_ADD_INCLUDEDIRS := src/led

COMPONENT_PRIV_INCLUDEDIRS := src

COMPONENT_SUBMODULES := src

dependencies:
  esp-idf: ">=5.3"