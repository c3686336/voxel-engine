#ifndef FORMATTER_H
#define FORMATTER_H

#include <format>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

template <glm::length_t L, typename T, glm::qualifier Q, class CharT>
struct std::formatter<glm::vec<L, T, Q>, CharT> {
    template <typename FormatParseContext>
    constexpr auto parse(FormatParseContext& pc) {
        // parse formatter args like padding, precision if you support it
        return pc.begin(); // returns the iterator to the last parsed character
                           // in the format string, in this case we just swallow
                           // everything
    }

    template <typename FormatContext>
    auto format(glm::vec<L, T, Q> p, FormatContext& fc) const {
        return std::format_to(fc.out(), "[{}]", glm::to_string(p));
    }
};

// template<class CharT>
// struct formatter<glm::vec4, CharT>
// {
//     template <typename FormatParseContext>
//     constexpr auto parse(FormatParseContext& pc)
//     {
//         // parse formatter args like padding, precision if you support it
//         return pc.end(); // returns the iterator to the last parsed character
//         in the format string, in this case we just swallow everything
//     }

//     template<typename FormatContext>
//     auto format(glm::vec4 p, FormatContext& fc) const
//     {
//         return std::format_to(fc.out(), "[{}, {}, {}, {}]", p.r, p.g, p.b,
//         p.a);
//     }
// };

// template<class CharT>
// struct formatter<glm::vec3, CharT>
// {
//     template <typename FormatParseContext>
//     constexpr auto parse(FormatParseContext& pc)
//     {
//         // parse formatter args like padding, precision if you support it
//         return pc.end(); // returns the iterator to the last parsed character
//         in the format string, in this case we just swallow everything
//     }

//     template<typename FormatContext>
//     auto format(glm::vec4 p, FormatContext& fc) const
//     {
//         return std::format_to(fc.out(), "[{}, {}, {}, {}]", p.r, p.g, p.b,
//         p.a);
//     }
// };

#endif
