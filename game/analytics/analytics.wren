import "engine_api.wren" for Analytics

class AnalyticsManager {
    static AccuracyEvent(currentWave, weaponName, accuracy) {
        Analytics.AddDesignEvent(Num.toString(currentWave) + ":" + weaponName, accuracy)
    }
}