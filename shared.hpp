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
        char update_type_char = SerializeUpdateType(update_type);
        json update;
        if (update_type == UpdateType::BallPosition) {
            update = {
                {"x", update_value.coordinate.x},
                {"y", update_value.coordinate.y},
            };
        } else {
            update = update_value.val;
        }
        json j = {
            {"t", update_type_char},
            {"v", update},
        };
        return j.dump();
    }

    static GameUpdate Deserialize(string update_string) {
        json j = json::parse(update_string);
        int update_type_int = j["t"];
        char update_type_char = (char)update_type_int;
        UpdateType update_type = DeserializeUpdateType(update_type_char);
        UpdateValue update_value;
        if (update_type == UpdateType::BallPosition) {
            update_value.coordinate.x = j["v"]["x"];
            update_value.coordinate.y = j["v"]["y"];
        } else {
            update_value.val = j["v"];
        }
        return GameUpdate{
            .update_type = update_type,
            .update_value = update_value
        };
    }
private:
    static char SerializeUpdateType(UpdateType update_type) {
        switch (update_type) {
            case UpdateType::GameStatus:
                return 's';
            case UpdateType::BallPosition:
                return 'b';
            case UpdateType::PlayerOnePaddlePosition:
                return '1';
            case UpdateType::PlayerTwoPaddlePosition:
                return '2';
        }
        assert(false && "switch statement should be comprehensive");
    }
    static UpdateType DeserializeUpdateType(char update_type_char) {
        switch (update_type_char) {
            case 's':
                return UpdateType::GameStatus;
            case 'b':
                return UpdateType::BallPosition;
            case '1':
                return UpdateType::PlayerOnePaddlePosition;
            case '2':
                return UpdateType::PlayerTwoPaddlePosition;
        }
        assert(false && "switch statement should be comprehensive");
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