/*
     Copyright (C) 2006-2010  Herve Fache

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
#include <iomanip>
#include <list>

#include <stdio.h>
#include <errno.h>

using namespace std;

#include "test.h"

#include "files.h"
#include "conditions.h"
#include "filters.h"
#include "parsers.h"
#include "opdata.h"
#include "db.h"
#include "attributes.h"
#include "paths.h"
#include "clients.h"

static void progress(long long previous, long long current, long long total) {
  if (current < total) {
    cout << "Done: " << setw(5) << setiosflags(ios::fixed) << setprecision(1)
      << 100.0 * static_cast<double>(current) / static_cast<double>(total)
      << "%\r" << flush;
  } else if (previous != 0) {
    cout << "            \r";
  }
}

int main(void) {
  Attributes attr;
  Client* client;
  list<Client*> clients;
  Database db("test_db");
  int sys_rc;

  report.setLevel(debug);
  db.setProgressCallback(progress);

  // Create global filter
  cout << "Global filter" << endl;
  attr.addFilter("and", "backup");
  attr.addFilterCondition("type", "file", false);
  attr.addFilterCondition("path_end", "~", false);
  attr.show(1);

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "localhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/doesnotexist");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  if (db.open(false, true) < 0) {
    cout << "failed to open DB" << endl;
    return 0;
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("echo path /home/User/test2 >> test_nfs/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "myClient2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("myClient");
  client->addOption("username", "Myself");
  client->addOption("password", "flesyM");
  client->setListfile("C:\\Backup\\Backup2.LST");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "otherClient");
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->addOption("username", "foo");
  client->addOption("password", "barZ?");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "otherClient1");
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->addOption("password", "barZ?");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "otherClient2");
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->addOption("username", "foo");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "otherClient3");
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "Client");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "user");
  client->addOption("iocharset", "utf8");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "Client2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("nocase");
  client->addOption("username", "user");
  client->addOption("password", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "Client3");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client(attr, "Client4");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->addOption("password", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client(attr, "testhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client(attr, "testhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client(attr, "testhost");
  clients.push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  sys_rc = system("echo path test1/subdir >> etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  client = new Client(attr, "testhost");
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("sed \"s%expire.*%path test2/subdir%\" etc/localhost.list > etc/localhost.list2");
  client->setListfile("etc/localhost.list2");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    if ((*i)->backup(db, "test_db/mount", 0) < 0) {
      cerr << "backup failed" << endl;
    }
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  cout << endl << "Test: missing clients" << endl;

  client = new Client(attr, "myhost2");
  client->setProtocol("smb");
  sys_rc = system("echo \"path 'C:\\Test Dir'\" > test_cifs/Backup/testhost2.list");
  client->setListfile("C:\\Backup\\testhost2.list");
  db.open();
  client->backup(db, "test_db/mount", 0);
  db.close();
  delete client;

  client = new Client(attr, "myhost3");
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");
  db.open();
  client->backup(db, "test_db/mount", 0);
  db.close();
  delete client;

  client = new Client(attr, "myhost");
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");
  db.open();
  client->backup(db, "test_db/mount", 0);
  db.close();
  delete client;

  list<string> records;
  db.open(true);

  cout << endl << "Clients in DB" << endl;
  db.getClients(records);
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost' client in DB" << endl;
  db.openClient("myhost", false);
  db.getRecords(records);
  db.closeClient();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost' client under test1 in DB" << endl;
  db.openClient("myhost", false);
  db.getRecords(records, "test1");
  db.closeClient();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost' client under test1/cvs/ in DB" << endl;
  db.openClient("myhost", false);
  db.getRecords(records, "test1/cvs/");
  db.closeClient();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost' client under test1/cvs/diroth in DB"
    << endl;
  db.openClient("myhost", false);
  db.getRecords(records, "test1/cvs/diroth");
  db.closeClient();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'myhost' client under test1/cvs/dirutd in DB"
    << endl;
  db.openClient("myhost", false);
  db.getRecords(records, "test1/cvs/dirutd");
  db.closeClient();
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  cout << endl << "Paths in 'not_here' client" << endl;
  if (db.openClient("not_here", false) == 0) {
    db.getRecords(records);
    db.closeClient();
    cout << "Records found: " << records.size() << endl;
    for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
      cout << " " << *i << endl;
    }
    records.clear();
  }

  db.close();

  printf("Create list of clients\n");
  client = new Client(attr, "myhost");
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client(attr, "myhost2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  client = new Client(attr, "myhost3");
  clients.push_back(client);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Create list of clients\n");
  client = new Client(attr, "myhost2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  client = new Client(attr, "myhost3");
  clients.push_back(client);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Create list of clients\n");
  client = new Client(attr, "myhost");
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client(attr, "myhost3");
  clients.push_back(client);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Create list of clients\n");
  client = new Client(attr, "myhost");
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client(attr, "myhost2");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Second mount fails\n");
  client = new Client(attr, "myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("touch test_nfs/test2/fail");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("First mount fails\n");
  client = new Client(attr, "myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("touch test_nfs/test/fail");
  sys_rc = system("rm -f test_nfs/test2/fail");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Works again\n");
  client = new Client(attr, "myClient");
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("rm -f test_nfs/test/fail");
  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  printf("Test with dual boot client\n");
  client = new Client(attr, "myClient", "xp");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\Backup.LST");

  printf(">List %u client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  db.open();
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->backup(db, "test_db/mount", 0);
  }
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    delete *i;
  }
  clients.clear();
  db.close();

  cout << endl << "Clients in DB" << endl;
  db.getClients(records);
  cout << "Records found: " << records.size() << endl;
  for (list<string>::iterator i = records.begin(); i != records.end(); i++) {
    cout << " " << *i << endl;
  }
  records.clear();

  return 0;
}
