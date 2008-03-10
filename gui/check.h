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

#ifndef CHECK_H
#define CHECK_H

#include <QtGui/QMessageBox>
#include "hbackup.h"
#include "choose.h"

class CheckMessageBox : public QMessageBox {
  Q_OBJECT
  hbackup::HBackup& _backup;
  ChooseDialog&     _dialog;
public:
  CheckMessageBox(
    hbackup::HBackup& backup,
    ChooseDialog&     dialog) :
      QMessageBox::QMessageBox(), _backup(backup), _dialog(dialog) {}
public Q_SLOTS:
  void check();
};

#endif
