import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random

class MusicPlayer {

    construct new(audio, musicList, volume) {

        _musicList = []
        _volume = volume

        for (music in musicList) {

      //      if (music.count > 0) {
//                audio.LoadSFX(music, false, true)
            }
            
     //       _musicList.insert(-1, music)
        }

       _index = 0

//        if (_musicList[_index].count > 0) {
   //         _currentSFX = audio.PlaySFX(_musicList[_index], _volume)
     //   }
    }

    CycleMusic(audio) {
        _index = (_index + 1) % _musicList.count

        if (_currentSFX != null) {
        //    audio.StopSFX(_currentSFX)
        }

    //    if (_musicList[_index].count  > 0) {
 //          _currentSFX = audio.PlaySFX(_musicList[_index], _volume)
    //    }
    }

    Destroy(audio) {
        if (_currentSFX != null) {
      //      audio.StopSFX(_currentSFX)
        }
    }
}

// This class uses events instead of sound instances

class BGMPlayer {
    construct new(audio, musicName, volume) {
        _event = audio.PlayEventLoop(musicName)
        audio.SetEventVolume(_event, volume)
    }

    SetAttribute(audio, name, val) {
        audio.SetEventFloatAttribute(_event, name, val)
    }

    SetVolume(audio, volume) {
        audio.SetEventVolume(_event, volume)
    }

    Destroy(audio) {
        audio.StopEvent(_event)
    }
}