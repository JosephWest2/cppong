#include <boost/beast.hpp>
#include <iostream>

using namespace std;
namespace beast = boost::beast;
namespace asio = boost::asio;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = asio::ip::tcp;

int main() {

    asio::io_context ioc;

    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};

    const auto results = resolver.resolve("127.0.0.1", "3000");

    auto endpoint = asio::connect(ws.next_layer(), results);

    string host = "127.0.0.1:3000";

    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    ws.handshake(host, "/");

    while (true) {
        string message;
        cout << "Enter your message:" << endl;
        getline(cin, message);
        ws.write(asio::buffer(message));

        beast::flat_buffer buffer;

        ws.read(buffer);

        cout << beast::make_printable(buffer.data()) << endl;
    }
}