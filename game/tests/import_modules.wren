import "class_examples.wren" for Player

var newPlayer = Player.new("Jeff", 42)
var formatting = "My Name is %(newPlayer.name), and I have %(newPlayer.score)!"