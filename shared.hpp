#pragma once
#include <string>
#include <json.hpp>
using namespace std;
using json = nlohmann::json;

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
class GameState {

    void Update(GameUpdate game_update) {
        switch (game_update.update_type) {
            case UpdateType::GameStatus:
                break;
            case UpdateType::BallPosition:
                break;
            case UpdateType::PlayerOnePaddlePosition:
                break;
            case UpdateType::PlayerTwoPaddlePosition:
                break;
        }

    }

private:
    uint m_paddle_height;
    uint m_board_height;
    uint m_board_width;

    // changes
    GameStatus m_game_status;
    Coordinate m_ball_pos;
    uint m_player_one_paddle_pos;
    uint m_player_two_paddle_pos;
};

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

    string Serialize() {
        json j = {
            {"i", (int)input_state},
            {"p", (int)player_number},
        };
        return j.dump();
    }
    static PlayerInput Deserialize(string player_input_string) {
        json j = json::parse(player_input_string);
        int player_number = j["p"];
        int input_state = j["i"];
        return PlayerInput{
            .player_number = (PlayerNumber)player_number,
            .input_state = (InputState)input_state,
        };
    }
};

