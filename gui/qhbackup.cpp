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
#include <QtGui/QApplication>
#include "choose.h"
#include "check.h"

int main(int argc, char* argv[]) {
  hbackup::HBackup  backup;
  QApplication      app(argc, argv);
  ChooseDialog      dialog;
  CheckMessageBox   message(backup, dialog);

  QObject::connect(&dialog, SIGNAL(accepted()), &message, SLOT(check()));
  dialog.show();

  return app.exec();
}
