/*
     Copyright (C) 2008  Herve Fache

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

#include "hbackup.h"
#include "choose.h"
#include <check.h>

void CheckMessageBox::check() {
  QString config = _dialog.getConfig();
  bool    failed = false;
  switch (_dialog.getMode()) {
    case ChooseDialog::user:
      failed = _backup.open(config.toAscii().constData(), true, true);
      break;
    case ChooseDialog::client:
      failed = true;
      break;
    case ChooseDialog::server:
      failed = _backup.open(config.toAscii().constData(), false, true);
      break;
    default:
      failed = true;
  }
  if (failed) {
    QMessageBox::critical(this, tr("Error reading configuration file"),
      tr("Get message from library!"));
  } else {
    setText(config);
    QMessageBox::show();
  }
}
