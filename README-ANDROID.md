# NearChuckle — Android Port 📱

Порт Far Cry (NearChuckle / SDL3) на Android: встроенный **лаунчер установки
игровых данных**, полноценное **сенсорное управление** с редактором раскладки
(кнопка **EDIT**) и **GLES3-рендер** для мобильных GPU.

English summary below — [Installation](#installation-en) · [Rendering](#rendering-note-en) ·
[Touch controls](#touch-controls-en) · [Logs](#logs-en).

---

## Что внутри

- **Лаунчер**: «Выбрать ZIP игры» / «Выбрать папку игры» → авто-распаковка в
  нужное место (вложенная папка игры находится сама) → кнопка **▶ PLAY**.
  Никаких ПК, файловых менеджеров и прав на хранилище не нужно.
- Полный движок (CryGame, Cry3DEngine, физика, ИИ, звук, Lua, сеть) собирается
  под **arm64-v8a** и **armeabi-v7a** через Android NDK + CMake + Ninja.
- **GLES3-рендер** (`SourceCode/GLESCompat`): эмуляция desktop-GL fixed
  function поверх OpenGL ES 3 — матрицы, освещение, туман, texenv/COMBINE,
  immediate mode, DXT-декомпрессия. Работает на обычных мобильных GPU
  (см. «Про рендер» ниже).
- Ввод: SDL3 (Android), поверх него — модуль `CSDLTouch` (CryInput):
  - пальцы, захваченные оверлеем, управляют кнопками/стиком;
  - свободный палец = обзор (как мышь), с настраиваемой чувствительностью;
  - в меню — прямое управление курсором и тапы (эмуляция мыши SDL3).
- Сенсорный оверлей (CryGame, `TouchControls.cpp`):
  - виртуальный стик (динамический или фиксированный) → WASD;
  - 16 кнопок по умолчанию + палитра дополнительных;
  - **EDIT-режим**: перемещение, изменение размера, скрытие/показ,
    добавление/удаление, вкл/выкл всего тача (**TCH**), сетка, сохранение
    раскладки в `touch_layout.json`.
- **Диагностика без ПК** (`AndroidApp/DiagLog.cpp`): единый лог (устройство +
  рендер + краши с картой памяти), кнопка **LOG** в игре и автопоказ после
  краша — отправка в мессенджер прямо с телефона.
- Хаптика (вибрация при нажатии), вертикальная синхронизация,
  мобильные настройки качества пишутся в `system.cfg` при первом запуске.

## Установка (RU)

1. Соберите APK:
   **Через GitHub Actions.** Файл workflow лежит в `ci-workflows/android-apk.yml`
   (GitHub-App не может пушить в `.github/workflows/`). Активируйте одной
   командой и запушьте — в Actions появится workflow **«CI Android APK»**:
   ```bash
   mkdir -p .github/workflows
   cp ci-workflows/android-apk.yml .github/workflows/
   git add .github/workflows/android-apk.yml && git commit -m "ci: android apk" && git push
   ```
   После прогона скачайте артефакт **NearChuckle-Android-apk** — внутри
   `apk-out/NearChuckle-Android-debug.apk` (готов к установке).
   **Или локально** (нужны NDK r27+, JDK 17, Android SDK):
   ```bash
   ./scripts/build_native.sh release
   gradle -p android assembleDebug
   # -> android/app/build/outputs/apk/debug/app-debug.apk
   ```
2. Установите APK (разрешите установку из неизвестных источников).
3. **Данные игры — прямо на телефоне, без ПК.** Откройте NearChuckle:
   - нажмите **«Выбрать ZIP игры»** (или «Выбрать папку игры» — через
     системный файловый менеджер), укажите ZIP/папку с Far Cry;
   - лаунчер сам найдёт вложенную папку игры, распакует и разложит всё
     в `/Android/data/app.nearchuckle.farcry/files/` (в архиве должна
     быть папка `FCData`);
   - статус «Данные игры на месте ✓» → жмите **▶ PLAY**.

   Вручную (через USB/MTP) тоже можно: скопируйте `FCData/`, `Levels/`,
   `Profiles/`, `Shaders/` в `/sdcard/Android/data/app.nearchuckle.farcry/files/`.
   Игра сама создаст `system.cfg` с мобильными настройками при первом запуске.

## Про рендер (важно!)

Оригинальный рендер Far Cry написан против **настольного OpenGL** (fixed
function + ARB-программы). На телефонах таких драйверов нет, поэтому порт
содержит слой **GLESCompat** (`SourceCode/GLESCompat`): он эмулирует нужный
движку desktop-GL поверх **OpenGL ES 3**, который есть на любом Android-GPU.

- Рендерер движка резолвит GL-функции динамически; GLESCompat подставляет
  свои реализации прямо в точку резолва — код рендера не переписывался.
- Реализовано: матричные стеки, CPU-освещение (8 источников, споты,
  затухание), туман, texenv MODULATE/REPLACE/ADD/DECAL + COMBINE,
  alpha-test, immediate mode, клиентские массивы/VBO, конверсия QUADS,
  текстуры (DXT1/3/5 софт-декод, luminance/intensity, RECT/1D-эмуляция).
- Сознательные упрощения первой версии: texenv DOT3 → modulate, texgen и
  пользовательские clip-плоскости игнорируются, Cg/ARB-программы не
  эмулируются (при `DISABLE_CG` движок и так использует fixed-function
  путь). Ничего из этого не мешает играбельности.
- Слой написан без доступа к живому ES-контексту — **первый запуск на
  устройстве покажет реальность**. Если экран чёрный/артефакты/вылет —
  отправьте лог (см. «Логи»): в нём видно, какие расширения отданы
  движку, поднялся ли контекст и где первая ошибка.
- Совсем без графики порт тоже работает: NULL-рендерер (консоль, тач,
  звук, физика) — это аварийный режим, не целевой.

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
5. Тулбар сверху: **SAVE · ADD · DEL · GRID · TCH · LOG · EXIT**
   - **SAVE** — сохранить раскладку (`touch_layout.json`);
   - **ADD** — добавить кнопку из палитры (HEAL/QSAVE/QLOAD/PRTSC),
     затем — собственную кнопку;
   - **DEL** — удалить последнюю выбранную кнопку;
   - **GRID** — вкл/выкл привязку к сетке;
   - **TCH** — полное вкл/выкл сенсорного управления (рамка зелёная =
     включено, красная = выключено; перед выключением раскладка
     сохраняется автоматически). Когда тач выключен, в правом верхнем
     углу остаётся зелёная кнопка **TOUCH OFF** — тап по ней возвращает
     управление;
   - **LOG** — собрать и отправить диагностический лог (см. «Логи»);
   - **EXIT** — выйти и сохранить.
6. Двумя пальцами по кнопке — масштабирование «щипком».

Раскладка сохраняется автоматически при выходе из редактора и
восстанавливается при запуске.

### Консольные переменные

| CVar | По умолчанию | Описание |
|---|---|---|
| `touch_enabled` | 1 | вкл/выкл сенсорное управление целиком (дубль кнопки TCH) |
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

## Логи (RU)

Всё diagnóstico работает **прямо на телефоне, без ПК**:

- **В игре**: EDIT → **LOG** — собирается лог и открывается системное
  меню «поделиться» (отправьте в любой чат/почту). Копия сохраняется в
  **Downloads** (`nearchuckle_log.txt`).
- **На лаунчере**: кнопка **«Поделиться логом»** — то же самое, работает
  даже если игра не запускается.
- **После краша**: при следующем запуске окно отправки откроется само —
  просто перешлите лог.

Что попадает в лог: модель телефона, версия Android, CPU/RAM/ABI, все
сообщения рендера и загрузки (SDL_Log → `diag.txt`), а при падении —
сигнал, адрес сбоя и карта памяти (`/proc/self/maps`). Файлы лежат в
`Android/data/app.nearchuckle.farcry/files/`: `diag.txt` (диагностика),
`log.txt` (движок), `touch_layout.json` (раскладка), `system.cfg` (конфиг).

Если есть ПК и adb: `adb logcat -s NearChuckle SDL` покажет то же самое
в реальном времени.

---

## Installation (EN)

1. Build the APK via **GitHub Actions**: the workflow file lives in
   `ci-workflows/android-apk.yml` (the GitHub App cannot push into
   `.github/workflows/`). Activate and push:
   ```bash
   mkdir -p .github/workflows
   cp ci-workflows/android-apk.yml .github/workflows/
   git add .github/workflows/android-apk.yml && git commit -m "ci: android apk" && git push
   ```
   Then grab the **NearChuckle-Android-apk** artifact from the workflow run
   (contains `apk-out/NearChuckle-Android-debug.apk`). Or build locally
   (needs NDK r27+, JDK 17, Android SDK):
   ```bash
   ./scripts/build_native.sh release
   gradle -p android assembleDebug
   # -> android/app/build/outputs/apk/debug/app-debug.apk
   ```
2. Install the APK.
3. Open NearChuckle: pick the game ZIP or folder (system file picker),
   the launcher extracts/copies it into
   `/sdcard/Android/data/app.nearchuckle.farcry/files/` automatically
   (a nested `FCData` folder is located on its own), then hit **PLAY**.
4. First launch writes a mobile-tuned `system.cfg` automatically.

### Rendering note (EN)

The 2004 renderer targets desktop OpenGL. On phones that does not exist,
so the port ships **GLESCompat** (`SourceCode/GLESCompat`): a
desktop-GL-1.x fixed-function emulation layer on top of **OpenGL ES 3**.
It plugs straight into the renderer's dynamic GL proc resolution — the
renderer code is untouched. Matrices, per-vertex lighting, fog, texenv
(+COMBINE), immediate mode, client arrays/VBOs, QUADS conversion, DXT
software decode, luminance/RECT/1D texture emulation are implemented.
Known first-pass simplifications: DOT3 combine → modulate, texgen and
user clip planes ignored, no ARB/Cg program emulation (the engine runs
its fixed-function path with `DISABLE_CG` anyway). If anything looks
wrong on real hardware, send the log — it states which extensions were
advertised and where the first error happened. Without any GL the port
falls back to the NULL renderer (console/touch/sound work, screen black).

### Touch controls (EN)

EDIT button (top-right) opens the layout editor: drag to move, the yellow
corner grip resizes, the red x-badge hides a button. Toolbar:
**SAVE · ADD · DEL · GRID · TCH · LOG · EXIT**. TCH toggles ALL touch
input on/off (layout auto-saves first; when off, a small green TOUCH OFF
button stays top-right — tap it to re-enable). LOG collects and shares
the diagnostics log. Layout persists in `touch_layout.json`. CVars:
`touch_enabled`, `touch_edit`, `touch_opacity`, `touch_scale`,
`touch_look_sens`, `touch_vibrate`, `touch_stick_dynamic`.

### Logs (EN)

No PC needed: EDIT → **LOG** (or the launcher's «Поделиться логом» button)
opens the Android share sheet with the combined log and saves a copy to
Downloads. After a native crash the share dialog opens automatically on
the next launch. Contents: device info (model/Android/CPU/RAM/ABI), all
SDL_Log lines (renderer + bootstrap), and for crashes the signal, fault
address and `/proc/self/maps`. Files live in
`Android/data/app.nearchuckle.farcry/files/`: `diag.txt`, `log.txt`,
`touch_layout.json`, `system.cfg`.

## Build layout

```
scripts/build_native.sh     # NDK build -> android/app/src/main/jniLibs/<abi>
android/                    # Gradle project + LauncherActivity + MainActivity
ci-workflows/android-apk.yml # GitHub Actions APK build (move to .github/workflows/ to enable)
SourceCode/GLESCompat/      # desktop-GL fixed-function emulation over ES3
SourceCode/AndroidApp/      # libmain.so: bootstrap, haptics, DiagLog (diag.txt + crash reports)
SourceCode/CryInput/SDLTouch.*   # finger routing, look-drag, virtual keys
SourceCode/CryGame/TouchControls.* # on-screen overlay + EDIT layout editor (TCH/LOG tools)
Externals/                  # SDL3, openal-soft, libogg, libvorbis sources
```

## Debugging

- Без ПК: EDIT → **LOG** (или кнопка на лаунчере) — лог уходит в мессенджер.
- С ПК: `adb logcat -s NearChuckle SDL` — логи в реальном времени.
- Файлы: `files/diag.txt`, `files/log.txt`, конфиг `files/system.cfg`,
  раскладка `files/touch_layout.json`.
