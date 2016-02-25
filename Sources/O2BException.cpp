#include "O2BException.hpp"

namespace Osu2Bms {

    O2BException::O2BException(const std::string &description)
        : _Description(description) {}

    const std::string &O2BException::Description() const {
        return _Description;
    }

}
