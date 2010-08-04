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
#include "db.h"
#include "attributes.h"
#include "paths.h"
#include "clients.h"

static void copy_progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("Copy: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

static void list_progress(long long previous, long long current, long long total) {
  if ((current != total) || (previous != 0)) {
    hlog_verbose_temp("List: %5.1lf%%",
      100 * static_cast<double>(current) / static_cast<double>(total));
  }
}

int main(void) {
  Client* client;
  list<Client*> clients;
  Database db("test_db");
  int sys_rc;

  report.setLevel(debug);
  db.setCopyProgressCallback(copy_progress);
  db.setListProgressCallback(list_progress);

  // Create global filter
  cout << "Global filter" << endl;
  Attributes attributes;
  vector<string> params;
  params.push_back("filter");
  params.push_back("and");
  params.push_back("backup");
  attributes.addFilter(params);
  params[0] = "condition";
  params[1] = "type";
  params[2] = "file";
  attributes.addFilterCondition(params);
  params[1] = "path_end";
  params[2] = "~";
  attributes.addFilterCondition(params);
  attributes.show(1);

  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("localhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/doesnotexist");
  printf(">List %zu client(s):\n", clients.size());
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

  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("myClient", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("echo path /home/User/test2 >> test_nfs/hbackup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("myClient2", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("myClient");
  client->addOption("username", "Myself");
  client->addOption("password", "flesyM");
  client->setListfile("C:\\Backup\\Backup2.LST");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("otherClient", attributes);
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->addOption("username", "foo");
  client->addOption("password", "barZ?");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("otherClient1", attributes);
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->addOption("password", "barZ?");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("otherClient2", attributes);
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->addOption("username", "foo");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("otherClient3", attributes);
  clients.push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->setListfile("/home/Blah/hbackup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("Client", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "user");
  client->addOption("iocharset", "utf8");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("Client2", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("nocase");
  client->addOption("username", "user");
  client->addOption("password", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("Client3", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %zu client(s):\n", clients.size());
  for (list<Client*>::iterator i = clients.begin(); i != clients.end(); i++) {
    (*i)->show(1);
  }
  client = new Client("Client4", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->addOption("password", "");
  client->setListfile("c:\\home\\BlaH\\Backup.list");
  printf(">List %zu client(s):\n", clients.size());
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

  client = new Client("testhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/localhost.list");
  printf(">List %zu client(s):\n", clients.size());
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

  client = new Client("testhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  printf(">List %zu client(s):\n", clients.size());
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

  client = new Client("testhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  sys_rc = system("echo path test1/subdir >> etc/localhost.list");
  printf(">List %zu client(s):\n", clients.size());
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

  client = new Client("testhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("sed \"s%expire.*%path test2/subdir%\" etc/localhost.list > etc/localhost.list2");
  client->setListfile("etc/localhost.list2");
  printf(">List %zu client(s):\n", clients.size());
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

  client = new Client("myhost2", attributes);
  client->setProtocol("smb");
  sys_rc = system("echo \"path 'C:\\Test Dir'\" > test_cifs/Backup/testhost2.list");
  client->setListfile("C:\\Backup\\testhost2.list");
  db.open();
  client->backup(db, "test_db/mount", 0);
  db.close();
  delete client;

  client = new Client("myhost3", attributes);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");
  db.open();
  client->backup(db, "test_db/mount", 0);
  db.close();
  delete client;

  client = new Client("myhost", attributes);
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
  client = new Client("myhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client("myhost2", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  client = new Client("myhost3", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myhost2", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  client = new Client("myhost3", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client("myhost3", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  sys_rc = system("echo path /home/User/test > test_nfs/testhost3.list");
  client->setListfile("/home/User/testhost3.list");

  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myhost", attributes);
  clients.push_back(client);
  client->setProtocol("file");
  sys_rc = system("echo path test1/cvs > etc/testhost.list");
  client->setListfile("etc/testhost.list");

  client = new Client("myhost2", attributes);
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\testhost2.list");

  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myClient", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("touch test_nfs/test2/fail");
  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myClient", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("touch test_nfs/test/fail");
  sys_rc = system("rm -f test_nfs/test2/fail");
  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myClient", attributes);
  clients.push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  sys_rc = system("rm -f test_nfs/test/fail");
  printf(">List %zu client(s):\n", clients.size());
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
  client = new Client("myClient", attributes, "xp");
  clients.push_back(client);
  client->setProtocol("smb");
  client->setListfile("C:\\Backup\\Backup.LST");

  printf(">List %zu client(s):\n", clients.size());
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
