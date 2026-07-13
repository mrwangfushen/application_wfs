# Magic Mouse Booster

轻量级 macOS 菜单栏工具，为 Magic Mouse 增加实用手势。

## 功能

- 双指捏合与张开：页面缩放
- 轻点左右侧：左键与右键
- 三指轻点：鼠标中键
- 左右轻点等待时间：50–500 ms

## 下载与安装

1. 从 [Releases](https://github.com/mrwangfushen/magic-mouse-booster/releases) 下载最新 DMG。
2. 打开 DMG，将 **Magic Mouse Booster** 拖入“应用程序”。
3. 启动应用，并在“系统设置 → 隐私与安全性 → 辅助功能”中授权。

当前版本未经过 Apple 公证。若首次打开被阻止，请前往“系统设置 → 隐私与安全性”，点击“仍要打开”。

## 要求

- Apple Silicon Mac
- macOS 26 或更高版本
- Magic Mouse

## 构建

```sh
make verify
make package
```

应用使用 macOS 私有多点触控接口，未来系统更新可能需要适配。
