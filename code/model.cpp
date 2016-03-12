#include "model.hh"

#include <unordered_set>
#include <iostream>
using namespace std;

namespace bn {

BN::BN(string name, vector<Variable*> &variables, vector<Factor*> &factors) :
	_name(name),
	_variables(variables),
	_factors(factors)
{
	for (unsigned i = 0; i < _variables.size(); ++i) {
		const Variable *v = _variables[i];
		unordered_set<const Variable*> children;
		_children[v] = children;
	}

	for (unsigned i = 0; i < _variables.size(); ++i) {
		const Variable *v = _variables[i];
		vector<const Variable*> scope = _factors[i]->domain().scope();
		unordered_set<const Variable*> parents(scope.begin()+1, scope.end());
		_parents[v] = parents;
		for (auto p : parents) {
			_children[p].insert(v);
		}
	}
}

BN::~BN()
{
	for (auto pv : _variables) {
		delete pv;
	}
	for (auto pf : _factors) {
		delete pf;
	}
}

Factor
BN::joint_distribution() const
{
	Factor f(1.0);
	for (auto pf : _factors) {
		f *= *pf;
	}
	return f;
}

Factor
BN::query(const unordered_set<const Variable*> &target, const unordered_set<const Variable*> &evidence, bool verbose) const
{
	unordered_set<const Variable*> Np, Ne, F;
	bayes_ball(target, evidence, F, Np, Ne);

	Factor joint(1.0);
	for (auto const pv : Np) {
		unsigned id = pv->id();
		joint *= *_factors[id];
	}

	if (verbose) {
		cout << ">> Requisite probability nodes Np:" << endl;
		for (auto const pv : Np) {
			cout << *pv << endl;
		}
		cout << endl;

		cout << ">> Requisite observation nodes Ne" << endl;
		for (auto const pv: Ne) {
			cout << *pv << endl;
		}
		cout << endl;
	}

	Factor f = joint;
	for (auto pv : _variables) {
		if (target.find(pv) == target.end() && evidence.find(pv) == evidence.end()) {
			f = f.sum_out(pv);
		}
	}
	if (!evidence.empty()) {
		Factor g = joint;
		for (auto pv : _variables) {
			if (evidence.find(pv) == evidence.end()) {
				g = g.sum_out(pv);
			}
		}
		f = f.divide(g);
	}
	return f;
}

void
BN::bayes_ball(const unordered_set<const Variable*> &J, const unordered_set<const Variable*> &K, const unordered_set<const Variable*> &F, unordered_set<const Variable*> &Np, unordered_set<const Variable*> &Ne) const
{
	// Initialize all nodes as neither visited, nor marked on the top, nor marked on the bottom.
	unordered_set<const Variable*> visited, top, bottom;

	// Create a schedule of nodes to be visited,
	// initialized with each node in J to be visited as if from one of its children.
	vector<const Variable*> schedule;
	vector<bool> origin; // true if from children, false from parent
	for (auto const j : J) {
		schedule.push_back(j);
		origin.push_back(true);
	}

	// While there are still nodes scheduled to be visited:
	while (!schedule.empty()) {

		// Pick any node j scheduled to be visited and remove it from the schedule.
		// Either j was scheduled for a visit from a parent, a visit from a child, or both.
		const Variable *j = schedule.back(); schedule.pop_back();
		bool from_child = origin.back();  origin.pop_back();

		// Mark j as visited.
		visited.insert(j);

		// If j not in K and the visit to j is from a child:
		if (K.find(j) == K.end() && from_child) {

			// if the top of j is not marked,
			// then mark its top and schedule each of its parents to be visited;
			if (top.find(j) == top.end()) {
				top.insert(j);
				unordered_set<const Variable*> parents = _parents.at(j);
				for (auto const pa : parents) {
					schedule.push_back(pa);
					origin.push_back(true);
				}
			}

			// if j not in F and the bottom of j is not marked,
			// then mark its bottom and schedule each of its children to be visited.
			if (F.find(j) == F.end() && bottom.find(j) == bottom.end()) {
				bottom.insert(j);
				unordered_set<const Variable*> children = _children.at(j);
				for (auto const ch : children) {
					schedule.push_back(ch);
					origin.push_back(false);
				}
			}
		}
		// If the visit to j is from a parent:
		else if (!from_child) {

			// If j in K and the top of j is not marked,
			// then mark its top and schedule each of its parents to be visited;
			if (K.find(j) != K.end() && top.find(j) == top.end()) {
				top.insert(j);
				unordered_set<const Variable*> parents = _parents.at(j);
				for (auto const pa : parents) {
					schedule.push_back(pa);
					origin.push_back(true);
				}
			}

			// If j not in K and the bottom of j is not marked,
			// then mark its bottom and schedule each of its children to be visited.
			if (K.find(j) == K.end() && bottom.find(j) == bottom.end()) {
				bottom.insert(j);
				unordered_set<const Variable*> children = _children.at(j);
				for (auto const ch : children) {
					schedule.push_back(ch);
					origin.push_back(false);
				}
			}
		}
	}

	// The requisite probability nodes Np are those nodes marked on top.
	for (auto const pv : top) {
		Np.insert(pv);
	}

	// The requisite observation nodes Ne are those nodes in K marked as visited.
	for (auto const pv : K) {
		if (visited.find(pv) != visited.end()) {
			Ne.insert(pv);
		}
	}
}

unordered_set<const Variable*>
BN::markov_independence(const Variable* v) const
{
	unordered_set<const Variable*> nd;
	for (auto pv : _variables) {
		nd.insert(pv);
	}
	nd.erase(nd.find(v));

	for (auto pv : _parents.find(v)->second) {
		nd.erase(nd.find(pv));
	}
	for (auto id : descendants(v)) {
		nd.erase(nd.find(id));
	}
	return nd;
}

unordered_set<const Variable*>
BN::descendants(const Variable *v) const
{
	unordered_set<const Variable*> desc;
	if (!_children.find(v)->second.empty()) {
		for (auto pv : _children.find(v)->second) {
			desc.insert(pv);
			for (auto d : descendants(pv)) {
				desc.insert(d);
			}
		}
	}
	return desc;
}

ostream&
operator<<(ostream &os, const BN &bn)
{
	os << ">> Variables" << endl;
	for (auto pv1 : bn._variables) {
		unordered_set<const Variable*> parents = bn._parents.find(pv1)->second;
		unordered_set<const Variable*> children = bn._children.find(pv1)->second;
		os << *pv1 << ", ";
		os << "parents:{";
		for (auto pv2 : parents) {
			os << " " << pv2->id();
		}
		os << " }, ";
		os << "children:{";
		for (auto pv2 : children) {
			os << " " << pv2->id();
		}
		os << " }" << endl;
		bn.markov_independence(pv1);
	}
	os << endl;
	os << ">> Factors" << endl;
	for (auto pf : bn._factors) {
		os << *pf << endl;
	}
	return os;
}

}