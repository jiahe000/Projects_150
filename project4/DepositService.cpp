#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "Base64.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") { }

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {
    // parse argument from request of deposit
    WwwFormEncodedDict deposit_dict = request->formEncodedBody();
    string amount = deposit_dict.get("amount");
    string stripe_token = deposit_dict.get("stripe_token");
    int amount_int = atoi(amount.c_str());
    // check the argument
    if (amount == "" || (stripe_token == "") || (amount_int < 50) ) {
        cerr << "less args" << endl;
        throw ClientError::badRequest();
    }
    // unauthorized
    if (!request->hasAuthToken()) {
        cerr << "no auth" << endl;
        throw ClientError::unauthorized();
    } else {
        auto pair = m_db->auth_tokens.find(request->getAuthToken());
        if (pair == m_db->auth_tokens.end()) {
            cerr << "auth  error" << endl;
            throw ClientError::badRequest();
        }
    }

    // build client request to Stripe
    WwwFormEncodedDict body;
    body.set("amount", amount);
    body.set("source", stripe_token);
    body.set("currency", "usd");
    string encoded_body = body.encode();
    HttpClient client("api.stripe.com", 443, true);
    string value = "Basic " + Base64::bytesToBase64((const unsigned char *) m_db->stripe_secret_key.c_str(),
						  m_db->stripe_secret_key.size());
    client.set_header("Authorization",value);
    HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);

    // judge the error code from stripe server
    if (client_response->status() >= 400) {
        return response->setStatus(client_response->status());
    }
    // convert the HTTP body into a rapidjson document
    Document *d = client_response->jsonBody();
    if ((*d)["status"] == "succeeded") {
        // add deposit
        Deposit* single_deposit = new Deposit();
        User* user =  getAuthenticatedUser(request);
        string charge_id = (*d)["id"].GetString();
        single_deposit->to = user;
        single_deposit->amount = amount_int;
        single_deposit->stripe_charge_id = charge_id;
        m_db->deposits.push_back(single_deposit);

        // update balance
        user->balance += amount_int; 

        Document document;
        Document::AllocatorType& a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("balance", user->balance, a);

        Value array;
        array.SetArray();
        for (unsigned int i = 0; i < m_db->deposits.size(); i++) {
            if (user == (m_db->deposits[i])->to) {
                // find the user all deposit
                Deposit* tmp_deposit = m_db->deposits[i];
                Value to;
                to.SetObject();
                to.AddMember("to", tmp_deposit->to->username, a);
                to.AddMember("amount", tmp_deposit->amount, a);
                to.AddMember("stripe_charge_id", tmp_deposit->stripe_charge_id, a);
                array.PushBack(to, a);
            }
        }
        o.AddMember("deposits", array, a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setBody(buffer.GetString() + string("\n"));
    } else {
        response->setStatus(client_response->status());
    }
    delete d;
    return;
}
