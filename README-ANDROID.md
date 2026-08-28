# NearChuckle — Android Port 📱

Порт Far Cry (NearChuckle / SDL3) на Android с полноценным сенсорным
управлением и встроенным **редактором раскладки** (кнопка **EDIT**).

English summary below — [Installation](#installation-en) · [Touch controls](#touch-controls-en).

---

## Что внутри

- Полный движок (CryGame, Cry3DEngine, физика, ИИ, звук, Lua, сеть) собирается
  под **arm64-v8a** и **armeabi-v7a** через Android NDK + CMake + Ninja.
- Ввод: SDL3 (Android), поверх него — модуль `CSDLTouch` (CryInput):
  - пальцы, захваченные оверлеем, управляют кнопками/стиком;
  - свободный палец = обзор (как мышь), с настраиваемой чувствительностью;
  - в меню — прямое управление курсором и тапы (эмуляция мыши SDL3).
- Сенсорный оверлей (CryGame, `TouchControls.cpp`):
  - виртуальный стик (динамический или фиксированный) → WASD;
  - 16 кнопок по умолчанию + палитра дополнительных;
  - **EDIT-режим**: перемещение, изменение размера, скрытие/показ,
    добавление/удаление кнопок, сетка, сохранение раскладки в
    `touch_layout.json`.
- Хаптика (вибрация при нажатии), вертикальная синхронизация,
  мобильные настройки качества пишутся в `system.cfg` при первом запуске.

## Установка (RU)

1. Скачайте `NearChuckle-Android-debug.apk` из артефактов CI (GitHub Actions →
   CI Android → NearChuckle-Android-debug) или соберите сами (ниже).
2. Установите APK (разрешите установку из неизвестных источников).
3. **Нужны данные игры.** Скопируйте из оригинальной игры (GOG/Steam/диск)
   папки в `/sdcard/Android/data/app.nearchuckle.farcry/files/`:

   ```
   files/
   ├── FCData/          (все .pak из FCData)
   ├── Levels/
   ├── Profiles/        (если есть)
   └── Shaders/         (если есть)
   ```

   Проще всего: подключите телефон по USB (MTP) или через `adb push`.
   Игра сама создаст `system.cfg` с мобильными настройками при первом запуске.

### Про рендер (важно!)

Оригинальный рендер Far Cry использует **настольный OpenGL 2.x** (Cg-шейдеры,
fixed function). На Android таких драйверов нет, поэтому:

- без настольного GL игра стартует в режиме **NULL-рендерера**: работает
  консоль, сенсорный оверлей, физика, звук — но экран чёрный;
- для честной картинки нужен desktop-GL поверх Vulkan — например
  **Mesa Zink** (Turnip/freedreno для Adreno, или llvmpipe — программный,
  медленный, но совместим со всем). Драйвер кладётся рядом с игрой и
  подключается переменными SDL (`SDL_VIDEO_EGL_DRIVER` и т.п.) через
  терминал/скрипт запуска; альтернативно используйте сборки с готовым
  Zink-окружением.
- Шейдерный кэш (`Shaders/Cache`) с десктопной сборки совместим по
  формату, но требует поддержки ARB-программ драйвером — с Zink работает.

Это фундаментальное ограничение исходного кода 2004 года, а не баг порта:
движок не содержит GLES/Vulkan-бэкенда.

## Сенсорное управление (RU)

| Элемент | Действие | Клавиша по умолчанию |
|---|---|---|
| Стик (слева внизу) | движение (динамический) | W/A/S/D |
| FIRE | огонь | мышь 1 |
| AIM | прицел | мышь 2 |
| JMP | прыжок | Space |
| CRCH | присесть | LCtrl |
| USE | использовать/лечиться | F |
| REL | перезарядка | R |
| GRN | граната | G |
| SPR | бег (удержание) | LShift |
| WPN | следующее оружие | Q |
| LIGHT | фонарь | L |
| BINOC | бинокль | B |
| MAP | карта | Tab |
| 1..4 | слоты оружия | 1..4 |
| EDIT | редактор раскладки | — |

### Режим EDIT

1. Нажмите **EDIT** (верхний правый угол).
2. **Перетаскивание** кнопки — перемещение (с привязкой к сетке).
3. **Жёлтый маркер "="** в углу кнопки — изменение размера.
4. **Красный крестик "x"** — скрыть кнопку (в редакторе она остаётся
   полупрозрачным призраком; тап по нему возвращает кнопку).
5. Тулбар сверху:
   - **SAVE** — сохранить раскладку (`touch_layout.json`);
   - **ADD** — добавить кнопку из палитры (HEAL/QSAVE/QLOAD/PRTSC),
     затем — собственную кнопку;
   - **DEL** — удалить последнюю выбранную кнопку;
   - **GRID** — вкл/выкл привязку к сетке;
   - **EXIT** — выйти и сохранить.
6. Двумя пальцами по кнопке — масштабирование «щипком».

Раскладка сохраняется автоматически при выходе из редактора и
восстанавливается при запуске.

### Консольные переменные

| CVar | По умолчанию | Описание |
|---|---|---|
| `touch_enabled` | 1 | вкл/выкл сенсорное управление целиком (в правом верхнем углу остаётся кнопка TOUCH OFF для возврата) |
| `touch_edit` | 0 | редактор раскладки (1 = включить) |
| `touch_opacity` | 0.35 | прозрачность кнопок |
| `touch_scale` | 1.0 | общий масштаб кнопок |
| `touch_look_sens` | 1.0 | чувствительность обзора пальцем |
| `touch_look_invert_x/y` | 0 | инверсия обзора |
| `touch_vibrate` | 1 | вибрация при нажатии |
| `touch_stick_dynamic` | 1 | стик под пальцем |
| `r_Width/r_Height` | авто | рендер-разрешение (масштаб задаётся при первом запуске, ~75% экрана) |

Файл раскладки: `/sdcard/Android/data/app.nearchuckle.farcry/files/touch_layout.json`
— можно править руками.

---

## Installation (EN)

1. Get the APK from CI artifacts (`CI Android` → `NearChuckle-Android-debug`)
   or build it yourself:
   ```bash
   # native modules (needs Android NDK r27+, cmake, ninja)
   ./scripts/build_native.sh release
   # APK (needs JDK 17, Android SDK, gradle 8.5+)
   gradle -p android assembleDebug
   # -> android/app/build/outputs/apk/debug/app-debug.apk
   ```
2. Install the APK.
3. Copy the retail game data (`FCData/`, `Levels/`, `Profiles/`, `Shaders/`)
   into `/sdcard/Android/data/app.nearchuckle.farcry/files/`.
4. First launch writes a mobile-tuned `system.cfg` automatically.

### Rendering note

The 2004 renderer requires desktop OpenGL 2.x. Without a desktop-GL driver
the port boots into the NULL renderer (console + touch UI work, screen is
black). For real rendering use a desktop-GL-on-Vulkan driver (Mesa Zink with
Turnip/freedreno on Adreno, or llvmpipe). This is a limitation of the
original engine, not of the port.

### Touch controls (EN)

EDIT button (top-right) opens the layout editor: drag to move, the yellow
corner grip resizes, the red x-badge hides a button, the toolbar
(SAVE/ADD/DEL/GRID/EXIT) manages the layout. Layout persists in
`touch_layout.json`. CVars: `touch_enabled`, `touch_edit`, `touch_opacity`,
`touch_scale`, `touch_look_sens`, `touch_vibrate`, `touch_stick_dynamic`.

## Build layout

```
scripts/build_native.sh     # NDK build -> android/app/src/main/jniLibs/<abi>
android/                    # Gradle project (packages prebuilt .so files)
SourceCode/AndroidApp/      # libmain.so: SDL_main bootstrap + haptics
SourceCode/CryInput/SDLTouch.*   # finger routing, look-drag, virtual keys
SourceCode/CryGame/TouchControls.* # on-screen overlay + EDIT layout editor
Externals/                  # SDL3, openal-soft, libogg, libvorbis sources
```

## Debugging

- `adb logcat -s SDL NearChuckle` — логи загрузки.
- Game log: `files/log.txt`, конфиг: `files/system.cfg`,
  раскладка: `files/touch_layout.json`.
