#include "../Client/client.h"

int main(){
    Client client= Client(8080,"127.0.0.1");
    client.connectToServer();
    return 0;
}