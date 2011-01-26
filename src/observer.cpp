/*
    Copyright (C) 2011  Hervé Fache

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

#include <observer.h>

using namespace htoolbox;

Observee::~Observee() {
  for (std::list<Observer*>::iterator it = _observers.begin();
      it != _observers.end(); ++it) {
    (*it)->_observees.remove(this);
    (*it)->notify();
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
