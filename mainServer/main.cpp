#include "../Server/server.h"

int main(){
    Server server(8080, 4);
    server.start();
    return 0;
}