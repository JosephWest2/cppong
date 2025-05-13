#pragma once
#include <string>
#include <json.hpp>
using namespace std;
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
enum class UpdateType {
    GameStatus,
    BallPosition,
    PlayerOnePaddlePosition,
    PlayerTwoPaddlePosition
};
struct Coordinate {
    int x;
    int y;

    bool operator==(const Coordinate& other) const {
        return x == other.x && y == other.y;
    }
};
union UpdateValue {
    int val;
    Coordinate coordinate;
};
struct GameUpdate {
public:
    UpdateType update_type;
    UpdateValue update_value;

    string Serialize() {
        json val;
        if (update_type == UpdateType::BallPosition) {
            val = {
                {"x", update_value.coordinate.x},
                {"y", update_value.coordinate.y},
            };
        } else {
            val = update_value.val;
        }
        json j = {
            {"t", (int)update_type},
            {"v", val},
        };
        return j.dump();
    }

    static GameUpdate Deserialize(string update_string) {
        json j = json::parse(update_string);
        int update_type = j["t"];
        UpdateValue update_value;
        if ((UpdateType)update_type == UpdateType::BallPosition) {
            update_value.coordinate.x = j["v"]["x"];
            update_value.coordinate.y = j["v"]["y"];
        } else {
            update_value.val = j["v"];
        }
        return GameUpdate{
            .update_type = (UpdateType)update_type,
            .update_value = update_value
        };
    }
};

enum class GameStatus {
    NOT_STARTED,
    PLAYING,
    PLAYER_ONE_WON,
    PLAYER_TWO_WON,
};
enum class VerticalDirection {
    UP,
    DOWN,
};
enum class HorizontalDirection {
    LEFT,
    RIGHT,
};
struct Direction {
    VerticalDirection vertical;
    HorizontalDirection horizontal;
};
struct GameState {

    uint paddle_height;
    uint board_height;
    uint board_width;

    // changes
    GameStatus game_status;
    Coordinate ball_pos;
    uint player_one_paddle_pos;
    uint player_two_paddle_pos;

    void Update(GameUpdate game_update) {
        switch (game_update.update_type) {
            case UpdateType::GameStatus:
                game_status = (GameStatus)game_update.update_value.val;
                break;
            case UpdateType::BallPosition:
                ball_pos = game_update.update_value.coordinate;
                break;
            case UpdateType::PlayerOnePaddlePosition:
                player_one_paddle_pos = game_update.update_value.val;
                break;
            case UpdateType::PlayerTwoPaddlePosition:
                player_two_paddle_pos = game_update.update_value.val;
                break;
        }

    }

    static GameState FromBoardAndPaddleDimensions(uint board_height, uint board_width, uint paddle_height) {
        return GameState{
            .paddle_height = paddle_height,
            .board_height = board_height,
            .board_width = board_width,
            .game_status = GameStatus::NOT_STARTED,
            .ball_pos{.x = (int)board_width / 2, .y = (int)board_height / 2},
            .player_one_paddle_pos = 0,
            .player_two_paddle_pos = 0
        };
    }

    friend ostream& operator<<(ostream& os, const GameState& game_state) {
        if (game_state.game_status == GameStatus::PLAYER_ONE_WON) {
            os << "Player One Won" << endl;
            return os;
        }
        if (game_state.game_status == GameStatus::PLAYER_TWO_WON) {
            os << "Player Two Won" << endl;
            return os;
        }
        int height = game_state.board_height;
        int width = game_state.board_width;
        for (int y : views::iota(0, height)) {
            for (int x : views::iota(0, width)) {
                Coordinate coordinate = {.x = x, .y = y};
                if (x == 0 && game_state.PaddleOverlapsCoord(PlayerNumber::PLAYER_ONE, coordinate)) {
                    os << ']';
                } else if (x == width - 1 && game_state.PaddleOverlapsCoord(PlayerNumber::PLAYER_TWO, coordinate)) {
                    os << '[';
                } else if (coordinate == game_state.ball_pos) {
                    os << '0';
                } else {
                    os << '.';
                }
            }
            os << endl;
        }
        os << endl;
        return os;
    }
    bool PaddleOverlapsCoord(const PlayerNumber player_number, const Coordinate coord) const {
        uint paddle_lower_bound = player_number == PlayerNumber::PLAYER_ONE ? player_one_paddle_pos : player_two_paddle_pos;
        uint paddle_upper_bound = paddle_lower_bound + paddle_height;
        return coord.y >= paddle_lower_bound && coord.y < paddle_upper_bound;
    }

};

struct PlayerInput {
    InputState input_state;

    string Serialize() {
        json j = {
            {"i", (int)input_state},
        };
        return j.dump();
    }
    static PlayerInput Deserialize(string player_input_string) {
        json j = json::parse(player_input_string);
        int input_state = j["i"];
        return PlayerInput{
            .input_state = (InputState)input_state,
        };
    }
};

