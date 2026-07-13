import AppKit
import ApplicationServices

final class AppDelegate: NSObject, NSApplicationDelegate, SettingsViewControllerDelegate {
    private enum DefaultsKey {
        static let pinchEnabled = "pinchEnabled"
        static let tapClickEnabled = "tapClickEnabled"
        static let middleTapEnabled = "middleTapEnabled"
        static let controlScrollEnabled = "controlScrollEnabled"
        static let sensitivity = "sensitivity"
        static let tapClickDelayMilliseconds = "tapClickDelayMilliseconds"
    }

    private let defaults = UserDefaults.standard
    private let gestureEngine = GestureEngine()
    private let settingsViewController = SettingsViewController()
    private var statusItem: NSStatusItem?
    private let popover = NSPopover()
    private var applicationNotificationTokens: [NSObjectProtocol] = []
    private var workspaceNotificationTokens: [NSObjectProtocol] = []

    private var pinchEnabled: Bool {
        defaults.bool(forKey: DefaultsKey.pinchEnabled)
    }

    private var tapClickEnabled: Bool {
        defaults.bool(forKey: DefaultsKey.tapClickEnabled)
    }

    private var middleTapEnabled: Bool {
        defaults.bool(forKey: DefaultsKey.middleTapEnabled)
    }

    private var controlScrollEnabled: Bool {
        defaults.bool(forKey: DefaultsKey.controlScrollEnabled)
    }

    private var anyFeatureEnabled: Bool {
        pinchEnabled || tapClickEnabled || middleTapEnabled || controlScrollEnabled
    }

    private var sensitivity: Double {
        defaults.double(forKey: DefaultsKey.sensitivity)
    }

    private var tapClickDelayMilliseconds: Int {
        min(500, max(50, defaults.integer(forKey: DefaultsKey.tapClickDelayMilliseconds)))
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        defaults.register(defaults: [
            DefaultsKey.pinchEnabled: true,
            DefaultsKey.tapClickEnabled: false,
            DefaultsKey.middleTapEnabled: false,
            DefaultsKey.controlScrollEnabled: false,
            DefaultsKey.sensitivity: 1.0,
            DefaultsKey.tapClickDelayMilliseconds: 150,
        ])

        setupStatusItem()
        setupPopover()
        observeSystemChanges()

        gestureEngine?.setSensitivity(sensitivity)
        gestureEngine?.setTapClickDelayMilliseconds(tapClickDelayMilliseconds)
        applyDesiredState(promptForPermission: anyFeatureEnabled && !AXIsProcessTrusted())
        updateInterface()
    }

    func applicationWillTerminate(_ notification: Notification) {
        gestureEngine?.setEnabled(false)
        applicationNotificationTokens.forEach(NotificationCenter.default.removeObserver)
        workspaceNotificationTokens.forEach(
            NSWorkspace.shared.notificationCenter.removeObserver
        )
        applicationNotificationTokens.removeAll()
        workspaceNotificationTokens.removeAll()
    }

    private func setupStatusItem() {
        let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        if let button = item.button {
            button.image = NSImage(
                systemSymbolName: "computermouse",
                accessibilityDescription: "Magic Mouse Booster"
            ) ?? NSImage(systemSymbolName: "cursorarrow", accessibilityDescription: nil)
            button.image?.isTemplate = true
            button.target = self
            button.action = #selector(togglePopover(_:))
        }
        statusItem = item
    }

    private func setupPopover() {
        settingsViewController.delegate = self
        popover.behavior = .transient
        popover.animates = false
        popover.contentViewController = settingsViewController
    }

    private func observeSystemChanges() {
        let center = NotificationCenter.default
        applicationNotificationTokens.append(
            center.addObserver(
                forName: Notification.Name("_NSNotificationDevicesChanged"),
                object: nil,
                queue: .main
            ) { [weak self] _ in
                self?.refreshDevices()
            }
        )
        applicationNotificationTokens.append(
            center.addObserver(
                forName: NSApplication.didBecomeActiveNotification,
                object: nil,
                queue: .main
            ) { [weak self] _ in
                self?.applyDesiredState(promptForPermission: false)
                self?.updateInterface()
            }
        )

        let workspaceCenter = NSWorkspace.shared.notificationCenter
        workspaceNotificationTokens.append(
            workspaceCenter.addObserver(
                forName: NSWorkspace.didWakeNotification,
                object: nil,
                queue: .main
            ) { [weak self] _ in
                self?.refreshDevices()
            }
        )
    }

    @objc private func togglePopover(_ sender: NSStatusBarButton) {
        if popover.isShown {
            popover.performClose(nil)
            return
        }

        refreshDevices()
        updateInterface()
        popover.show(relativeTo: sender.bounds, of: sender, preferredEdge: .minY)
    }

    private func refreshDevices() {
        gestureEngine?.refreshDevices()
        updateInterface()
    }

    private func applyDesiredState(promptForPermission: Bool) {
        guard let gestureEngine else {
            return
        }

        gestureEngine.setSensitivity(sensitivity)
        gestureEngine.setTapClickDelayMilliseconds(tapClickDelayMilliseconds)
        gestureEngine.setPinchEnabled(pinchEnabled)
        gestureEngine.setTapClickEnabled(tapClickEnabled)
        gestureEngine.setMiddleTapEnabled(middleTapEnabled)
        gestureEngine.setControlScrollEnabled(controlScrollEnabled)
        guard anyFeatureEnabled else {
            gestureEngine.setEnabled(false)
            return
        }

        guard accessibilityTrusted(prompt: promptForPermission) else {
            gestureEngine.setEnabled(false)
            return
        }

        gestureEngine.setEnabled(true)
    }

    private func accessibilityTrusted(prompt: Bool) -> Bool {
        if !prompt {
            return AXIsProcessTrusted()
        }
        let promptKey = kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String
        return AXIsProcessTrustedWithOptions([promptKey: true] as CFDictionary)
    }

    private func statusText() -> String {
        guard gestureEngine != nil else {
            return "手势引擎初始化失败"
        }
        guard anyFeatureEnabled else {
            return "所有功能已关闭"
        }
        guard AXIsProcessTrusted() else {
            return "请在系统设置中授权“辅助功能”"
        }
        guard gestureEngine?.isEnabled == true else {
            return "事件监听启动失败，请关闭后重新开启"
        }

        let count = gestureEngine?.deviceCount ?? 0
        return count == 0 ? "未检测到 Magic Mouse" : "已连接 \(count) 个 Magic Mouse"
    }

    private func updateInterface() {
        settingsViewController.update(
            pinchEnabled: pinchEnabled,
            tapClickEnabled: tapClickEnabled,
            middleTapEnabled: middleTapEnabled,
            controlScrollEnabled: controlScrollEnabled,
            sensitivity: sensitivity,
            tapClickDelayMilliseconds: tapClickDelayMilliseconds,
            status: statusText()
        )
        statusItem?.button?.appearsDisabled = !anyFeatureEnabled
    }

    func settingsViewControllerDidSetPinchEnabled(_ enabled: Bool) {
        defaults.set(enabled, forKey: DefaultsKey.pinchEnabled)
        applyDesiredState(promptForPermission: enabled)
        updateInterface()
    }

    func settingsViewControllerDidSetTapClickEnabled(_ enabled: Bool) {
        defaults.set(enabled, forKey: DefaultsKey.tapClickEnabled)
        applyDesiredState(promptForPermission: enabled)
        updateInterface()
    }

    func settingsViewControllerDidSetMiddleTapEnabled(_ enabled: Bool) {
        defaults.set(enabled, forKey: DefaultsKey.middleTapEnabled)
        applyDesiredState(promptForPermission: enabled)
        updateInterface()
    }

    func settingsViewControllerDidSetControlScrollEnabled(_ enabled: Bool) {
        defaults.set(enabled, forKey: DefaultsKey.controlScrollEnabled)
        applyDesiredState(promptForPermission: enabled)
        updateInterface()
    }

    func settingsViewControllerDidSetSensitivity(_ sensitivity: Double) {
        defaults.set(sensitivity, forKey: DefaultsKey.sensitivity)
        gestureEngine?.setSensitivity(sensitivity)
        updateInterface()
    }

    func settingsViewControllerDidSetTapClickDelayMilliseconds(_ milliseconds: Int) {
        let clamped = min(500, max(50, milliseconds))
        defaults.set(clamped, forKey: DefaultsKey.tapClickDelayMilliseconds)
        gestureEngine?.setTapClickDelayMilliseconds(clamped)
        updateInterface()
    }

    func settingsViewControllerDidRequestQuit() {
        NSApplication.shared.terminate(nil)
    }
}
