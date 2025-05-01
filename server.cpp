#include <boost/beast.hpp>
#include <iostream>
#include <thread>

using namespace std;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

template <typename T>
class Arc {
public:

private:
    T _data;
    mutex _mu;
};

class Game {


};

int main() {
    const auto address = asio::ip::make_address("127.0.0.1");
    const auto port = static_cast<unsigned short>(atoi("3000"));

    asio::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {address, port}};

    while (true) {
        tcp::socket socket{ioc};
        acceptor.accept(socket);
        cout << "socket accepted" << endl;

        thread{[sock = std::move(socket)]() mutable {
            websocket::stream<tcp::socket> ws{std::move(sock)};
            ws.accept();

            while (true) {
                beast::flat_buffer buffer;
                ws.read(buffer);

                cout << beast::make_printable(buffer.data()) << endl;

                string message = "Message recieved: " + beast::buffers_to_string(buffer.data());
                ws.write(asio::buffer(message));
            }
        }}.detach();
    }
}