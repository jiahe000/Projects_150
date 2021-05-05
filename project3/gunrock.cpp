#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <deque>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HttpService.h"
#include "HttpUtils.h"
#include "FileService.h"
#include "MySocket.h"
#include "MyServerSocket.h"
#include "dthread.h"

using namespace std;

int PORT = 8080;
int THREAD_POOL_SIZE = 1;
int BUFFER_SIZE = 1;
string BASEDIR = "static";
string SCHEDALG = "FIFO";
string LOGFILE = "/dev/null";

vector<HttpService *> services;

deque<MySocket *> clients;
pthread_cond_t  notfull_cond  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  notempty_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

HttpService *find_service(HTTPRequest *request) {
   // find a service that is registered for this path prefix
  for (unsigned int idx = 0; idx < services.size(); idx++) {
    if (request->getPath().find(services[idx]->pathPrefix()) == 0) {
      return services[idx];
    }
  }

  return NULL;
}


void invoke_service_method(HttpService *service, HTTPRequest *request, HTTPResponse *response) {
  stringstream payload;

  // invoke the service if we found one
  if (service == NULL) {
    // not found status
    response->setStatus(404);
  } else if (request->isHead()) {
    payload << "HEAD " << request->getPath();
    sync_print("invoke_service_method", payload.str());
    cout << payload.str() << endl;
    service->head(request, response);
  } else if (request->isGet()) {
    payload << "GET " << request->getPath();
    sync_print("invoke_service_method", payload.str());
    cout << payload.str() << endl;
    service->get(request, response);
  } else {
    // not implemented status
    response->setStatus(405);
  }
}

void handle_request(MySocket *client) {
  HTTPRequest *request = new HTTPRequest(client, PORT);
  HTTPResponse *response = new HTTPResponse();
  stringstream payload;

  // read in the request
  bool readResult = false;
  try {
    payload << "client: " << (void *) client;
    sync_print("read_request_enter", payload.str());
    readResult = request->readRequest();
    sync_print("read_request_return", payload.str());
  } catch (...) {
    // swallow it
  }

  if (!readResult) {
    // there was a problem reading in the request, bail
    delete response;
    delete request;
    sync_print("read_request_error", payload.str());
    return;
  }

  HttpService *service = find_service(request);
  invoke_service_method(service, request, response);

  // send data back to the client and clean up
  payload.str(""); payload.clear();
  payload << " RESPONSE " << response->getStatus() << " client: " << (void *) client;
  sync_print("write_response", payload.str());
  cout << payload.str() << endl;
  client->write(response->response());
  // sleep(5);
  delete response;
  delete request;

  payload.str(""); payload.clear();
  payload << " client: " << (void *) client;
  sync_print("close_connection", payload.str());
  client->close();
  delete client;
}

void* handle_client(void *args)
{
  while(true)
  {
    sync_print("handle_client", "");
    dthread_mutex_lock(&queue_lock);
    while(clients.empty())
    {
      // if empty wait for notify
      dthread_cond_wait(&notempty_cond, &queue_lock);
    }
    MySocket *client = clients.front();
    clients.pop_front();
    dthread_mutex_unlock(&queue_lock);
    handle_request(client);
    dthread_cond_signal(&notfull_cond);
  }
}

void create_thread_pool(int thread_size)
{
  // create thread pool
  for (int i = 0; i < thread_size; i++)
  {
    pthread_t id;
    dthread_create(&id, NULL, handle_client, NULL);
  }
}

void check_base_dir(std::string &basedir)
{
  // std::cout << "basedir: " << basedir << std::endl;
  if (basedir.find("..") != basedir.npos)
  {
    sync_print("check_base_dir", "dir find error");
    basedir = "static";
  }
}

int main(int argc, char *argv[]) {

  signal(SIGPIPE, SIG_IGN);
  int option;

  while ((option = getopt(argc, argv, "d:p:t:b:s:l:")) != -1) {
    switch (option) {
    case 'd':
      BASEDIR = string(optarg);
      break;
    case 'p':
      PORT = atoi(optarg);
      break;
    case 't':
      THREAD_POOL_SIZE = atoi(optarg);
      break;
    case 'b':
      BUFFER_SIZE = atoi(optarg);
      break;
    case 's':
      SCHEDALG = string(optarg);
      break;
    case 'l':
      LOGFILE = string(optarg);
      break;
    default:
      cerr<< "usage: " << argv[0] << " [-p port] [-t threads] [-b buffers]" << endl;
      exit(1);
    }
  }

  set_log_file(LOGFILE);

  sync_print("init", "");

  // create thread pool
  create_thread_pool(THREAD_POOL_SIZE);

  MyServerSocket *server = new MyServerSocket(PORT);
  MySocket *client;

  // chdir("/");
  if (chroot(BASEDIR.c_str()) != 0) 
  {
    // std::cout << "BASEDIR: " << BASEDIR << std::endl;
    exit(1);
  }
  // chdir("/");

  // check_base_dir(BASEDIR);
  std::cout << "BASEDIR: " << BASEDIR << std::endl;

  services.push_back(new FileService(BASEDIR));

  while(true) {
    sync_print("waiting_to_accept", "");
    client = server->accept();
    sync_print("client_accepted", "");

    // lock queue
    dthread_mutex_lock(&queue_lock);
    
    while(clients.size() == (unsigned int)BUFFER_SIZE)
    {
      dthread_cond_wait(&notfull_cond, &queue_lock);
    }

    // if not over buffer add it to queue
    if (clients.size() < (unsigned int)BUFFER_SIZE)
      clients.push_back(client);

    // notify work thread handle request
    dthread_cond_signal(&notempty_cond);

    // unlock queue
    dthread_mutex_unlock(&queue_lock);
  }
}
