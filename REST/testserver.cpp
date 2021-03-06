#include <cpprest/http_listener.h>
#include <windows.h>
using namespace web::http::experimental::listener;
using namespace web::http;
using namespace web;

// C++ REST SDK HTTP Server test
// http://casablanca.codeplex.com/wikipage?title=HTTP%20Listener&referringTitle=Documentation

int main(void)
{
    http_listener listener(U("http://localhost:1234/"));
    listener.open().wait();
    listener.support(methods::GET, [](http_request request){
        request.reply(http::status_codes::OK, U("Hello"));
    });

    for(;;) { ::Sleep(100); }
    listener.close();
    return 0;
}
