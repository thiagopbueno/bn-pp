#include "domain.hh"

#include <iostream>

using namespace std;

namespace bn {

Domain::Domain(vector<const Variable*> scope) : _scope(scope), _width(scope.size())
{
    _size = 1;
    if (_width > 0) {
        _offset.reserve(_width);
        for (int i = _width-1; i >= 0; --i) {
            _offset[i] = _size;
            _size *= _scope[i]->size();
            _var_to_index[_scope[i]->id()] = i;
        }
    }
}

const Variable*
Domain::operator[](unsigned i) const
{
    if (i < _width) return _scope[i];
    else throw "Domain::operator[unsigned i]: Index out of range!";
}

void
Domain::next_valuation(vector<unsigned> &valuation) const
{
    int j;
    for (j = valuation.size()-1; j >= 0 && valuation[j] == _scope[j]->size()-1; --j) {
        valuation[j] = 0;
    }
    if (j >= 0) {
        valuation[j]++;
    }
}

ostream&
operator<<(ostream &o, const Domain &d)
{
    unsigned width = d._width;
    if (width == 0) { o << "Domain{}"; }
    else {
        o << "Domain{";
        unsigned i;
        for (i = 0; i < width-1; ++i) {
            o << d._scope[i]->id() << ", ";
        }
        o << d._scope[i]->id() << "}";
    }
    return o;
}

}