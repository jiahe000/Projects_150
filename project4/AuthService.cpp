#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens") {
  
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {
    // 1. get argument from http
    WwwFormEncodedDict auth_dict = request->formEncodedBody();
    string username = auth_dict.get("username");
    string password = auth_dict.get("password");

    // check the compliement of argument
    if ((username == "") || (password == "")) {
        throw ClientError::badRequest();
        // response->setStatus(400);
        // return;
    }
    // judege the username is lower
    for (unsigned int i = 0; i < username.size() ; i++) {
        if (!islower(username[i])) {
            throw ClientError::badRequest();
        }
    }

    string auth_token_id = "";
    string user_id = "";
    bool exist_user = false;
    auto user_pair =  m_db->users.find(username);
    if (user_pair != m_db->users.end()) {
        // 2. user account is exist, then login
        // 2.1 verificate the correctionof password
        string true_password = user_pair->second->password;
        if (password != true_password) {
            // 2.2 password is not correct
            response->setStatus(403);
            return;
        } else {
            // 2.3 password is correct,then crate auth token for login
            StringUtils string_util = StringUtils();
            auth_token_id = string_util.createAuthToken();
            m_db->auth_tokens.insert({auth_token_id, user_pair->second});
            exist_user = true;
            user_id = user_pair->second->user_id;
        }

    } else {
        // 3. user account is not exist, create it and login
        StringUtils string_util = StringUtils();
        user_id = string_util.createUserId();
        auth_token_id = string_util.createAuthToken();
        User* user = new User();
        user->username = username;
        user->password = password;
        user->user_id = user_id;
        user->email = string("");
        user->balance = 0;
        //std::pair<string, User*> pair = {username, user};
        m_db->users.insert({username, user});
        m_db->auth_tokens.insert({auth_token_id, user});
    }
    // 4. build response
    Document document;
    Document::AllocatorType& a = document.GetAllocator();
    Value o;
    o.SetObject();
    o.AddMember("auth_token", auth_token_id, a);
    o.AddMember("user_id", user_id, a);
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);
    
    // set the return object
    response->setBody(buffer.GetString() + string("\n"));
    if (exist_user) {
        response->setStatus(200);
    } else {
        response->setStatus(201);
    }
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {
    // delete auth_token for log out
    if (request->hasAuthToken()) {
        string auth_token_id = request->getAuthToken();
        auto pair = m_db->auth_tokens.find(auth_token_id);
        if (pair != m_db->auth_tokens.end()) {
            vector<string> split_string = StringUtils::split(request->getUrl(), '/');
            if (split_string.size() < 2) {
                throw ClientError::badRequest();
            }
            // validate the permission
            if (pair->first != split_string[1]) {
                throw ClientError::forbidden();
            }
            m_db->auth_tokens.erase(pair);
            
            // build response
            response->setStatus(200);
            return;
        } else {
            // the auth_token is error
            throw ClientError::badRequest();
        }
    } else {
        // request without auth_token
        throw ClientError::unauthorized();
    }
}

