/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>
#include <list>
#include <errno.h>

using namespace std;

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "clients.h"
#include "hbackup.h"

using namespace hbackup;

int hbackup::verbosity(void) {
  return 3;
}

int hbackup::terminating(const char* unused) {
  return 0;
}

int main(void) {
  Client*       client;
  list<Client*> clients;
  Database      db("test_db");
  Filters       filters;

  // Create global filter
  Filter* filter = filters.add("and", "backup");
  filter->add("type", "file", false);
  filter->add("path_end", "~", false);

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("localhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/doesnotexist");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  system("echo path /home/User/test2 >> test_nfs/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("myClient2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("myClient");
  client->addOption("username", "Myself");
  client->addOption("password", "flesyM");
  client->setListfile("C:\\Backup\\Backup.LST");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("otherClient");
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->setListfile("c:/home/backup/Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("Client");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "user");
  client->addOption("iocharset", "utf8");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("Client2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("nocase");
  client->addOption("username", "user");
  client->addOption("password", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("Client3");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  client = new Client("Client4");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->addOption("password", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client("testhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client("testhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client("testhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  system("echo path test1/subdir >> etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client("testhost");
  clients.push_back(client);
  client->setProtocol("file");
  system("sed \"s%expire.*%path test2/subdir%\" etc/localhost.list > etc/localhost.list2");
  client->setListfile("etc/localhost.list2");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    if ((*i)->backup(db, filters, 0) < 0) {
      cerr << "backup failed" << endl;
    }
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  cout << endl << "Test: missing clients" << endl;

  client = new Client("myhost2");
  client->setProtocol("smb");
  system("echo \"path C:\\Test\" > test_cifs/Backup/testhost2.list");
  client->setListfile("C:\\Backup\\testhost2.list");
  db.open_rw();
  client->setMountPoint("test_db/mount");
  client->backup(db, filters, 0);
  db.close();
  delete client;

  client = new Client("myhost3");
  client->setProtocol("nfs");
  system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");
  db.open_rw();
  client->setMountPoint("test_db/mount");
  client->backup(db, filters, 0);
  db.close();
  delete client;

  client = new Client("myhost");
  client->setProtocol("file");
  system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");
  db.open_rw();
  client->setMountPoint("test_db/mount");
  client->backup(db, filters, 0);
  db.close();
  delete client;

  list<string> records;

  cout << endl << "Clients in DB" << endl;
  db.open_ro();
  db.getRecords(records);
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost#' client in DB" << endl;
  db.open_ro();
  db.getRecords(records, "myhost#");
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost#' client under test1 in DB" << endl;
  db.open_ro();
  db.getRecords(records, "myhost#", "test1");
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost#' client under test1/cvs in DB" << endl;
  db.open_ro();
  db.getRecords(records, "myhost#", "test1/cvs");
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost#' client under test1/cvs/diroth in DB"
    << endl;
  db.open_ro();
  db.getRecords(records, "myhost#", "test1/cvs/diroth");
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost#' client under test1/cvs/dirutd in DB"
    << endl;
  db.open_ro();
  db.getRecords(records, "myhost#", "test1/cvs/dirutd");
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  printf("Create list of clients\n");
  client = new Client("myhost");
  clients.push_back(client);
  client->setProtocol("file");
  system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client("myhost2");
  clients.push_back(client);
  client->setProtocol("smb");
  system("echo \"path C:\\Test\" > test_cifs/Backup/testhost2.list");
  client->setListfile("C:\\Backup\\testhost2.list");

  client = new Client("myhost3");
  clients.push_back(client);
  client->setProtocol("nfs");
  system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Create list of clients\n");
  client = new Client("myhost2");
  clients.push_back(client);
  client->setProtocol("smb");
  system("echo \"path C:\\Test\" > test_cifs/Backup/testhost2.list");
  client->setListfile("C:\\Backup\\testhost2.list");

  client = new Client("myhost3");
  clients.push_back(client);
  client->setProtocol("nfs");
  system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Create list of clients\n");
  client = new Client("myhost");
  clients.push_back(client);
  client->setProtocol("file");
  system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client("myhost3");
  clients.push_back(client);
  client->setProtocol("nfs");
  system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Create list of clients\n");
  client = new Client("myhost");
  clients.push_back(client);
  client->setProtocol("file");
  system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client("myhost2");
  clients.push_back(client);
  client->setProtocol("smb");
  system("echo \"path C:\\Test\" > test_cifs/Backup/testhost2.list");
  client->setListfile("C:\\Backup\\testhost2.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Second mount fails\n");
  client = new Client("myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  system("touch test_nfs/test2/fail");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("First mount fails\n");
  client = new Client("myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  system("touch test_nfs/test/fail");
  system("rm -f test_nfs/test2/fail");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Works again\n");
  client = new Client("myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  system("rm -f test_nfs/test/fail");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Test with dual boot client\n");
  client = new Client("myClient");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show();
  }
  db.open_rw();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->setMountPoint("test_db/mount");
    (*i)->backup(db, filters, 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  cout << endl << "Clients in DB" << endl;
  db.open_ro();
  db.getRecords(records);
  db.close();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  return 0;
}
