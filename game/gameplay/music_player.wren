import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random

class MusicPlayer {

    construct new(audio, musicList) {

        _musicList = []

        for (music in musicList) {
            audio.LoadSFX(music, false, true)
            _musicList.insert(-1, music)
        }

        _index = 0
        _currentSFX = audio.PlaySFX(_musicList[_index], 0.2)
    }

    Update(input, audio) {
        if (input.DebugGetKey(Keycode.eM())) {
            _index = (_index + 1) % _musicList.count
            audio.StopSFX(_currentSFX)
            _currentSFX = audio.PlaySFX(_musicList[_index], 0.2)
        }
    }

    Destroy(audio) {
        audio.StopSFX(_currentSFX)
    }
}