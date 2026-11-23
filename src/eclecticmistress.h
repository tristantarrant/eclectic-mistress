#pragma once

#ifndef EM_ECLECTICMISTRESS_H
#define EM_ECLECTICMISTRESS_H

#include <lv2.h>

#define EMPLUGIN_URI "http://dataforte.stom-pi/plugins/eclecticmistress"

typedef enum
{
  RATE,
  RANGE,
  COLOR,
  MIX,
  MATRIX,
  EFFECTS_OUTPUT,
  EFFECTS_INPUT,
} PortIndex;

#endif // EM_ECLECTICMISTRESS_H
