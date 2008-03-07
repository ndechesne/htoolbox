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
#include <stdlib.h> // getenv
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include "qhbackup.h"

void ChooseDialog::setOpenFileName() {
  QFileDialog::Options options;
  QString selectedFilter;
  QString fileName = QFileDialog::getOpenFileName(this,
                          tr("Choose configuration file"),
                          choice.configpath->text(),
                          tr("Conf Files (*.conf)"));
  if (!fileName.isEmpty()) {
    choice.configpath->setText(fileName);
  }
}

QString ChooseDialog::getConfig() {
  if (choice.servermode->isChecked())
    return choice.configpath->text();
  QString path = getenv("HOME");
  path += "/.hbackup/config";
  return path;
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  ChooseDialog dialog;
  ViewPath message(dialog);

  dialog.show();
  QObject::connect(&dialog, SIGNAL(accepted()), &message, SLOT(show()));

  return app.exec();
}
