import AppKit

protocol SettingsViewControllerDelegate: AnyObject {
    func settingsViewControllerDidSetPinchEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetTapClickEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetMiddleTapEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetSensitivity(_ sensitivity: Double)
    func settingsViewControllerDidSetTapClickDelayMilliseconds(_ milliseconds: Int)
    func settingsViewControllerDidRequestQuit()
}

final class SettingsViewController: NSViewController {
    weak var delegate: SettingsViewControllerDelegate?

    private let pinchCheckbox = NSButton(
        checkboxWithTitle: "启用双指缩放",
        target: nil,
        action: nil
    )
    private let tapClickCheckbox = NSButton(
        checkboxWithTitle: "轻点左右侧模拟左键和右键",
        target: nil,
        action: nil
    )
    private let middleTapCheckbox = NSButton(
        checkboxWithTitle: "三指轻点模拟中键",
        target: nil,
        action: nil
    )
    private let sensitivitySlider = NSSlider(
        value: 1.0,
        minValue: 0.5,
        maxValue: 2.5,
        target: nil,
        action: nil
    )
    private let sensitivityValueLabel = NSTextField(labelWithString: "1.0×")
    private let tapClickDelaySlider = NSSlider(
        value: 150,
        minValue: 50,
        maxValue: 500,
        target: nil,
        action: nil
    )
    private let tapClickDelayValueLabel = NSTextField(labelWithString: "150 ms")
    private let statusLabel = NSTextField(wrappingLabelWithString: "")

    override func loadView() {
        let rootView = NSView(frame: NSRect(x: 0, y: 0, width: 330, height: 258))
        preferredContentSize = rootView.frame.size

        let titleLabel = NSTextField(labelWithString: "Magic Mouse Booster")
        titleLabel.font = .boldSystemFont(ofSize: 14)

        pinchCheckbox.target = self
        pinchCheckbox.action = #selector(pinchEnabledChanged(_:))
        tapClickCheckbox.target = self
        tapClickCheckbox.action = #selector(tapClickEnabledChanged(_:))
        middleTapCheckbox.target = self
        middleTapCheckbox.action = #selector(middleTapEnabledChanged(_:))

        let sensitivityLabel = NSTextField(labelWithString: "灵敏度")
        sensitivitySlider.target = self
        sensitivitySlider.action = #selector(sensitivityChanged(_:))
        sensitivitySlider.isContinuous = true
        sensitivitySlider.widthAnchor.constraint(equalToConstant: 145).isActive = true
        sensitivityValueLabel.alignment = .right
        sensitivityValueLabel.widthAnchor.constraint(equalToConstant: 38).isActive = true

        let sensitivityRow = NSStackView(
            views: [sensitivityLabel, sensitivitySlider, sensitivityValueLabel]
        )
        sensitivityRow.orientation = .horizontal
        sensitivityRow.alignment = .centerY
        sensitivityRow.spacing = 8

        let tapClickDelayLabel = NSTextField(labelWithString: "左右轻点等待")
        tapClickDelaySlider.target = self
        tapClickDelaySlider.action = #selector(tapClickDelayChanged(_:))
        tapClickDelaySlider.isContinuous = true
        tapClickDelaySlider.widthAnchor.constraint(equalToConstant: 125).isActive = true
        tapClickDelayValueLabel.alignment = .right
        tapClickDelayValueLabel.widthAnchor.constraint(equalToConstant: 54).isActive = true

        let tapClickDelayRow = NSStackView(
            views: [tapClickDelayLabel, tapClickDelaySlider, tapClickDelayValueLabel]
        )
        tapClickDelayRow.orientation = .horizontal
        tapClickDelayRow.alignment = .centerY
        tapClickDelayRow.spacing = 8

        statusLabel.textColor = .secondaryLabelColor
        statusLabel.font = .systemFont(ofSize: 11)
        statusLabel.maximumNumberOfLines = 2

        let quitButton = NSButton(title: "退出", target: self, action: #selector(quit(_:)))
        quitButton.bezelStyle = .inline

        let footerRow = NSStackView(views: [statusLabel, quitButton])
        footerRow.orientation = .horizontal
        footerRow.alignment = .centerY
        footerRow.spacing = 8
        statusLabel.setContentHuggingPriority(.defaultLow, for: .horizontal)
        quitButton.setContentHuggingPriority(.required, for: .horizontal)

        let stack = NSStackView(
            views: [
                titleLabel,
                pinchCheckbox,
                sensitivityRow,
                tapClickCheckbox,
                tapClickDelayRow,
                middleTapCheckbox,
                footerRow,
            ]
        )
        stack.orientation = .vertical
        stack.alignment = .leading
        stack.spacing = 12
        stack.translatesAutoresizingMaskIntoConstraints = false
        rootView.addSubview(stack)

        NSLayoutConstraint.activate([
            rootView.widthAnchor.constraint(equalToConstant: 330),
            stack.topAnchor.constraint(equalTo: rootView.topAnchor, constant: 16),
            stack.leadingAnchor.constraint(equalTo: rootView.leadingAnchor, constant: 16),
            stack.trailingAnchor.constraint(equalTo: rootView.trailingAnchor, constant: -16),
            stack.bottomAnchor.constraint(equalTo: rootView.bottomAnchor, constant: -14),
            sensitivityRow.widthAnchor.constraint(equalTo: stack.widthAnchor),
            tapClickDelayRow.widthAnchor.constraint(equalTo: stack.widthAnchor),
            footerRow.widthAnchor.constraint(equalTo: stack.widthAnchor),
        ])

        self.view = rootView
    }

    func update(
        pinchEnabled: Bool,
        tapClickEnabled: Bool,
        middleTapEnabled: Bool,
        sensitivity: Double,
        tapClickDelayMilliseconds: Int,
        status: String
    ) {
        loadViewIfNeeded()
        pinchCheckbox.state = pinchEnabled ? .on : .off
        tapClickCheckbox.state = tapClickEnabled ? .on : .off
        middleTapCheckbox.state = middleTapEnabled ? .on : .off
        sensitivitySlider.doubleValue = sensitivity
        sensitivitySlider.isEnabled = pinchEnabled
        sensitivityValueLabel.stringValue = String(format: "%.1f×", sensitivity)
        tapClickDelaySlider.doubleValue = Double(tapClickDelayMilliseconds)
        tapClickDelaySlider.isEnabled = tapClickEnabled
        tapClickDelayValueLabel.stringValue = "\(tapClickDelayMilliseconds) ms"
        statusLabel.stringValue = status
    }

    @objc private func pinchEnabledChanged(_ sender: NSButton) {
        let enabled = sender.state == .on
        sensitivitySlider.isEnabled = enabled
        delegate?.settingsViewControllerDidSetPinchEnabled(enabled)
    }

    @objc private func tapClickEnabledChanged(_ sender: NSButton) {
        let enabled = sender.state == .on
        tapClickDelaySlider.isEnabled = enabled
        delegate?.settingsViewControllerDidSetTapClickEnabled(enabled)
    }

    @objc private func middleTapEnabledChanged(_ sender: NSButton) {
        delegate?.settingsViewControllerDidSetMiddleTapEnabled(sender.state == .on)
    }

    @objc private func sensitivityChanged(_ sender: NSSlider) {
        let rounded = (sender.doubleValue * 10).rounded() / 10
        sensitivityValueLabel.stringValue = String(format: "%.1f×", rounded)
        delegate?.settingsViewControllerDidSetSensitivity(rounded)
    }

    @objc private func tapClickDelayChanged(_ sender: NSSlider) {
        let milliseconds = min(500, max(50, Int(sender.doubleValue.rounded())))
        sender.doubleValue = Double(milliseconds)
        tapClickDelayValueLabel.stringValue = "\(milliseconds) ms"
        delegate?.settingsViewControllerDidSetTapClickDelayMilliseconds(milliseconds)
    }

    @objc private func quit(_ sender: NSButton) {
        delegate?.settingsViewControllerDidRequestQuit()
    }
}
