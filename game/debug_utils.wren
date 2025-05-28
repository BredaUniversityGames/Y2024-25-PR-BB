import "game.wren" for Main
import "gameplay/coin.wren" for CoinManager
import "engine_api.wren" for Vec3, Keycode, Random, ECS, Quat, TransformComponent
import "gameplay/flash_system.wren" for FlashSystem
import "gameplay/station.wren" for PowerUpType

import "gameplay/enemies/melee.wren" for MeleeEnemy
import "gameplay/enemies/berserker.wren" for BerserkerEnemy
import "gameplay/enemies/ranged.wren" for RangedEnemy

class DebugUtils {
    static Tick(engine, enemyList, coinManager, flashSystem, waveSystem, player) {

        var playerEntity = engine.GetECS().GetEntityByName("Player")
        var playerRot = playerEntity.GetTransformComponent().rotation
        var playerPos = playerEntity.GetTransformComponent().translation
        var spawnPos = playerPos + playerRot.mulVec3(Vec3.new(0, 0, -15.0))

        if (engine.GetInput().DebugGetKey(Keycode.eL())) {
            enemyList.add(MeleeEnemy.new(engine, spawnPos))
        }

        if (engine.GetInput().DebugGetKey(Keycode.eK())) {
            enemyList.add(BerserkerEnemy.new(engine, spawnPos))
        }

        if (engine.GetInput().DebugGetKey(Keycode.eI())) {
            player.godMode = !player.godMode
            System.print("Toggled god mode: %(player.godMode)")
        }

        if (engine.GetInput().DebugGetKey(Keycode.eJ())) {
            enemyList.add(RangedEnemy.new(engine, spawnPos))
        }

        if (engine.GetInput().DebugGetKey(Keycode.eY())) {
            player.SetCurrentPowerUp(PowerUpType.DOUBLE_GUNS)
        }

        // Spawn between 7 and 12 coins
        if (engine.GetInput().DebugGetKey(Keycode.eB())) {
            var coinCount = Random.RandomIndex(7, 12)
            
            for(i in 0...coinCount) {
                coinManager.SpawnCoin(engine, spawnPos)
            }
        }
        if(engine.GetInput().DebugGetKey(Keycode.eO())){
           flashSystem.Flash(Vec3.new(0.0, 1.0, 0.0),0.25)
        }

        if(engine.GetInput().DebugGetKey(Keycode.eP())){

            for (enemy in enemyList) {
                engine.GetECS().DestroyEntity(enemy.entity)
            }

            enemyList.clear()
            waveSystem.NextWave(engine, enemyList)
        }

        // if (engine.GetInput().DebugGetKey(Keycode.eU())) {
        //     if (__playerVariables.ultCharge >= __playerVariables.ultMaxCharge) {
        //         System.print("Activate ultimate")
        //         __activeWeapon = __armory[Weapons.shotgun]
        //         __activeWeapon.equip(engine)
        //         __playerVariables.ultActive = true

        //         engine.GetAudio().PlayEventOnce("event:/SFX/ActivateUlt")

        //         var particleEntity = engine.GetECS().NewEntity()
        //         particleEntity.AddTransformComponent().translation = __player.GetTransformComponent().translation - Vec3.new(0,3.5,0)
        //         var lifetime = particleEntity.AddLifetimeComponent()
        //         lifetime.lifetime = 400.0
        //         var emitterFlags = SpawnEmitterFlagBits.eIsActive()
        //         engine.GetParticles().SpawnEmitter(particleEntity, EmitterPresetID.eHealth(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
        //     }
        // }
        // if (engine.GetInput().DebugGetKey(Keycode.eG()) && false) {
        //     if (__playerVariables.grenadeCharge == __playerVariables.grenadeMaxCharge) {
        //         // Throw grenade
        //         __playerVariables.grenadeCharge = 0
        //     }
        // }
        // if (engine.GetInput().DebugGetKey(Keycode.e1()) && __activeWeapon.isUnequiping(engine) == false) {
        //     __activeWeapon.unequip(engine)
        //     __nextWeapon = __armory[Weapons.pistol]
        // }

        // if (engine.GetInput().DebugGetKey(Keycode.e2()) && __activeWeapon.isUnequiping(engine) == false) {
        //     __activeWeapon.unequip(engine)
        //     __nextWeapon = __armory[Weapons.shotgun]
        // }
    }
}