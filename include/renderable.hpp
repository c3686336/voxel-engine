#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP

#include "svodag.hpp"

#include <optional>

class Renderable {
public:
    const size_t model_id;
    const unsigned int max_level;
    bool visible = true;
};

#endif
