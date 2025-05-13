#include "shared.hpp"
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <termios.h>
#include <unistd.h>
#include <queue>

using namespace std;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using boost_error_code = boost::system::error_code;

enum class TerminalStatus {
    Error = -1,
    Eof = 0,
    Success = 1,
};
struct MessageQueue {
    pair<queue<GameUpdate>, mutex> update_queue;
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
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        tcsetattr(STDIN_FILENO, TCSANOW, &m_raw_input_term_settings);
    }
    pair<char, TerminalStatus> GetInputChar() {
        char c = '\0';
        TerminalStatus status = (TerminalStatus)read(STDIN_FILENO, &c, 1);
        if (c == 3) {
            exit(EXIT_SUCCESS);
        }
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

class Client {
public:
    Client(MessageQueue& message_queue, string& host, string& port)
    : m_ioc{}
    , m_resolver{m_ioc}
    , m_socket{m_ioc}
    , message_queue{message_queue} {
        auto endpoint = m_resolver.resolve(host, port);
        asio::async_connect(m_socket, endpoint, [this](boost_error_code ec, const tcp::endpoint& ep) {
            if (!ec) {Read();}
        });

    }
    void Run() {
        Read();
    }
    void Write(string& message) {
        asio::async_write(m_socket, asio::buffer(message), [this](boost_error_code ec, size_t length) {});
    }
private:
    void Read() {
        m_socket.async_read_some(asio::buffer(m_buffer), [this](boost_error_code ec, size_t length) {
            if (!ec) {
                auto& [queue, mutex] = message_queue.update_queue;
                string data_string{m_buffer.data(), length};
                cout << "message received: " << data_string << endl;
                auto update = GameUpdate::Deserialize(data_string);
                scoped_lock lock{mutex};
                queue.push(update);
            }
            Read();
        });
    }
public:
    MessageQueue& message_queue;
private:
    asio::io_context m_ioc;
    tcp::resolver m_resolver;
    tcp::socket m_socket;
    array<char, 1024> m_buffer;
};

class GameManager {
public:
    GameManager(Client& client, TerminalRawInputHandler& input_handler)
        : m_client{client}
        , m_input_handler{input_handler}
        , m_game_state{}
        , m_last_frame_time{chrono::steady_clock::now()} {}
    
    void Run() {
        while (true) {
            auto now = chrono::steady_clock::now();
            auto frametime = chrono::duration_cast<chrono::milliseconds>(now - m_last_frame_time);
            if (frametime.count() < m_ms_tickrate) {continue;}
            m_last_frame_time = now;
            auto [c, terminal_status] = m_input_handler.GetInputChar();
            if (terminal_status == TerminalStatus::Success && (c == 'w' || c == 's')) {
                PlayerInput input;
                if (c == 'w') {
                    input.input_state = InputState::UP;
                }
                if (c == 's') {
                    input.input_state = InputState::DOWN;
                }
                string message = input.Serialize();
                m_client.Write(message);
            }
            auto& [queue, mutex] = m_client.message_queue.update_queue;
            scoped_lock lock{mutex};
            while (!queue.empty()) {
                auto update = queue.front(); queue.pop();
                m_game_state.Update(update);
            }
            cout << m_game_state;
        }
    }

private:
    Client& m_client;
    GameState m_game_state;
    long m_ms_tickrate = 500;
    chrono::time_point<chrono::steady_clock> m_last_frame_time;
    TerminalRawInputHandler& m_input_handler;
};

int main() {

    TerminalRawInputHandler terminal_raw_input_handler{};
    terminal_raw_input_handler.EnterRawInputMode();

    MessageQueue message_queue{};
    string host = "127.0.0.1";
    string port = "3000";
    Client client{message_queue, host, port};
    client.Run();

    GameManager game_manager{client, terminal_raw_input_handler};
    game_manager.Run();

    return EXIT_SUCCESS;
}