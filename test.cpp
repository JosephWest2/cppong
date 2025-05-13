#include <iostream>
#include "shared.hpp"
using namespace std;

int main() {
    cout << "testing" << endl;
    GameUpdate update;
    update.update_type = UpdateType::BallPosition;
    update.update_value.coordinate = Coordinate{.x = 55, .y = 123};
    string update_string = update.Serialize();
    GameUpdate out_update = GameUpdate::Deserialize(update_string);

    assert(update.update_type == out_update.update_type);
    assert(update.update_value.coordinate == out_update.update_value.coordinate);

    update.update_type = UpdateType::GameStatus;
    update.update_value.val = 5;
    update_string = update.Serialize();
    out_update = GameUpdate::Deserialize(update_string);

    assert(update.update_type == out_update.update_type);
    assert(update.update_value.val == out_update.update_value.val);

    GameState game_state = GameState::FromBoardAndPaddleDimensions(12, 12, 4);

    cout << game_state;

    cout << "tests passed" << endl;
    return EXIT_SUCCESS;
}