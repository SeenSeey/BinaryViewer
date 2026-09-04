# Binary Viewer

[![Windows](https://github.com/SeenSeey/BinaryViewer/actions/workflows/windows.yml/badge.svg?branch=main)](https://github.com/SeenSeey/BinaryViewer/actions/workflows/windows.yml)

Binary Viewer — кроссплатформенное приложение для просмотра бинарных файлов в
режиме только для чтения. Файл обрабатывается порциями, поэтому приложение не
загружает его целиком в оперативную память.

## Возможности

- просмотр данных в шестнадцатеричном формате с абсолютными 64-битными смещениями;
- выделение непрерывного диапазона байтов и вывод его двоичного представления;
- выбор порядка байтов: Little Endian или Big Endian;
- группировка данных в слова размером 16, 32, 64 или 128 бит;
- настройка размера порции от 1 байта до 4 МиБ;
- переход между порциями кнопками интерфейса или клавишами `PageUp` и `PageDown`;
- работа с файлом без изменения его содержимого.

## Быстрый старт в Windows

Готовая portable-версия предназначена для 64-битной Windows. Устанавливать CMake,
Qt или Visual Studio для её запуска не требуется.

1. Откройте страницу [Releases](https://github.com/SeenSeey/BinaryViewer/releases)
   и выберите последний релиз.
2. В разделе **Assets** скачайте файл
   `BinaryViewer-<version>-windows-x64.zip`.
3. В Проводнике Windows нажмите на скачанный ZIP правой кнопкой мыши, выберите
   **Извлечь всё...** и укажите каталог назначения.
4. Откройте полученный после извлечения каталог и запустите находящийся в нём
   файл `BinaryViewer.exe`.

После извлечения файлы приложения должны находиться рядом друг с другом:

```text
BinaryViewer.exe
Qt5Core.dll
Qt5Gui.dll
Qt5Widgets.dll
platforms/
  qwindows.dll
...
```

Не перемещайте `BinaryViewer.exe` отдельно от DLL и каталога `platforms`: они
являются частью portable-приложения и необходимы для его запуска.

> **Обратите внимание:** файлы `Source code (zip)` и `Source code (tar.gz)`,
> автоматически добавленные GitHub, содержат исходный код, а не готовое
> Windows-приложение.

Приложение пока не имеет цифровой подписи. Поэтому Microsoft Defender SmartScreen
может показать предупреждение при первом запуске.

## Работа с приложением

Нажмите `Open File` или `Ctrl+O`, чтобы открыть бинарный файл. Размер порции,
порядок байтов и размер слова настраиваются на панели инструментов.

Левая область отображает по 16 байт в строке. Выделите нужный диапазон мышью —
его двоичное представление появится в правой области. Для перехода между порциями
используйте кнопки `Previous` и `Next` либо клавиши `PageUp` и `PageDown`.

## Сборка из исходников

Следующие зависимости нужны только разработчикам:

- CMake 3.16 или новее;
- компилятор с поддержкой C++20;
- Qt 5.15 с компонентами Core, Gui и Widgets;
- компонент Qt Test для сборки автоматических тестов.

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/BinaryViewer
```

### Windows (MSVC x64)

Установите Visual Studio 2022 с компонентом **Desktop development with C++** и
Qt 5.15 для MSVC x64. Затем выполните в PowerShell, заменив путь к Qt при
необходимости:

```powershell
cmake -S . -B build/windows `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DBUILD_TESTING=ON `
  "-DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64"

cmake --build build/windows --config Release --parallel
ctest --test-dir build/windows -C Release --output-on-failure
```

Для создания portable-архива после успешных тестов выполните:

```powershell
cmake --build build/windows --config Release --target package_windows
```

Архив будет создан в каталоге:

```text
build/windows/package/windows/BinaryViewer-<version>-windows-x64.zip
```

## Непрерывная интеграция

Workflow [Windows](.github/workflows/windows.yml) автоматически собирает Release
x64, запускает тесты и формирует portable-архив для изменений в основной ветке и
pull request. Проверенные пользовательские сборки публикуются на странице
[Releases](https://github.com/SeenSeey/BinaryViewer/releases).
