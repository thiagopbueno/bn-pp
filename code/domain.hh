#ifndef _BN_DOMAIN_H_
#define _BN_DOMAIN_H_

#include "variable.hh"

#include <vector>
#include <unordered_map>

namespace bn {

class Domain {
public:
    Domain(std::vector<const Variable*> scope);

    unsigned width() const { return _width; };
    unsigned size()  const { return _size;  };

    friend std::ostream &operator<<(std::ostream &o, const Domain &v);

private:
    std::vector<const Variable*> _scope;
    unsigned _width;
    unsigned _size;
    std::vector<unsigned> _offset;
    std::unordered_map<unsigned, unsigned> _var_to_index;
};

}

#endif