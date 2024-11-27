import "EngineAPI" for Engine, TimeModule

var time_module = Engine.GetTime()
var deltatime = time_module.GetDeltatime()

System.print(deltatime)