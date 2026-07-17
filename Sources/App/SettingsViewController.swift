import AppKit

protocol SettingsViewControllerDelegate: AnyObject {
    func settingsViewControllerDidSetPinchEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetPinchDirectionLocked(_ locked: Bool)
    func settingsViewControllerDidSetTapClickEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetMiddleTapEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetControlScrollEnabled(_ enabled: Bool)
    func settingsViewControllerDidSetSensitivity(_ sensitivity: Double)
    func settingsViewControllerDidSetTapClickDelayMilliseconds(_ milliseconds: Int)
    func settingsViewControllerDidRequestQuit()
}

final class SettingsViewController: NSViewController {
    weak var delegate: SettingsViewControllerDelegate?

    private static let panelWidth: CGFloat = 360
    private static let initialPanelHeight: CGFloat = 320

    private let pinchCheckbox = NSButton(
        checkboxWithTitle: "双指捏合缩放",
        target: nil,
        action: nil
    )
    private let pinchDirectionLockCheckbox = NSButton(
        checkboxWithTitle: "单次捏合锁定放大/缩小方向",
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
    private let controlScrollCheckbox = NSButton(
        checkboxWithTitle: "Control + 上下滚动显示窗口",
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
    private var contentStack: NSStackView?
    private var pinchOptionsStack: NSStackView?
    private var tapClickOptionsStack: NSStackView?

    override func loadView() {
        let rootView = NSView(
            frame: NSRect(
                x: 0,
                y: 0,
                width: Self.panelWidth,
                height: Self.initialPanelHeight
            )
        )
        preferredContentSize = rootView.frame.size

        let headerIcon = NSImageView()
        headerIcon.image = NSImage(
            systemSymbolName: "computermouse.fill",
            accessibilityDescription: "Magic Mouse Booster"
        )
        headerIcon.contentTintColor = .controlAccentColor
        headerIcon.symbolConfiguration = NSImage.SymbolConfiguration(pointSize: 24, weight: .medium)
        headerIcon.widthAnchor.constraint(equalToConstant: 32).isActive = true
        headerIcon.heightAnchor.constraint(equalToConstant: 32).isActive = true

        let titleLabel = NSTextField(labelWithString: "Magic Mouse Booster")
        titleLabel.font = .systemFont(ofSize: 16, weight: .semibold)
        let subtitleLabel = NSTextField(labelWithString: "轻量 Magic Mouse 手势增强")
        subtitleLabel.font = .systemFont(ofSize: 11.5)
        subtitleLabel.textColor = .secondaryLabelColor

        let titleStack = NSStackView(views: [titleLabel, subtitleLabel])
        titleStack.orientation = .vertical
        titleStack.alignment = .leading
        titleStack.spacing = 2

        let headerRow = NSStackView(views: [headerIcon, titleStack])
        headerRow.orientation = .horizontal
        headerRow.alignment = .centerY
        headerRow.spacing = 10

        [
            pinchCheckbox,
            tapClickCheckbox,
            middleTapCheckbox,
            controlScrollCheckbox,
        ].forEach { checkbox in
            checkbox.font = .systemFont(ofSize: 13.5, weight: .medium)
        }
        pinchDirectionLockCheckbox.font = .systemFont(ofSize: 12.5, weight: .regular)

        pinchCheckbox.target = self
        pinchCheckbox.action = #selector(pinchEnabledChanged(_:))
        pinchDirectionLockCheckbox.target = self
        pinchDirectionLockCheckbox.action = #selector(pinchDirectionLockChanged(_:))
        tapClickCheckbox.target = self
        tapClickCheckbox.action = #selector(tapClickEnabledChanged(_:))
        middleTapCheckbox.target = self
        middleTapCheckbox.action = #selector(middleTapEnabledChanged(_:))
        controlScrollCheckbox.target = self
        controlScrollCheckbox.action = #selector(controlScrollEnabledChanged(_:))

        let sensitivityLabel = NSTextField(labelWithString: "灵敏度")
        sensitivityLabel.font = .systemFont(ofSize: 12.5)
        sensitivitySlider.target = self
        sensitivitySlider.action = #selector(sensitivityChanged(_:))
        sensitivitySlider.isContinuous = true
        sensitivitySlider.widthAnchor.constraint(equalToConstant: 145).isActive = true
        sensitivityValueLabel.alignment = .right
        sensitivityValueLabel.font = .systemFont(ofSize: 12.5, weight: .medium)
        sensitivityValueLabel.widthAnchor.constraint(equalToConstant: 38).isActive = true

        let sensitivityRow = NSStackView(
            views: [sensitivityLabel, sensitivitySlider, sensitivityValueLabel]
        )
        sensitivityRow.orientation = .horizontal
        sensitivityRow.alignment = .centerY
        sensitivityRow.spacing = 8

        let pinchOptionsStack = NSStackView(
            views: [pinchDirectionLockCheckbox, sensitivityRow]
        )
        pinchOptionsStack.orientation = .vertical
        pinchOptionsStack.alignment = .leading
        pinchOptionsStack.spacing = 8
        pinchOptionsStack.edgeInsets = NSEdgeInsets(top: 0, left: 18, bottom: 0, right: 0)
        self.pinchOptionsStack = pinchOptionsStack

        let tapClickDelayLabel = NSTextField(labelWithString: "左右轻点等待")
        tapClickDelayLabel.font = .systemFont(ofSize: 12.5)
        tapClickDelaySlider.target = self
        tapClickDelaySlider.action = #selector(tapClickDelayChanged(_:))
        tapClickDelaySlider.isContinuous = true
        tapClickDelaySlider.widthAnchor.constraint(equalToConstant: 125).isActive = true
        tapClickDelayValueLabel.alignment = .right
        tapClickDelayValueLabel.font = .systemFont(ofSize: 12.5, weight: .medium)
        tapClickDelayValueLabel.widthAnchor.constraint(equalToConstant: 54).isActive = true

        let tapClickDelayRow = NSStackView(
            views: [tapClickDelayLabel, tapClickDelaySlider, tapClickDelayValueLabel]
        )
        tapClickDelayRow.orientation = .horizontal
        tapClickDelayRow.alignment = .centerY
        tapClickDelayRow.spacing = 8

        let tapClickOptionsStack = NSStackView(views: [tapClickDelayRow])
        tapClickOptionsStack.orientation = .vertical
        tapClickOptionsStack.alignment = .leading
        tapClickOptionsStack.edgeInsets = NSEdgeInsets(
            top: 0,
            left: 18,
            bottom: 0,
            right: 0
        )
        self.tapClickOptionsStack = tapClickOptionsStack

        statusLabel.textColor = .secondaryLabelColor
        statusLabel.font = .systemFont(ofSize: 11.5, weight: .medium)
        statusLabel.maximumNumberOfLines = 2

        let quitButton = NSButton(title: "退出应用", target: self, action: #selector(quit(_:)))
        quitButton.bezelStyle = .rounded
        quitButton.controlSize = .regular
        quitButton.contentTintColor = .systemRed
        quitButton.font = .systemFont(ofSize: 12.5, weight: .medium)

        let footerRow = NSStackView(views: [statusLabel, quitButton])
        footerRow.orientation = .horizontal
        footerRow.alignment = .centerY
        footerRow.spacing = 8
        statusLabel.setContentHuggingPriority(.defaultLow, for: .horizontal)
        quitButton.setContentHuggingPriority(.required, for: .horizontal)

        let headerSeparator = makeSeparator()
        let tapSeparator = makeSeparator()
        let windowSeparator = makeSeparator()
        let footerSeparator = makeSeparator()

        let stack = NSStackView(
            views: [
                headerRow,
                headerSeparator,
                makeSectionLabel("缩放"),
                pinchCheckbox,
                pinchOptionsStack,
                tapSeparator,
                makeSectionLabel("轻点"),
                tapClickCheckbox,
                tapClickOptionsStack,
                middleTapCheckbox,
                windowSeparator,
                makeSectionLabel("窗口管理"),
                controlScrollCheckbox,
                footerSeparator,
                footerRow,
            ]
        )
        stack.orientation = .vertical
        stack.alignment = .leading
        stack.spacing = 12
        stack.translatesAutoresizingMaskIntoConstraints = false
        rootView.addSubview(stack)
        contentStack = stack

        NSLayoutConstraint.activate([
            rootView.widthAnchor.constraint(equalToConstant: Self.panelWidth),
            stack.topAnchor.constraint(equalTo: rootView.topAnchor, constant: 16),
            stack.leadingAnchor.constraint(equalTo: rootView.leadingAnchor, constant: 16),
            stack.trailingAnchor.constraint(equalTo: rootView.trailingAnchor, constant: -16),
            stack.bottomAnchor.constraint(equalTo: rootView.bottomAnchor, constant: -14),
            pinchOptionsStack.widthAnchor.constraint(equalTo: stack.widthAnchor),
            tapClickOptionsStack.widthAnchor.constraint(equalTo: stack.widthAnchor),
            headerSeparator.widthAnchor.constraint(equalTo: stack.widthAnchor),
            tapSeparator.widthAnchor.constraint(equalTo: stack.widthAnchor),
            windowSeparator.widthAnchor.constraint(equalTo: stack.widthAnchor),
            footerSeparator.widthAnchor.constraint(equalTo: stack.widthAnchor),
            footerRow.widthAnchor.constraint(equalTo: stack.widthAnchor),
        ])

        self.view = rootView
    }

    func update(
        pinchEnabled: Bool,
        pinchDirectionLocked: Bool,
        tapClickEnabled: Bool,
        middleTapEnabled: Bool,
        controlScrollEnabled: Bool,
        sensitivity: Double,
        tapClickDelayMilliseconds: Int,
        status: String
    ) {
        loadViewIfNeeded()
        pinchCheckbox.state = pinchEnabled ? .on : .off
        pinchDirectionLockCheckbox.state = pinchDirectionLocked ? .on : .off
        pinchOptionsStack?.isHidden = !pinchEnabled
        tapClickCheckbox.state = tapClickEnabled ? .on : .off
        tapClickOptionsStack?.isHidden = !tapClickEnabled
        middleTapCheckbox.state = middleTapEnabled ? .on : .off
        controlScrollCheckbox.state = controlScrollEnabled ? .on : .off
        sensitivitySlider.doubleValue = sensitivity
        sensitivitySlider.isEnabled = pinchEnabled
        sensitivityValueLabel.stringValue = String(format: "%.1f×", sensitivity)
        tapClickDelaySlider.doubleValue = Double(tapClickDelayMilliseconds)
        tapClickDelaySlider.isEnabled = tapClickEnabled
        tapClickDelayValueLabel.stringValue = "\(tapClickDelayMilliseconds) ms"
        statusLabel.stringValue = status
        updatePanelSize()
    }

    @objc private func pinchEnabledChanged(_ sender: NSButton) {
        let enabled = sender.state == .on
        delegate?.settingsViewControllerDidSetPinchEnabled(enabled)
    }

    @objc private func pinchDirectionLockChanged(_ sender: NSButton) {
        delegate?.settingsViewControllerDidSetPinchDirectionLocked(sender.state == .on)
    }

    @objc private func tapClickEnabledChanged(_ sender: NSButton) {
        let enabled = sender.state == .on
        delegate?.settingsViewControllerDidSetTapClickEnabled(enabled)
    }

    @objc private func middleTapEnabledChanged(_ sender: NSButton) {
        delegate?.settingsViewControllerDidSetMiddleTapEnabled(sender.state == .on)
    }

    @objc private func controlScrollEnabledChanged(_ sender: NSButton) {
        delegate?.settingsViewControllerDidSetControlScrollEnabled(sender.state == .on)
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

    private func updatePanelSize() {
        contentStack?.layoutSubtreeIfNeeded()
        let contentHeight = contentStack?.fittingSize.height ?? Self.initialPanelHeight
        preferredContentSize = NSSize(
            width: Self.panelWidth,
            height: ceil(contentHeight) + 30
        )
    }

    private func makeSectionLabel(_ title: String) -> NSTextField {
        let label = NSTextField(labelWithString: title)
        label.font = .systemFont(ofSize: 11.5, weight: .semibold)
        label.textColor = .secondaryLabelColor
        return label
    }

    private func makeSeparator() -> NSBox {
        let separator = NSBox()
        separator.boxType = .separator
        return separator
    }
}
