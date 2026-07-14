> If you find it useful, please give me a free star.⭐️  Thank you very much!

# Magic Mouse Booster

A lightweight macOS menu bar app that adds practical gestures to Magic Mouse.

## Features

- Pinch with two fingers to zoom.
- Tap the left or right side for a left or right click.
- Tap with three fingers for a middle click.
- Adjust the left/right tap delay from 50 to 500 ms.
- Hold Control and swipe up to show all application windows (Mission Control).
- Hold Control and swipe down to show all windows of the current application (App Exposé).
- Enable or disable each feature from the menu bar.

## Download and install

1. Download [Magic Mouse Booster 0.4.1](https://github.com/mrwangfushen/application_wfs/raw/refs/heads/main/Downloads/MagicMouseBooster-0.4.1.dmg).
2. Open the DMG and drag **Magic Mouse Booster** to **Applications**.
3. Launch the app.
4. Allow **Magic Mouse Booster** in **System Settings → Privacy & Security → Accessibility**.

This build is not notarized by Apple. If macOS blocks the first launch, open **System Settings → Privacy & Security** and click **Open Anyway**.

## Requirements

- Apple silicon Mac
- macOS 26 or later
- Magic Mouse

The Control + swipe feature invokes Mission Control and App Exposé directly, so it does not depend on their keyboard shortcuts being enabled.

## Build from source

```sh
make SIGN_IDENTITY=- verify
make package
```

The app uses a private macOS multitouch framework, so a future macOS update may require changes.

## License

[MIT](LICENSE)
