#pragma once
#include <string>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace detail {

// Перегрузка для случая, когда аргументы закончились (только обработка остатка строки)
inline void format_parse(std::ostringstream& oss, const std::string& fmt, size_t& i) {
    while (i < fmt.size()) {
        if (fmt[i] == '%') {
            if (i + 1 < fmt.size() && fmt[i + 1] == '%') {
                oss << '%';
                i += 2;
            } else {
                // Встретили спецификатор, но нет аргумента
                throw std::runtime_error("format: not enough arguments");
            }
        } else {
            oss << fmt[i++];
        }
    }
}

// Рекурсивная перегрузка, обрабатывающая один спецификатор и оставшиеся аргументы
template<typename T, typename... Rest>
void format_parse(std::ostringstream& oss, const std::string& fmt, size_t& i, T&& arg, Rest&&... rest) {
    while (i < fmt.size()) {
        if (fmt[i] == '%') {
            if (i + 1 < fmt.size() && fmt[i + 1] == '%') {
                oss << '%';
                i += 2;
                continue;
            }
            // Извлекаем спецификатор
            char spec = fmt[++i];
            switch (spec) {
                case 'i': {
                    using CleanT = std::decay_t<T>;
                    if constexpr (std::is_integral_v<CleanT>) {
                        oss << arg;
                    } else {
                        throw std::runtime_error("format: %i requires integral type");
                    }
                    break;
                }
                case 'f': {
                    using CleanT = std::decay_t<T>;
                    if constexpr (std::is_floating_point_v<CleanT>) {
                        oss << arg;
                    } else {
                        throw std::runtime_error("format: %f requires floating point type");
                    }
                    break;
                }
                case 's': {
                    using CleanT = std::decay_t<T>;
                    if constexpr (std::is_convertible_v<CleanT, std::string>) {
                        oss << static_cast<std::string>(arg);
                    } else if constexpr (std::is_convertible_v<CleanT, const char*>) {
                        oss << static_cast<const char*>(arg);
                    } else {
                        throw std::runtime_error("format: %s requires string type (std::string or const char*)");
                    }
                    break;
                }
                default:
                    // Неизвестный спецификатор – выводим как есть
                    oss << '%' << spec;
                    break;
            }
            ++i; // переходим за спецификатор
            // Обрабатываем оставшиеся аргументы
            format_parse(oss, fmt, i, std::forward<Rest>(rest)...);
            return;
        } else {
            oss << fmt[i++];
        }
    }
    // Если дошли до конца строки, а аргументы ещё остались – ошибка
    if constexpr (sizeof...(Rest) != 0) {
        throw std::runtime_error("format: too many arguments");
    }
}

} // namespace detail

/**
 * Форматирует строку с использованием спецификаторов %i, %f, %s.
 * Поддерживается экранирование %%.
 *
 * @param fmt  Строка формата
 * @param args Аргументы для подстановки (типы должны соответствовать спецификаторам)
 * @return     Отформатированная строка
 * @throws std::runtime_error при несоответствии числа аргументов или неверном типе
 */
template<typename... Args>
std::string format(const std::string& fmt, Args&&... args) {
    std::ostringstream oss;
    size_t i = 0;
    detail::format_parse(oss, fmt, i, std::forward<Args>(args)...);
    // Обрабатываем хвост строки после последнего аргумента
    detail::format_parse(oss, fmt, i);
    return oss.str();
}