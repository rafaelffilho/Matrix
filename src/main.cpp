#include <iostream>
#include <td/telegram/td_json_client.h>
#include <json.hpp>
#include <string>
#include <unistd.h>
#include <unordered_set>

std::unordered_set<int>* chats;

using json = nlohmann::json;

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

  chats = new std::unordered_set<int>();

  const double WAIT_TIMEOUT = 100.0;  // seconds

  while (true) {
    json result = json::parse(td_json_client_receive(client, WAIT_TIMEOUT));
    if (result != nullptr) {
      if (result["@type"] == "updateAuthorizationState")
        handleAuthentication(client, result["authorization_state"]);

      // Ignore this events
      if (result["@type"] == "updateUser") continue;
      if (result["@type"] == "updateChatPinnedMessage") continue;
      if (result["@type"] == "updateSupergroupFullInfo") continue;
      if (result["@type"] == "updateOption") continue;
      if (result["@type"] == "updateChatLastMessage") continue;
      if (result["@type"] == "updateSupergroup") continue;
      if (result["@type"] == "updateBasicGroup") continue;
      if (result["@type"] == "updateNewChat") continue;
      if (result["@type"] == "updateSelectedBackground") continue;
      if (result["@type"] == "updateScopeNotificationSettings") continue;
      if (result["@type"] == "updateChatReadInbox") continue;
      if (result["@type"] == "updateChatReadOutbox") continue;
      if (result["@type"] == "updateChatNotificationSettings") continue;
      if (result["@type"] == "updateNewMessage") continue;
      if (result["@type"] == "updateUnreadMessageCount") continue;
      if (result["@type"] == "updateBasicGroupFullInfo") continue;
      if (result["@type"] == "updateDeleteMessages") continue;
      if (result["@type"] == "updateMessageContent") continue;
      if (result["@type"] == "updateMessageEdited") continue;
      if (result["@type"] == "updateFile") continue;
      if (result["@type"] == "remoteFile") continue;
      if (result["@type"] == "updateUnreadChatCount") continue;
      if (result["@type"] == "updateHavePendingNotifications") continue;
      if (result["@type"] == "updateUserStatus") continue;
      if (result["@type"] == "updateMessageViews") continue;
      // if (result["@type"] == "error") continue;

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
          chats->emplace(result["chat_ids"][i]);
        }

        continue;
      }

      // Every time a chat is received, print it's id and title
      if (result["@type"] == "chat") {
        
        if (result["title"] == "") continue;

        std::cout << "[" << result["id"] << "]: " << result["title"] << "\n";

        // std::cout << "--------------------------------\n";
        // std::cout << result.dump();
        // std::cout << "--------------------------------\n";

        continue;
      }


      // Ask for the information about all the chats
      for (auto i : *chats) {

          json params = {
            {"@type", "getChat"},
            {"chat_id", i}
          };
          td_json_client_send(client, params.dump().c_str());
      }

      std::cout << result.dump() << std::endl;
    }
  }

  td_json_client_destroy(client);
}
