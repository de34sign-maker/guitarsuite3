# Roger That (vst34) — клон Adobe Audition 3.0 Guitar Suite (VST3 + AU)

Бывший GuitarSuite3. Имя в DAW: **Roger That**, производитель **vst34**, файл `Roger That.vst3`.

Снято измерениями с оригинала 2 ноября 2026 (v1.1: переснято на 44,1 кГц, Resonance 34). Точность на вокале: −29 дБ по форме волны
(спектр ±0,3 дБ) — на слух неотличимо. На любой частоте проекта (48, 96 кГц…) плагин звучит так же, как Audition на 44,1: ядра фильтра и кабинета пересчитываются под частоту хоста без задержки и без обрезки (сам Audition на 48 кГц добавляет 0,5 мс задержки и режет хвост кабинета — это не воспроизводится намеренно).

Пресет по умолчанию = твой: Compressor 100 · Retro Bandpass 2042 Гц / Res 34 · Garage Fuzz 64 · Box Model 9 · Mix 14.

Известное отличие от оригинала: в Audition при Compressor ровно 50 % подкачка падает в 0 дБ (баг); в плагине там +2,5 дБ, как по кривой.

## Сборка на MacBook (один раз, ~10 минут)

1. **Терминал** (Launchpad → Другие → Terminal). Установить инструменты Xcode:
   ```
   xcode-select --install
   ```
   Появится окно — нажать «Установить», дождаться окончания.

2. Установить CMake. Если есть Homebrew:
   ```
   brew install cmake
   ```
   Если Homebrew нет — поставить его одной командой с https://brew.sh (скопировать команду с сайта), потом `brew install cmake`.

3. Распаковать `RogerThat_src.zip`, например в `~/Documents/RogerThat`.

4. В терминале:
   ```
   cd ~/Documents/RogerThat
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release -j8
   ```
   Первая команда скачает JUCE с GitHub (~2 мин). Вторая компилирует (~3–5 мин). В конце не должно быть слова `error`.

5. Плагин сам копируется в:
   - `~/Library/Audio/Plug-Ins/VST3/Roger That.vst3`
   - `~/Library/Audio/Plug-Ins/Components/Roger That.component`

6. **Studio One**: перезапустить → Studio One → Settings → Locations → VST Plug-Ins → Reset Blocklist / Rescan. Плагин появится как **Roger That** (производитель vst34).
   Если AU-версия не видна в других DAW — один раз выполнить в терминале: `killall -9 AudioComponentRegistrar`.

## Если что-то пошло не так
- `xcode-select: error: command line tools are already installed` — это нормально, идём дальше.
- `cmake: command not found` — шаг 2 не выполнен.
- Ошибка про `CMAKE_OSX_ARCHITECTURES` / x86_64 — открыть `CMakeLists.txt`, строку
  `set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" ...)` заменить на `set(CMAKE_OSX_ARCHITECTURES "arm64" ...)`.
- Скриншот терминала с ошибкой → Claude.

## Структура
- `GuitarSuiteDSP.h` — вся обработка (компрессор, фильтр-FIR, tanh-фузз, импульс кабинета, микс).
- `GuitarSuiteData.h` — измеренные коэффициенты на 44,1 кГц (256 отсчётов фильтра, 111 отсчётов кабинета, k фузза, точки компрессора).
- `Source/` — JUCE-обёртка: `PluginProcessor` (параметры, данные для дисплея), `PluginEditor` (компоновка 620×400), `Widgets` (ручка с вводом числа, тогглы, дисплей волны, тема).
- `Assets/` — шрифты Inter и JetBrains Mono (SIL OFL), вшиваются в бинарник через `juce_add_binary_data`.
- `test/test_chain.cpp`, `test/test_blocks.cpp` — офлайн-тесты: прогоняют WAV через ядро (все блоки / выбранные) и сравнивают с эталоном из Audition.

## Что живое, что фиксировано (v1)
- Живые: Compressor Amount (измерено при 25/45/50/55/75/100, кривая линейная — точно везде), Distortion Amount
  (точно при 10/30/50/70/90/100, между — интерполяция), Mix, Bypass каждого блока.
- Фиксировано: Filter = Retro BP 2042 / 34, Box = Model 9 (как в пресете). Ручки Freq/Resonance
  можно добавить позже — для этого нужны дополнительные измерения.

## Интерфейс (v1.2)
Светлая тема по макету «D». Сверху — дисплей: серым входной сигнал, розовым — то, что плагин добавляет (мокрый тракт × Mix); одна колонка = 4 мс, окно ≈ 2,5 с. Внизу в порядке сигнала: COMP · FILTER · FUZZ · BOX · MIX.
- Ручка: тянуть мышью (вверх/вниз или вправо/влево), колёсико — мелкие шаги, Cmd при перетаскивании — точный режим.
- **Двойной клик по числу в ручке** — ввести значение с клавиатуры, Enter.
- COMP / FUZZ: клик по подписи выключает блок (подчёркивание гаснет, ручка тускнеет). FILTER / BOX — кнопки-пилюли.
