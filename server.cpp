#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <json.hpp>
#include <unordered_map>
#include <queue>
#include "shared.hpp"

using namespace std;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using json = nlohmann::json;
typedef uint ConnectionID;
typedef uint GameID;

struct CrossThreadMessageQueue {

    pair<queue<ConnectionID>, mutex> player_join_queue;
    unordered_map<ConnectionID, pair<queue<PlayerInput>, mutex>> player_inputs;

    void PushPlayerJoined(ConnectionID connection_id) {
        auto& [queue, mutex] = player_join_queue;
        scoped_lock lock{mutex};
        queue.push(connection_id);
    }

    void PushPlayerInput(ConnectionID connection_id, PlayerInput player_input) {
        auto& [queue, mutex] = player_inputs[connection_id];
        scoped_lock lock{mutex};
        queue.push(player_input);
    }
};
class Connection : public enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket socket, CrossThreadMessageQueue& message_queue, ConnectionID connection_id)
        : m_socket{std::move(socket)}
        , m_message_queue{message_queue}
        , m_connection_id{connection_id} {}
    void Start() {
        m_message_queue.PushPlayerJoined(m_connection_id);
        DoRead();
    }
    void Send(boost::asio::io_context& ioc, string& message) {
        auto self{shared_from_this()};
        ioc.post([message, this, self](){
            cout << "message: " << message << " sent to " << m_connection_id << endl;
            m_socket.async_write_some(asio::buffer(message), [](boost::system::error_code error_code, size_t size){});
        });
    }
private:
    void DoRead() {
        auto self{shared_from_this()};
        m_socket.async_read_some(asio::buffer(m_buffer),
            [this, self](boost::system::error_code error_code, size_t length) {
                if (!error_code) {
                    auto& [queue, mutex] = m_message_queue.player_inputs[m_connection_id];
                    scoped_lock lock{mutex};
                    string data_string{m_buffer.data(), length};
                    auto player_input = PlayerInput::Deserialize(data_string);
                    queue.push(player_input);
                }
                DoRead();
            });
    }

    ConnectionID m_connection_id;
    array<char, 1024> m_buffer;
    tcp::socket m_socket;
    CrossThreadMessageQueue& m_message_queue;
};
class Server {
public:
    Server(asio::ip::address address, uint16_t port, CrossThreadMessageQueue& message_queue)
        : m_ioc{1}
        , m_acceptor{m_ioc, {address, port}} 
        , m_message_queue{message_queue} {
    }
    void Run() {
        cout << "server starting" << endl;
        DoAccept(m_message_queue);
        m_ioc.run();
    }
    void SendMessage(ConnectionID connection_id, string& message) {
        if (!m_connections.contains(connection_id)) {
            cout << "No connection with id: " << connection_id << endl;
            return;
        }
        if (m_connections[connection_id].get() == nullptr) {
            cout << "sharedptr connection with id: " << connection_id << " is nullptr" << endl;
            return;
        }
        m_connections[connection_id]->Send(m_ioc, message);
    }
    void DoAccept(CrossThreadMessageQueue& message_queue) {
        m_acceptor.async_accept([this, &message_queue](boost::system::error_code error_code, tcp::socket socket) {
            cout << "connection received" << endl;
            if (!error_code) {
                m_last_connection_id++;
                auto connection = make_shared<Connection>(std::move(socket), message_queue, m_last_connection_id);
                m_connections[m_last_connection_id] = connection;
                connection->Start();
            }
            DoAccept(m_message_queue);
        });
    }
private:
    ConnectionID m_last_connection_id = 0;
    unordered_map<ConnectionID, shared_ptr<Connection>> m_connections;
    asio::io_context m_ioc;
    tcp::acceptor m_acceptor;
    CrossThreadMessageQueue& m_message_queue;
};
class Game {
public:
    vector<GameUpdate> Tick() {
        vector<GameUpdate> updates{};
        m_last_frame_time = chrono::steady_clock::now();
        // check game state
        if (m_game_state.game_status == GameStatus::PLAYER_ONE_WON) {
            cout << "Player one won" << endl;
            return updates;
        } else if (m_game_state.game_status == GameStatus::PLAYER_TWO_WON) {
            cout << "Player two won" << endl;
            return updates;
        }


        // process paddle movement
        if (m_player_one_input_state == InputState::UP) {
            MovePaddle(PlayerNumber::PLAYER_ONE, VerticalDirection::UP, updates);
        } else if (m_player_one_input_state == InputState::DOWN) {
            MovePaddle(PlayerNumber::PLAYER_ONE, VerticalDirection::DOWN, updates);
        }
        if (m_player_two_input_state == InputState::UP) {
            MovePaddle(PlayerNumber::PLAYER_TWO, VerticalDirection::UP, updates);
        } else if (m_player_two_input_state == InputState::DOWN) {
            MovePaddle(PlayerNumber::PLAYER_TWO, VerticalDirection::DOWN, updates);
        }

        // ball movement and pong hit check
        if (m_game_state.ball_pos.y == 0) {
            m_ball_direction.vertical = VerticalDirection::DOWN;
        } else if (m_game_state.ball_pos.y == m_game_state.board_height - 1) {
            m_ball_direction.vertical = VerticalDirection::UP;
        }
        if (m_game_state.ball_pos.x == 0) {
            if (!m_game_state.PaddleOverlapsCoord(PlayerNumber::PLAYER_ONE, m_game_state.ball_pos)) {
                m_game_state.game_status = GameStatus::PLAYER_TWO_WON;
                UpdateType update_type = UpdateType::GameStatus;
                UpdateValue update_val;
                update_val.val = (int)GameStatus::PLAYER_TWO_WON;
                updates.push_back({
                    .update_type = update_type,
                    .update_value{update_val}
                });
                return updates;
            }
            m_ball_direction.horizontal = HorizontalDirection::RIGHT;
        } else if (m_game_state.ball_pos.x == m_game_state.board_width - 1) {
            if (!m_game_state.PaddleOverlapsCoord(PlayerNumber::PLAYER_TWO, m_game_state.ball_pos)) {
                m_game_state.game_status = GameStatus::PLAYER_ONE_WON;
                UpdateType update_type = UpdateType::GameStatus;
                UpdateValue update_val;
                update_val.val = (int)GameStatus::PLAYER_ONE_WON;
                updates.push_back({
                    .update_type = update_type,
                    .update_value{update_val}
                });
                return updates;
            }
            m_ball_direction.horizontal = HorizontalDirection::LEFT;
        }
        if (m_ball_direction.horizontal == HorizontalDirection::LEFT) {
            m_game_state.ball_pos.x--;
        } else {
            m_game_state.ball_pos.y++;
        }

        if (m_ball_direction.vertical == VerticalDirection::UP) {
            if (m_game_state.ball_pos.y == 0) {
                m_ball_direction.vertical = VerticalDirection::DOWN;
                m_game_state.ball_pos.y++;
            } else {
                m_game_state.ball_pos.y--;
            }
        } else {
            m_game_state.ball_pos.y++;
        }
        UpdateType update_type = UpdateType::BallPosition;
        UpdateValue update_val;
        update_val.coordinate = Coordinate{.x =m_game_state.ball_pos.x, .y = m_game_state.ball_pos.y};
        updates.push_back({
            .update_type = update_type,
            .update_value{update_val}
        });
        // output
        return updates;
    }


    void MovePaddle(const PlayerNumber player_number, const VerticalDirection direction, vector<GameUpdate>& updates) {
        uint& pos = player_number == PlayerNumber::PLAYER_ONE ? m_game_state.player_one_paddle_pos : m_game_state.player_two_paddle_pos;
        if (direction == VerticalDirection::UP) {
            if (pos == 0) {return;}
            pos--;
        } else {
            if (pos == m_game_state.board_height - m_game_state.paddle_height) {return;}
            pos++;
        }
        UpdateType update_type = player_number == PlayerNumber::PLAYER_ONE ? UpdateType::PlayerOnePaddlePosition : UpdateType::PlayerTwoPaddlePosition;
        UpdateValue update_val;
        update_val.val = pos;
        updates.push_back({
            .update_type = update_type,
            .update_value{update_val}
        });
    }

    void ProcessInput(PlayerInput input, PlayerNumber player_number) {
        if (player_number == PlayerNumber::PLAYER_ONE) {
            m_player_one_input_state = input.input_state;
        } else {
            m_player_two_input_state = input.input_state;
        }
    }


    // board height must be greater than 1 and width must be greater than 3
    Game(const uint board_height = 12, const uint board_width = 12, const uint paddle_height = 4)
        : m_ball_direction{.vertical = VerticalDirection::DOWN, .horizontal = HorizontalDirection::LEFT}
        , m_game_state{GameState::FromBoardAndPaddleDimensions(board_height, board_width, paddle_height)}
        , m_player_one_input_state{InputState::NONE}
        , m_player_two_input_state{InputState::NONE}
        , m_last_frame_time{chrono::steady_clock::now()} {
            if (board_height < 2) {
                throw invalid_argument("board height < 2");
            } else if (board_width < 4) {
                throw invalid_argument("board width < 2");
            }
    }

    auto GetLastFrameTime() {
        return m_last_frame_time;
    }

private:
    Direction m_ball_direction;
    GameState m_game_state;

    chrono::time_point<chrono::steady_clock> m_last_frame_time;
    InputState m_player_one_input_state;
    InputState m_player_two_input_state;
};

struct GamePlayerConnectionIDs {
    ConnectionID player_one;
    ConnectionID player_two;
};

class GameManager {
public:
    GameManager(Server& server, CrossThreadMessageQueue& message_queue)
        : m_server{server}
        , m_message_queue{message_queue} {}
    
    void TickGames() {
        for (auto& [game_id, game] : m_games) {
            auto now = chrono::steady_clock::now();
            auto frametime = chrono::duration_cast<chrono::milliseconds>(now - game.GetLastFrameTime());
            if (frametime.count() < m_ms_tickrate) {return;}
            ProcessInputs(game_id);
            auto updates = game.Tick();
            for (auto& update : updates) {
                string message = update.Serialize();
                auto [player_one_connection_id, player_two_connection_id] = m_player_mapping[game_id];
                m_server.SendMessage(player_one_connection_id, message);
                m_server.SendMessage(player_two_connection_id, message);
            }
        }
    }
    void ProcessInputs(GameID game_id) {
        auto [player_one_connection_id, player_two_connection_id] = m_player_mapping[game_id];
        {
            auto& [queue, mutex] = m_message_queue.player_inputs[player_one_connection_id];
            scoped_lock lock{mutex};
            while (!queue.empty()) {
                m_games[game_id].ProcessInput(queue.front(), PlayerNumber::PLAYER_ONE);
                queue.pop();
            }
        }
        {
            auto& [queue, mutex] = m_message_queue.player_inputs[player_two_connection_id];
            scoped_lock lock{mutex};
            while (!queue.empty()) {
                m_games[game_id].ProcessInput(queue.front(), PlayerNumber::PLAYER_TWO);
                queue.pop();
            }
        }
    }
    void Run() {
        cout << "Starting game manager" << endl;
        while (true) {
            CheckStartGames();
            TickGames();
        }
    }
    void CheckStartGames() {
        auto& [queue, mutex] = m_message_queue.player_join_queue;
        scoped_lock lock{mutex};
        while (queue.size() >= 2) {
            ConnectionID player_one = queue.front(); queue.pop();
            ConnectionID player_two = queue.front(); queue.pop();
            cout << "game started between connection ids: " << player_one << " and " << player_two << endl;
            m_last_game_id++;
            m_games[m_last_game_id] = Game{};
            m_player_mapping[m_last_game_id] = {player_one, player_two};
        }
    }

private:
    unordered_map<GameID, GamePlayerConnectionIDs> m_player_mapping;
    GameID m_last_game_id = 0;
    unordered_map<GameID, Game> m_games;
    Server& m_server;
    CrossThreadMessageQueue& m_message_queue;
    uint m_ms_tickrate = 500;
};


int main() {
    const auto address = asio::ip::make_address("127.0.0.1");
    uint16_t port = 3000;

    CrossThreadMessageQueue message_queue{};

    Server server{address, port, message_queue};
    thread{[&server](){
        server.Run();
    }}.detach();
    
    GameManager game_manager{server, message_queue};
    game_manager.Run();

    return EXIT_SUCCESS;
}
