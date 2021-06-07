#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

#include "Base64.h"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;

void get_balance();
void addEmail(string email);
void auth(string username, string password, string email);
void deposit(string amount_in_dollars, string credit_card_number, string exp_year, string exp_month, string cvc);
void send(string to_username, string amount_in_dollars);
string make_token(string credit_card_number, string exp_year, string exp_month, string cvc);

string make_token(string credit_card_number, string exp_year, string exp_month, string cvc)
{
  // make strpe_token
  // client
  HttpClient client("api.stripe.com", 443, true);
  string value = "Basic " + Base64::bytesToBase64((const unsigned char *)PUBLISHABLE_KEY.c_str(),
                                                  PUBLISHABLE_KEY.size());
  client.set_header("Authorization", value);
  client.set_header("Content-Type", "application/x-www-form-urlencoded");

  // body
  WwwFormEncodedDict body;
  body.set("card[number]", credit_card_number);
  body.set("card[exp_month]", exp_month);
  body.set("card[exp_year]", exp_year);
  body.set("card[cvc]", cvc);
  string encoded_body = body.encode();
  // post
  HTTPClientResponse *client_response = client.post("/v1/tokens", encoded_body);
  Document *d = client_response->jsonBody();
  string token = (*d)["id"].GetString();
  delete d;
  return token;
  // end make strpe_token
}

/**
using username password
parameters: {string} username, {string} password
**/
void auth(string username, string password, string email) {

  for(auto u:username) {
    if(!islower(u)){
      cout << "Error" << endl;
      return;
    }
  }
  HttpClient client(API_SERVER_HOST.c_str(), int(API_SERVER_PORT), false);
  // body
  WwwFormEncodedDict body;
  body.set("username", username);
  body.set("password", password);
  string encoded_body = body.encode();

  // post
  HTTPClientResponse *client_response = client.post("/auth-tokens",
                                                    encoded_body);
  // response
  // check status code
  auto status = client_response->status();
  
  if(status == 200) {
    // auth success
    Document *d = client_response->jsonBody();
    auth_token = (*d)["auth_token"].GetString();
    user_id = (*d)["user_id"].GetString();
    get_balance();
    delete d;
    return;
  }
  else if(status == 201) {
    Document *dd = client_response->jsonBody();
    auth_token = (*dd)["auth_token"].GetString();
    user_id = (*dd)["user_id"].GetString();
    addEmail(email);
    delete dd;
    return;
  } else {
    cout << "Error" << endl;
  }
}

void addEmail(string email) {
  // user id add email to this user
  // client
  HttpClient client(API_SERVER_HOST.c_str(), int(API_SERVER_PORT), false);
  client.set_header("x-auth-token", auth_token);

  // body
  WwwFormEncodedDict body;
  body.set("email", email);
  string encoded_body = body.encode();

  // put
  HTTPClientResponse *client_response = client.put("/users/" + user_id,
                                                   encoded_body);
  // res
  // response
  if (client_response->status() == 200) {
    Document *d = client_response->jsonBody();
    auto balance = (*d)["balance"].GetInt();
    cout << "Balance: $" << fixed << setprecision(2) << float(balance/100) << endl;
    delete d;
  }
  else {
    cout << "Error" << endl;
  }
}


// get balance using auth_token
void get_balance() {

  HttpClient client(API_SERVER_HOST.c_str(), int(API_SERVER_PORT), false);
  // get
  client.set_header("x-auth-token", auth_token);
  HTTPClientResponse *client_response = client.get("/users/" + user_id);
  // response
  if (client_response->status() == 200) {
    Document *d = client_response->jsonBody();
    auto balance = (*d)["balance"].GetInt();
    cout << "Balance: $" << fixed << setprecision(2) << float(balance/100) << endl;
    delete d;
  }
  else {
    cout << "Error" << endl;
  }
}

// get balance using auth_token
void deposit(string amount_in_dollars, string credit_card_number, string exp_year, string exp_month, string cvc) {
  if (auth_token.size() == 0) {
    cout << "Error" << endl;
    return;
  }

  // card length
  if(credit_card_number.size() != 16) {
    cout << "Error" << endl;
    return;
  }
  // make token
  string token = make_token(credit_card_number, exp_year, exp_month, cvc);
  HttpClient client(API_SERVER_HOST.c_str(), int(API_SERVER_PORT), false);
  // header
  client.set_header("x-auth-token", auth_token);
  // body
  WwwFormEncodedDict body;
  body.set("stripe_token", token);
  body.set("amount", stof(amount_in_dollars)*100);
  string encoded_body = body.encode();
  // post
  HTTPClientResponse *client_response = client.post("/deposits",
                                                    encoded_body);
  // response
  if (client_response->status() == 200) {
    Document *d = client_response->jsonBody();
    auto balance = (*d)["balance"].GetInt();
    cout << "Balance: $" << fixed << setprecision(2) << float(balance/100) << endl;
    delete d;
  }
  else {
    cout << "Error" << endl;
  }
}

void send(string to_username, string amount_in_dollars) {
  if (auth_token.size() == 0) {
    cout << "Error" << endl;
    return;
  }
  HttpClient client(API_SERVER_HOST.c_str(), int(API_SERVER_PORT), false);
  // header
  client.set_header("x-auth-token", auth_token);
  // body
  WwwFormEncodedDict body;
  body.set("to", to_username);
  body.set("amount", amount_in_dollars);
  string encoded_body = body.encode();

  // post
  HTTPClientResponse *client_response = client.post("/transfers",
                                                    encoded_body);
  // response
  if (client_response->status() == 200) {
    Document *d = client_response->jsonBody();
    auto balance = (*d)["balance"].GetInt();
    cout << "Balance: $" << fixed << setprecision(2) << float(balance/100) << endl;
    delete d;
  }
  else {
    cout << "Error" << endl;
  }
}
/***
split command to vector string
parameter: {const string} command
***/
vector<string> splitCommand(const string command) {

  vector<string> res;
  if ("" == command)
    return res;

  //convert string to char*
  char *strs = new char[command.length() + 1];
  strcpy(strs, command.c_str());

  char *d = new char[string(" ").length() + 1];
  strcpy(d, string(" ").c_str());

  char *p = strtok(strs, d);
  while (p) {
    string s = p;     //convert it to string
    res.push_back(s); //store into res
    p = strtok(NULL, d);
  }
  return res;
}
int main(int argc, char *argv[]) {
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0) {
    cout << "could not open config.json" << endl;
    exit(1);
  }
  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0) {
    config << string(buffer, ret);
  }
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();
  if (argc == 1) {
    while (true) {
      try {
        cout << "D$>";
        string command;
        getline(std::cin, command);
        auto vComm = splitCommand(command);
        string first = vComm.size() == 0 ? "" : vComm[0];
        if (first == "auth") {
          if (vComm.size() == 4) {
            auth(vComm[1], vComm[2], vComm[3]);
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "balance") {
          if (vComm.size() == 1) {
            get_balance();
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "deposit") {
          if (vComm.size() == 6) {
            deposit(vComm[1], vComm[2], vComm[3], vComm[4], vComm[5]);
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "send") {
          if (vComm.size() == 3) {
            send(vComm[1], vComm[2]);
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "logout") {
          break;
        }
        else {
          // define
          cout << "Error" << endl;
          continue;
        }
      }
      catch (const std::exception &e) {
        cout << "Error" << endl;
        continue;
      }
    }
  }
  else if (argc == 2) {

    string file_name = argv[1];
    ifstream ifs;
    ifs.open(file_name, ios::in);
    string line;

    while (getline(ifs, line)) {
      try {
        auto vComm = splitCommand(line);
        string first = vComm.size() == 0 ? "" : vComm[0];
        if (first == "auth") {
          if (vComm.size() == 4) {
            auth(vComm[1], vComm[2], vComm[3]);
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "balance") {
          if (vComm.size() == 1) {
            get_balance();
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "deposit") {
          if (vComm.size() == 6) {
            deposit(vComm[1], vComm[2], vComm[3], vComm[4], vComm[5]);
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "send") {
          if (vComm.size() == 3) {
            send(vComm[1], vComm[2]);
          }
          else {
            cout << "Error" << endl;
          }
          continue;
        }
        else if (first == "logout") {
          break;
        }
        else {
          // define
          cout << "Error" << endl;
          continue;
        }
      }
      catch (const std::exception &e) {
        cout << "Error" << endl;
        continue;
      }
    }
  }
  return 0;
}
