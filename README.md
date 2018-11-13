# komfortblinker
Комфортные поворотники для автомобилей Тойота, Субару на Atmel ATtiny13a.

Программа написана в стеде Atmel Studio 7.0
В папке "Debug" находятся скомпилированные файлы.

В программе реализованы следующие функции.

- антидребезг
- возможность менять колличество повторений для поворотников и для режима "спасибо"
- возможность менять задержки пауз (дребезг)
- возможность отмены включенного поворотника
- режим "спасибо"

Режим "спасибо" работает не совсем корректно. По наблюдениям, если задействован режим "спасибо", то примерно 1 раз в 2 месяца, при слишком коротком нажатии на рычак поворота, срабатывает режим "спасибо". Т.е. вместо поворота включается аварийка.
Поэтому, по-умолчанию, режим "спасибо" отключен, но его всегда можно включить.
