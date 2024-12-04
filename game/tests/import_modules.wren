import "wren_classes.wren" for Player
import "tests/hello_world.wren"

var newPlayer = Player.new("Jeff", 42)
var formatting = "My Name is %(newPlayer.name), and I have %(newPlayer.score)!"