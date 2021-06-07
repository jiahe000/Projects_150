#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") { }


void TransferService::post(HTTPRequest *request, HTTPResponse *response) {
    // parse the argument of request
    WwwFormEncodedDict deposit_dict = request->formEncodedBody();
    string to = deposit_dict.get("to");
    string amount = deposit_dict.get("amount");
    
    // check the argument
    if (amount == "" || (to == "")) {
        throw ClientError::badRequest();
    }
    // unauthorized
    if (!request->hasAuthToken()) {
        throw ClientError::unauthorized();
    } else {
        auto pair = m_db->auth_tokens.find(request->getAuthToken());
        if (pair == m_db->auth_tokens.end()) {
            throw ClientError::badRequest();
        }
    }

    // cheeck the exist of to_user
    auto pair = m_db->users.find(to);
    if (pair != m_db->users.end()) {
        User* to_user = pair->second;
        User* from_user = getAuthenticatedUser(request);
        // check the balance is more than or equal to amount
        if (from_user->balance >= atoi(amount.c_str())) {
            to_user->balance += atoi(amount.c_str());
            from_user->balance -= atoi(amount.c_str());

            // record the transfer
            Transfer* signle_transfer = new Transfer();
            signle_transfer->amount = atoi(amount.c_str());
            signle_transfer->from = from_user;
            signle_transfer->to = to_user;
            m_db->transfers.push_back(signle_transfer);

            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("balance", from_user->balance, a);

            Value array;
            array.SetArray();
            for (unsigned int i = 0; i < m_db->transfers.size(); i++) {
                if (from_user == (m_db->transfers[i])->from) {
                    // find all deposit of the user
                    Transfer* tmp_transfer = m_db->transfers[i];
                    Value to;
                    to.SetObject();
                    to.AddMember("from", tmp_transfer->from->username, a);
                    to.AddMember("to", tmp_transfer->to->username, a);
                    to.AddMember("amount", tmp_transfer->amount, a);
                    array.PushBack(to, a);
                }
            }
            o.AddMember("transfers", array, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            response->setBody(buffer.GetString() + string("\n"));
            return;
        } else {
            // balance is not enough
            throw ClientError::badRequest();
        }
    } else {
        // to_user does not exist
        throw ClientError::notFound();
    }
}
