import AppKit

@main
struct SettingsViewControllerTests {
    static func main() {
        _ = NSApplication.shared
        let controller = SettingsViewController()

        update(controller, pinchEnabled: false, tapClickEnabled: false)
        let collapsedHeight = controller.preferredContentSize.height

        update(controller, pinchEnabled: true, tapClickEnabled: false)
        let pinchHeight = controller.preferredContentSize.height
        assert(pinchHeight > collapsedHeight)

        update(controller, pinchEnabled: true, tapClickEnabled: true)
        let fullyExpandedHeight = controller.preferredContentSize.height
        assert(fullyExpandedHeight > pinchHeight)
        assert(controller.preferredContentSize.width == 360)

        print(
            "SettingsViewControllerTests: all tests passed "
                + "(\(collapsedHeight)/\(pinchHeight)/\(fullyExpandedHeight))"
        )
    }

    private static func update(
        _ controller: SettingsViewController,
        pinchEnabled: Bool,
        tapClickEnabled: Bool
    ) {
        controller.update(
            pinchEnabled: pinchEnabled,
            pinchDirectionLocked: false,
            tapClickEnabled: tapClickEnabled,
            middleTapEnabled: false,
            controlScrollEnabled: false,
            sensitivity: 1.0,
            tapClickDelayMilliseconds: 150,
            status: "已连接 1 个 Magic Mouse"
        )
    }
}
