# Апдейты
## 2025-05-18  
МС:  
- Я не понял, зачем скрипты из Examples были перенесены в корень. Я их грохнул

# Установка пакета
введите из окружения: `pip install -e .` — чтобы обеспечить динамическое обновление пакета с каждым обновлением кода

# Логирование
сохраняет в `./MotoricaInterface/log` на данный момент

# Для Teleplot
Установите расширение Teleplot для Visual Studio Code

# Отладочные скрипты
- Unsafe Scripts

# Основные примеры
## Чтение данных с EMG
- `Examples/EMG`:
-- `Simple 2EMG Reader.ipynb` - как читать данные с 2EMG
-- `Simple 2EMG logger.py` - как логировать данные с 2EMG

## Работа с Indy
- `Examples/Simple Indy Serial Interface.ipynb` - как читать и записывать данные на Indy с использованием Serial
- `Examples/Simple Indy BLE Interface.ipynb` - как читать и записывать данные на Indy с использованием BLE

## Работа с Fest
- `Examples/Simple Fest Serial Interface.ipynb` - как читать и записывать данные на Fest с использованием Serial

# Билд документации
`./make_docs.sh` — билд и запуск локального сервера с документацией по адресу http://127.0.0.1:8000/  
`mkdocs serve` — запуск локального сервера с документацией по адресу http://127.0.0.1:8000/  
`mkdocs build` — билд документации  
`mkdocs -h` — просмотр других команд  
