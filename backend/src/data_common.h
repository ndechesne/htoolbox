/*
     Copyright (C) 2010  Herve Fache

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

#ifndef _DATA_COMMON_H
#define _DATA_COMMON_H

namespace hbackend {

enum Tags {
  METHOD            = 1,
  STATUS            = 2,
  HASH              = 11,
  PATH              = 12,
  EXTENSION         = 13,
  COMPRESSION_LEVEL = 14,
  COMPRESSION_CASE  = 15,
  STORE_PATH        = 16,
  THOROUGH          = 17,
  REPAIR            = 18,
  COLLECTOR         = 20,
  COLLECTOR_HASH    = 21,
  COLLECTOR_DATA    = 22,
  COLLECTOR_FILE    = 23,
};

enum Methods {
  NAME     = 1,
  READ     = 2,
  WRITE    = 3,
  REMOVE   = 4,
  CRAWL    = 5,
  PROGRESS = 6,
};

}

#endif // _DATA_COMMON_H
