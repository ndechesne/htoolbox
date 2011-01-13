/*
     Copyright (C) 2011  Herve Fache

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

#include <observer.h>

using namespace htoolbox;

Observee::~Observee() {
  for (std::list<Observer*>::iterator it = _observers.begin();
      it != _observers.end(); it = _observers.begin()) {
    this->unregisterObserver(*it);
  }
}

void Observee::registerObserver(Observer* observer) {
  _observers.push_back(observer);
  observer->_observees.push_back(this);
  observer->notify();
}

void Observee::unregisterObserver(Observer* observer) {
  _observers.remove(observer);
  observer->_observees.remove(this);
  observer->notify();
}

void Observee::notifyObservers() const {
  for (std::list<Observer*>::const_iterator it = _observers.begin();
      it != _observers.end(); ++it) {
    (*it)->notify();
  }
}
