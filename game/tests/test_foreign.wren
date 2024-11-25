import "foreign_class_example.wren" for TimeModule, Vec3

var a = Vec3.new(1.0, 0.0, 0.0)
var b = Vec3.new(0.0, 0.0, 1.0)

var result = a.dot(b)
