#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
  
}

/**
Fetches the user object for this account. 
You can only fetch the user object for a the account that you authenticated.
parameters: {HTTPRequest} *request, {HTTPResponse} *response
email string, balance int
**/
void AccountService::get(HTTPRequest *request, HTTPResponse *response)
{
    auto user = this->getAuthenticatedUser(request);
    // cout << "get account info" << endl;

    // judege authenticated
    if (user) {
        // judege the permission of user
        vector<string> split_user_string = StringUtils::split(request->getUrl(), '/');
        if (split_user_string.size() < 2) {
            // request url is error
            throw ClientError::badRequest();
        } else {
            if (split_user_string[1] != user->user_id) {
                // current user permisson denied
                throw ClientError::forbidden();
            }
        }
        string email = user->email;
        int balance = user->balance;
        // reponse
        Document document;
        Document::AllocatorType &a = document.GetAllocator();

        Value resData;
        resData.SetObject();
        resData.AddMember("email", email, a);
        resData.AddMember("balance", balance, a);

        document.Swap(resData);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
        return;
    }
    else
    {
        throw ClientError::unauthorized();
        // cout << "no auth" << endl;
        // response->setStatus(401);
        // return;
    }
}

/**
Updates the information for a user.
parameter: {HTTPRequest} *request
email string
parameter: {HTTPResponse} *response
email string
balance int
**/
void AccountService::put(HTTPRequest *request, HTTPResponse *response)
{
    auto user = this->getAuthenticatedUser(request);
    // judege authenticated
    if (user) {
        // judege the permission of user
        vector<string> split_user_string = StringUtils::split(request->getUrl(), '/');
        if (split_user_string.size() < 2) {
            // request url is error
            throw ClientError::badRequest();
        } else {
            if (split_user_string[1] != user->user_id) {
                // current user permisson denied
                throw ClientError::forbidden();
            }
        }
        // get new email
        WwwFormEncodedDict auth_dict = request->formEncodedBody();
        string newEmail = auth_dict.get("email");
        
        // check the argument
        if (newEmail == "") {
            // email can not be empty
            throw ClientError::badRequest();
        }

        // set new email
        user->email = newEmail;

        // get balance
        int balance = user->balance;

        // reponse
        Document document;
        Document::AllocatorType &a = document.GetAllocator();

        Value resData;
        resData.SetObject();
        resData.AddMember("email", newEmail, a);
        resData.AddMember("balance", balance, a);

        document.Swap(resData);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
    }
    else {
        throw ClientError::unauthorized();
        // cout << "no auth" << endl;
        // response->setStatus(401);
        // return;
    }
}
