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

#ifndef _OBSERVER_H
#define _OBSERVER_H

#include <list>

namespace htoolbox {
  class Observer;

  class Observee {
    std::list<Observer*>  _observers;
  public:
    friend class Observer;
    virtual ~Observee();
    void registerObserver(Observer*);
    void unregisterObserver(Observer*);
    void notifyObservers() const;
    // Remove observer without notification
    void remove(Observer* observer) {
      _observers.remove(observer);
    }

  };

  class Observer {
  protected:
    std::list<Observee*>  _observees;
  public:
    friend class Observee;
    virtual ~Observer() {
      for (std::list<Observee*>::iterator it = _observees.begin();
          it != _observees.end(); ++it) {
        (*it)->_observers.remove(this);
      }
    }
    virtual void notify() = 0;
  };
}

#endif // _OBSERVER_H
