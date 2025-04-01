#include "../Client/client.h"

int main(){
    Client client= Client(8080,"127.0.0.1");
    client.connectToServer();
    client.sendMessage("Hello, Server!");
    std::string response = client.receiveMessage();
    std::cout << "Received response: " << response << std::endl;
    client.closeConnection();
    return 0;
}