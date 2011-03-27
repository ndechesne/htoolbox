/*
    Copyright (C) 2008 - 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "criticality.h"

using namespace htoolbox;

int main(void) {
  Criticality crit;
  
  printf("String conversions\n");
  const char* str;
  str = "alert";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "AlErT";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "error";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "ErRoR";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "warning";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "WaRnInG";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "info";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "InFo";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "verbose";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "VeRbOsE";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "debug";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "DeBuG";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "regression";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "ReGrEsSiOn";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "bidon";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }
  str = "Alest";
  if (crit.setFromString(str) == 0) {
    printf("criticality set from '%s' to '%s'\n", crit.toString(), str);
  } else {
    printf("failed to set criticality from '%s'\n", str);
  }

  printf("Comparisons\n");
  for (int l = alert; l <= regression; ++l) {
    crit.setFromInt(l);
    Criticality crit2;
    for (int r = alert; r <= regression; ++r) {
      crit2.setFromInt(r);
      if (crit < crit2) {
        printf("%s < %s\n", crit.toString(), crit2.toString());
      } else {
        printf("not %s < %s\n", crit.toString(), crit2.toString());
      }
      if (crit > crit2) {
        printf("%s > %s\n", crit.toString(), crit2.toString());
      } else {
        printf("not %s > %s\n", crit.toString(), crit2.toString());
      }
      if (crit <= crit2) {
        printf("%s <= %s\n", crit.toString(), crit2.toString());
      } else {
        printf("not %s <= %s\n", crit.toString(), crit2.toString());
      }
      if (crit >= crit2) {
        printf("%s >= %s\n", crit.toString(), crit2.toString());
      } else {
        printf("not %s >= %s\n", crit.toString(), crit2.toString());
      }
      if (crit == crit2) {
        printf("%s == %s\n", crit.toString(), crit2.toString());
      } else {
        printf("not %s == %s\n", crit.toString(), crit2.toString());
      }
      if (crit != crit2) {
        printf("%s != %s\n", crit.toString(), crit2.toString());
      } else {
        printf("not %s != %s\n", crit.toString(), crit2.toString());
      }
    }
  }

  return 0;
}
