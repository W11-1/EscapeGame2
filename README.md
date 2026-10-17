# 异界逃生：考试倒计时

一个使用 C++17 和 raylib 5.5 开发的 2D 暗黑幻想恐怖逃生游戏原型。

玩家是一名普通人类，却误入恶魔生活的世界，并在恶魔学校中参加一场看似普通的考试。随着调查推进，考试规则和两位老师的目的逐渐显露出异常。

## 当前功能

- 第一章：恶魔学校开场、教室探索、物品调查、对白和三选一。
- 第二章：不公平的考试、试卷和黑板调查、作弊事件、恶魔追逐与逃跑。
- 第三章：目前只有占位入口，尚未实现完整关卡。
- 生命值、精神值和信任值 HUD。
- 调查交互、分支选项和属性变化。
- 恶魔追逐、碰撞扣血和 GAME OVER。
- 当前美术使用 raylib 基本图形绘制的像素风占位画面。
- 中文界面优先使用 Windows 黑体；字体加载失败时切换到英文界面。

## 操作

| 按键 | 功能 |
|---|---|
| `WASD` / 方向键 | 移动 |
| `E` | 调查、交谈或触发交互 |
| `E` / `Enter` / `Space` | 推进对话 |
| `W` / `S` | 在选项间移动 |
| `1` / `2` / `3` | 直接选择对应选项 |
| `Enter` | 确认章节提示或进入下一章节入口 |

生命值归零后会进入 GAME OVER。

## 环境要求

- Windows 10 或 Windows 11
- Visual Studio 2022，安装“使用 C++ 的桌面开发”
- CMake 3.20 或更高版本
- Ninja
- Git
- 首次配置时需要访问 GitHub，以下载 raylib 5.5

## 构建

请从 **x64 Native Tools Command Prompt for VS 2022** 或对应的 Visual Studio Developer PowerShell 中，在项目根目录执行：

```powershell
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

首次配置时，CMake 会通过 FetchContent 自动获取 raylib 5.5。

## 运行

```powershell
out\build\x64-debug\EscapeGame2.exe
```

## 开发状态

这是一个仍在开发中的学习项目。当前剧情、场景和交互会继续调整，第三章尚未完成。
