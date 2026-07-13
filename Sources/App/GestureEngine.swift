import Foundation

final class GestureEngine {
    private let handle: OpaquePointer

    init?() {
        guard let handle = MPZEngineCreate() else {
            return nil
        }
        self.handle = handle
    }

    deinit {
        MPZEngineDestroy(handle)
    }

    var isEnabled: Bool {
        MPZEngineIsEnabled(handle)
    }

    var deviceCount: Int {
        Int(MPZEngineGetDeviceCount(handle))
    }

    @discardableResult
    func setEnabled(_ enabled: Bool) -> Bool {
        MPZEngineSetEnabled(handle, enabled)
    }

    func setSensitivity(_ sensitivity: Double) {
        MPZEngineSetSensitivity(handle, sensitivity)
    }

    func setTapClickDelayMilliseconds(_ milliseconds: Int) {
        MPZEngineSetTapClickDelayMilliseconds(handle, Double(milliseconds))
    }

    func setPinchEnabled(_ enabled: Bool) {
        MPZEngineSetPinchEnabled(handle, enabled)
    }

    func setTapClickEnabled(_ enabled: Bool) {
        MPZEngineSetTapClickEnabled(handle, enabled)
    }

    func setMiddleTapEnabled(_ enabled: Bool) {
        MPZEngineSetMiddleTapEnabled(handle, enabled)
    }

    func setControlScrollEnabled(_ enabled: Bool) {
        MPZEngineSetControlScrollEnabled(handle, enabled)
    }

    func refreshDevices() {
        MPZEngineRefreshDevices(handle)
    }
}
