#include <boost/beast.hpp>
#include <iostream>
#include <termios.h>
#include <unistd.h>


using namespace std;
namespace beast = boost::beast;
namespace asio = boost::asio;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = asio::ip::tcp;

enum class TerminalStatus {
    Error = -1,
    Eof = 0,
    Success = 1,
};

// resets terminal on destruction or call to ExitRawInputMode()
class TerminalRawInputHandler {
public:
    TerminalRawInputHandler() {
        tcgetattr(STDIN_FILENO, &m_default_term_settings);
        termios new_term_settings = m_default_term_settings;
        m_raw_input_term_settings.c_lflag &= ~(ICANON | ECHO);
        m_raw_input_term_settings.c_cc[VMIN] = 1;
        m_raw_input_term_settings.c_cc[VTIME] = 0;
    }
    void EnterRawInputMode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &m_raw_input_term_settings);
    }
    pair<char, TerminalStatus> GetInputChar() {
        char c = '\0';
        TerminalStatus status = (TerminalStatus)read(STDIN_FILENO, &c, 1);
        return {c, status};
    }
    void ExitRawInputMode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &m_default_term_settings);
    }
    ~TerminalRawInputHandler() {
        ExitRawInputMode();
    }
private:
    termios m_default_term_settings;
    termios m_raw_input_term_settings;
};

int main() {

    TerminalRawInputHandler terminal_raw_input_handler{};
    terminal_raw_input_handler.EnterRawInputMode();

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

    return EXIT_SUCCESS;
}