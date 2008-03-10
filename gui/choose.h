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

#ifndef CHOOSE_H
#define CHOOSE_H

#include <stdlib.h>             // getenv
#include <QtGui/QDialog>
#include "choose-dialog.uih"

class ChooseDialog : public QDialog {
  Q_OBJECT
private:
  Ui_choose   choice;
private Q_SLOTS:
  void setOpenFileName();
public:
  enum Mode {
    none,
    user,
    client,
    server
  };
  ChooseDialog(QWidget *parent = 0) : QDialog(parent) {
    choice.setupUi(this);
    QObject::connect(choice.browse, SIGNAL(clicked()), this,
      SLOT(setOpenFileName()));
  }
  QString getConfig();
  Mode getMode();
};

#endif
