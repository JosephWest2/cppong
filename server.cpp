#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <json.hpp>
#include <queue>
#include "shared.hpp"

using namespace std;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

enum class PlayerNumber {
    PLAYER_ONE = 1,
    PLAYER_TWO = 2,
};
enum class InputState {
    NONE = 0,
    UP = 1,
    DOWN = 2,
};
struct PlayerInput {
    PlayerNumber player_number;
    InputState input_state;
};
using InputQueue = pair<queue<PlayerInput>, mutex>;
class Game {
public:
    friend ostream& operator<<(ostream& cout, const Game& game) {
        int height = game.m_board_height;
        int width = game.m_board_width;
        for (int y : views::iota(0, height)) {
            for (int x : views::iota(0, width)) {
                Coordinate coordinate = {.x = x, .y = y};
                if (x == 0 && game.PaddleOverlapsCoord(PlayerNumber::PLAYER_ONE, coordinate)) {
                    cout << ']';
                } else if (x == width - 1 && game.PaddleOverlapsCoord(PlayerNumber::PLAYER_TWO, coordinate)) {
                    cout << '[';
                } else if (coordinate == game.m_ball_pos) {
                    cout << '0';
                } else {
                    cout << '.';
                }
            }
            cout << endl;
        }
        return cout;
    }
    void Tick() {
        cout << *this;
        cout << "hallo" << endl;
        m_last_frame_time = chrono::steady_clock::now();
        // check game state
        if (m_game_status == GameStatus::PLAYER_ONE_WON) {
            cout << "Player one won" << endl;
            return;
        } else if (m_game_status == GameStatus::PLAYER_TWO_WON) {
            cout << "Player two won" << endl;
            return;
        }


        // process paddle movement
        if (m_player_one_input_state == InputState::UP) {
            MovePaddle(PlayerNumber::PLAYER_ONE, VerticalDirection::UP);
        } else if (m_player_one_input_state == InputState::DOWN) {
            MovePaddle(PlayerNumber::PLAYER_ONE, VerticalDirection::DOWN);
        }
        if (m_player_two_input_state == InputState::UP) {
            MovePaddle(PlayerNumber::PLAYER_TWO, VerticalDirection::UP);
        } else if (m_player_two_input_state == InputState::DOWN) {
            MovePaddle(PlayerNumber::PLAYER_TWO, VerticalDirection::DOWN);
        }

        // ball movement and pong hit check
        if (m_ball_pos.y == 0) {
            m_ball_direction.vertical == VerticalDirection::DOWN;
        } else if (m_ball_pos.y == m_board_height - 1) {
            m_ball_direction.vertical == VerticalDirection::UP;
        }
        if (m_ball_pos.x == 0) {
            if (!PaddleOverlapsCoord(PlayerNumber::PLAYER_ONE, m_ball_pos)) {
                m_game_status = GameStatus::PLAYER_TWO_WON;
                return;
            }
            m_ball_direction.horizontal = HorizontalDirection::RIGHT;
        } else if (m_ball_pos.x == m_board_width - 1) {
            if (!PaddleOverlapsCoord(PlayerNumber::PLAYER_TWO, m_ball_pos)) {
                m_game_status = GameStatus::PLAYER_ONE_WON;
                return;
            }
            m_ball_direction.horizontal = HorizontalDirection::LEFT;
        }
        if (m_ball_direction.horizontal == HorizontalDirection::LEFT) {
            m_ball_pos.x--;
        } else {
            m_ball_pos.y++;
        }

        if (m_ball_direction.vertical == VerticalDirection::UP) {
            if (m_ball_pos.y == 0) {
                m_ball_direction.vertical = VerticalDirection::DOWN;
                m_ball_pos.y++;
            } else {
                m_ball_pos.y--;
            }
        } else {
            m_ball_pos.y++;
        }
        // output
    }

    bool PaddleOverlapsCoord(const PlayerNumber player_number, const Coordinate coord) const {
        uint paddle_lower_bound = player_number == PlayerNumber::PLAYER_ONE ? m_player_one_paddle_pos : m_player_two_paddle_pos;
        uint paddle_upper_bound = paddle_lower_bound + m_paddle_height;
        return coord.y >= paddle_lower_bound && coord.y < paddle_upper_bound;
    }

    void MovePaddle(const PlayerNumber player_number, const VerticalDirection direction) {
        uint& pos = player_number == PlayerNumber::PLAYER_ONE ? m_player_one_paddle_pos : m_player_two_paddle_pos;
        if (direction == VerticalDirection::UP) {
            if (pos == 0) {return;}
            pos--;
        } else {
            if (pos == m_board_height - m_paddle_height) {return;}
            pos++;
        }
    }

    void ProcessInputs() {
        auto& [queue, mutex] = *m_input_queue;
        scoped_lock lock{mutex};
        while (!queue.empty()) {
            auto input = queue.front();
            if (input.player_number == PlayerNumber::PLAYER_ONE) {
                m_player_one_input_state = input.input_state;
            } else {
                m_player_two_input_state = input.input_state;
            }
            queue.pop();
        }
    }

    void Run() {
        while (true) {
            auto now = chrono::steady_clock::now();
            auto frametime = chrono::duration_cast<chrono::milliseconds>(now - m_last_frame_time);
            if (frametime.count() < m_ms_tickrate) {continue;}
            ProcessInputs();
            Tick();
        }
    }
    GameStatus GetGameSatate() {
        return m_game_status;
    }
    shared_ptr<InputQueue> GetInputQueue() {
        return m_input_queue;
    }



    // board height must be greater than 1 and width must be greater than 3
    Game(const uint board_height = 12, const uint board_width = 12, const uint paddle_height = 4)
        : m_ball_direction{.vertical = VerticalDirection::DOWN, .horizontal = HorizontalDirection::LEFT}
        , m_ball_pos{.x = (int)board_width / 2, .y = (int)board_height / 2}
        , m_game_status{GameStatus::NOT_STARTED}
        , m_player_one_input_state{InputState::NONE}
        , m_player_two_input_state{InputState::NONE}
        , m_last_frame_time{chrono::steady_clock::now()}
        , m_listening{true}
        , m_input_queue{make_shared<InputQueue>()}
        , m_ms_tickrate{1000}
        , m_paddle_height{paddle_height}
        , m_board_height{board_height}
        , m_board_width{board_width}
        , m_player_one_paddle_pos{0}
        , m_player_two_paddle_pos{0} {

            if (board_height < 2) {
                throw invalid_argument("board height < 2");
            } else if (board_width < 4) {
                throw invalid_argument("board width < 2");
            }
    }

private:
    Direction m_ball_direction;
    uint m_paddle_height;
    uint m_board_height;
    uint m_board_width;
    uint m_player_one_paddle_pos;
    uint m_player_two_paddle_pos;
    atomic_bool m_listening;
    shared_ptr<InputQueue> m_input_queue;
    chrono::time_point<chrono::steady_clock> m_last_frame_time;
    InputState m_player_one_input_state;
    InputState m_player_two_input_state;
    Coordinate m_ball_pos;
    uint m_ms_tickrate;
    GameStatus m_game_status;
};
class Connection : public enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket socket, shared_ptr<InputQueue> input_queue)
        : m_socket{std::move(socket)}
        , m_input_queue{input_queue} {}
    void Start() {
        DoRead();
    }
private:
    void DoRead() {
        auto self{shared_from_this()};
        m_socket.async_read_some(asio::buffer(m_buffer),
            [this, self](boost::system::error_code error_code, size_t length) {
                if (!error_code) {
                    auto& [queue, mutex] = *m_input_queue;
                    scoped_lock lock{mutex};
                    json data = json::parse(string{m_buffer.data(), length});
                    if (data.contains("input") && data.contains("player")) {
                        int input = data["input"];
                        int player_number = data["player"];
                        if (input < 3 && input >= 0 && (player_number == 1 || player_number == 2)) {
                            queue.push(PlayerInput{.player_number = (PlayerNumber)player_number, .input_state = (InputState)input});
                        }
                    }
                }
                DoRead();
            });
    }
    Connection(asio::io_context& ioc)
        : m_socket{ioc} {}

    array<char, 1024> m_buffer;
    tcp::socket m_socket;
    shared_ptr<InputQueue> m_input_queue;
};
class Server {
public:
    Server(asio::ip::address address, uint16_t port)
        : m_ioc{1}
        , m_acceptor{m_ioc, {address, port}} {
    }
    void DoAccept(shared_ptr<InputQueue> input_queue) {
        m_acceptor.async_accept([this, input_queue](boost::system::error_code error_code, tcp::socket socket) {
            cout << "Async accept" << endl;
            if (!error_code) {
                make_shared<Connection>(std::move(socket), input_queue);
            }
            DoAccept(input_queue);
        });
    }
private:
    asio::io_context m_ioc;
    tcp::acceptor m_acceptor;
};

int main() {
    const auto address = asio::ip::make_address("127.0.0.1");
    uint16_t port = 3000;

    Server ws_handler{address, port};
}