#include <iostream>
#include <td/telegram/td_json_client.h>
#include <json.hpp>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <sys/mman.h>
#include <thread>

// std::unordered_set<int>* chats;

using json = nlohmann::json;

typedef struct Chat {
  long long id;
  std::string title;
  std::vector<std::string> messages;
} Chat;

std::vector<std::string> ignoredUpdates = {
    "updateUser"
  , "updateChatPinnedMessage"
  , "updateSupergroupFullInfo"
  , "updateOption"
  , "updateChatLastMessage"
  , "updateSupergroup"
  , "updateBasicGroup"
  , "updateNewChat"
  , "updateSelectedBackground"
  , "updateScopeNotificationSettings"
  , "updateChatReadInbox"
  , "updateChatReadOutbox"
  , "updateChatNotificationSettings"
  , "updateNewMessage"
  , "updateUnreadMessageCount"
  , "updateBasicGroupFullInfo"
  , "updateDeleteMessages"
  , "updateMessageContent"
  , "updateMessageEdited"
  , "updateFile"
  , "remoteFile"
  , "updateUnreadChatCount"
  , "updateHavePendingNotifications"
  , "updateUserStatus"
  , "updateMessageViews"
};

std::unordered_map<long long, Chat> *chats;

void handleAuthentication (void *client, json authenticationState) {
  if (authenticationState["@type"] == "authorizationStateWaitTdlibParameters") {
    json params = {
      {"@type", "setTdlibParameters"},
      {"parameters", {
        {"database_directory", "tdlib"},
        {"use_message_database", true},
        {"api_id", YOUR ID},
        {"api_hash", YOUR HASH},
        {"system_language_code", "en"},
        {"device_model", "Desktop"},
        {"system_version", "Linux"},
        {"application_version", "0.1"},
        {"enable_storage_optimizer", true}
      }}
    };

    td_json_client_send(client, params.dump().c_str());
  }

  if (authenticationState["@type"] == "authorizationStateWaitEncryptionKey") {

    std::string buf = "random_key";

    json params = {
      {"@type", "checkDatabaseEncryptionKey"},
      {"key", buf}
    };

    td_json_client_send(client, params.dump().c_str());
  }


  if (authenticationState["@type"] == "authorizationStateWaitPhoneNumber") {

    std::string buf = "";

    std::cout << "Phone number: ";

    std::cin >> buf;

    json params = {
      {"@type", "setAuthenticationPhoneNumber"},
      {"phone_number", buf}
    };

    td_json_client_send(client, params.dump().c_str());
  }

  if (authenticationState["@type"] == "authorizationStateWaitCode") {

    std::string buf = "random_key";

    std::cout << "Token code: ";

    std::cin >> buf;

    json params = {
      {"@type", "checkAuthenticationCode"},
      {"code", buf}
    };

    td_json_client_send(client, params.dump().c_str());
  }

  if (authenticationState["@type"] == "authorizationStateWaitPassword") {

    std::string prompt = "Type your password [hint: ";

    prompt += authenticationState["password_hint"];
    prompt += "]: ";

    char *pass = getpass(prompt.c_str());

    json params = {
      {"@type", "checkAuthenticationPassword"},
      {"password", pass}
    };

    td_json_client_send(client, params.dump().c_str());
  }

  if (authenticationState["@type"] == "authorizationStateWaitRegistration") {

    std::string first_name;
    std::string last_name;

    std::cout << "Enter your first name: ";
    std::cin >> first_name;

    std::cout << "Enter your last name: ";
    std::cin >> last_name;

    json params = {
      {"@type", "registerUser"},
      {"first_name", first_name},
      {"last_name", last_name}
    };

    td_json_client_send(client, params.dump().c_str());
  }

  if (authenticationState["@type"] == "authorizationStateReady") {

    json params = {
      {"@type", "getChats"},
      {"offset_order", 0},
      {"offset_chat_id", 0},
      {"limit", 10}
    };

    td_json_client_send(client, params.dump().c_str());
  }
}

int main() {
  { // Disable TDLib logs
    json params = {
      {"@type", "setLogVerbosityLevel"},
      {"new_verbosity_level", 0}
    };

    td_json_client_execute(nullptr, params.dump().c_str());
  }

  void *client = td_json_client_create();

  // chats = (std::unordered_map<long long int, Chat>*) mmap(NULL, sizeof (std::unordered_map<long long, Chat>), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  chats = new std::unordered_map<long long int, Chat>();

  const double WAIT_TIMEOUT = 100.0;  // seconds

  std::thread t([&](){
    long long c_id = 0;
    while (true) {
      std::string choice;
      std::cout << "Option [l - list, u - update chat info, m - get chat messages, p - print chat messages]: ";
      std::cin >> choice;

      if (choice == "l") {
        for (auto i : *chats)
          if (i.second.title == "") continue;
          else std::cout << "[" << i.second.id << "] - " << i.second.title << "\n";
      }

      if (choice == "m") {
        std::string id;
        std::cout << "Chat id: ";
        std::cin >> id;

        char *b;

        c_id = strtoll(id.c_str(), &b, 10);
      }

      if (c_id != 0) {
        json _params = {
          {"@type", "getChatHistory"},
          {"chat_id", c_id},
          {"from_message_id", 0},
          {"offset", -10},
          {"limit", 20},
          {"only_local", false}
        };

        td_json_client_send(client, _params.dump().c_str());
      }

      if (choice == "p") {
        for (auto i : (*chats)[c_id].messages){
          std::cout << i << '\n';
        }
      }

      if (choice == "u") {
        // Ask for the information about all the chats
        for (auto i : *chats) {

            json params = {
              {"@type", "getChat"},
              {"chat_id", i.second.id}
            };

            td_json_client_send(client, params.dump().c_str());
        }
      }
    }
  });

  t.detach();

  while (true) {
    json result = json::parse(td_json_client_receive(client, WAIT_TIMEOUT));
    if (result != nullptr) {
      if (result["@type"] == "updateAuthorizationState")
        handleAuthentication(client, result["authorization_state"]);

      // Ignore certain events
      if (std::find(ignoredUpdates.begin(), ignoredUpdates.end(), result["@type"]) != ignoredUpdates.end()) continue;

      // Get the list of chats
      if (result["@type"] == "updateChatOrder") {
        json params = {
            {"@type", "getChats"},
            {"offset_order", result["order"]},
            {"offset_chat_id", result["chat_id"]},
            {"limit", 9999}
          };

        td_json_client_send(client, params.dump().c_str());

        continue;
      }

      // When receiving a chat list, add all members to the chat set
      if (result["@type"] == "chats") {
        for (int i = 0; i < sizeof(result["chat_ids"]); ++i) {
          if (result["chat_ids"][i] == nullptr) continue;
          Chat buf;
          buf.id = result["chat_ids"][i];
          chats->insert(std::make_pair(result["chat_ids"][i], buf));
        }

        continue;
      }

      // Every time a chat is received, print it's id and title
      if (result["@type"] == "chat") {
        (*chats)[result["id"]].title = result["title"];

        continue;
      }

      // Every time a chat is received, print it's id and title
      if (result["@type"] == "messages") {
        for (int i = 0; i < result["total_count"]; i++) {
          auto a = (*chats)[result["messages"][i]["chat_id"]].messages;
          if (result["messages"][i]["content"]["@type"] != "messageText") continue;
          if (std::find(a.begin(), a.end(), result["messages"][i]["content"]["text"]["text"]) != a.end()) continue;
          (*chats)[result["messages"][i]["chat_id"]].messages.push_back(result["messages"][i]["content"]["text"]["text"]);
        }

        continue;
      }

      // std::cout << result.dump() << std::endl;
    }
  }

  td_json_client_destroy(client);
}
