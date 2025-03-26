#include "../log/log.h"

int main(){
    LOG_INIT();
    LOG_DEBUG("hello");
    LOG_INFO("hello");
    LOG_WARN("hello");
    LOG_ERROR("hello");
    LOG_FATAL("hello");
}