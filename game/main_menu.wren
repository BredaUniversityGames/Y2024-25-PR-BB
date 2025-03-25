import "engine_api.wren" for Engine, Input, Vec3, Quat, Math, Keycode
import "gameplay/camera.wren" for CameraVariables
import "gameplay/music_player.wren" for MusicPlayer

class Main {

    static Start(engine) {
        System.print("Start main menu")
        engine.GetInput().SetMouseHidden(false)
        engine.GetGame().SetMainMenuEnabled(true)
        // __background = engine.LoadModel("assets/models/main_menu.glb")
       
        // __transform = __background.GetTransformComponent()
        // __transform.translation = Vec3.new(15.4,-20,-203)
        // __transform.rotation = Quat.new(0,0,-0.2,0.1)
        
        __camera = engine.GetECS().NewEntity()
        __cameraVariables = CameraVariables.new()
        
        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = 45.0
        cameraProperties.nearPlane = 0.5
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
        __camera.AddAudioEmitterComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()

        var camTrans = __camera.GetTransformComponent()
        camTrans.rotation = Quat.new(0.982,0.145,0.117,-0.017)

        var musicList = [
            "assets/music/main_menu/Alon Peretz - The Queens Quarters.wav",
            "assets/music/main_menu/Itai Argaman - The Sacred Voice.wav",
            "assets/music/main_menu/Kyle Preston - Dark Tension.wav",
            "assets/music/main_menu/Kyle Preston - Orchestral Tension.wav",
            ""
            ]

        var ambientList = [
            "assets/music/ambient/Crow Cawing 2 - QuickSounds.com.mp3",
            ""
            ]

        __musicPlayer = MusicPlayer.new(engine.GetAudio(), musicList, 0.3)
        __ambientPlayer = MusicPlayer.new(engine.GetAudio(), ambientList, 0.1)
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
        engine.GetECS().DestroyAllEntities()
        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())
    }

    static Update(engine, dt) {

        if (engine.GetInput().DebugGetKey(Keycode.e8())) {
            System.print("Next Main Menu Track")
            __musicPlayer.CycleMusic(engine.GetAudio())
        }

        if (engine.GetInput().DebugGetKey(Keycode.e9())) {
            System.print("Next Ambient Track")
            __ambientPlayer.CycleMusic(engine.GetAudio())
        }

        if(engine.GetGame().GetMainMenu().PlayButtonPressedOnce()){
            engine.GetGame().SetMainMenuEnabled(false)
            engine.TransitionToScript("game/game.wren")
            return
        }
        if(engine.GetGame().GetMainMenu().QuitButtonPressedOnce()){
            engine.SetExit(0)
            return
        }
    }
}