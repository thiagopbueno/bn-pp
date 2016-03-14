#include "io.hh"
using namespace bn;

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <regex>
using namespace std;


void
usage(const char *progname);

void
read_options(unordered_map<string,bool> &options,  int argc, char *argv[]);

void
prompt(const BN &model, bool verbose);

void
parse_query(const BN &model, smatch result, string &target, string &evidence, unordered_set<const Variable*> &target_vars, unordered_set<const Variable*> &evidence_vars);

void
execute_query(const BN &model, smatch result, bool verbose);

int
main(int argc, char *argv[])
{
	char *progname = argv[0];
	if (argc < 2) {
		usage(progname);
		exit(1);
	}

	unordered_map<string,bool> options;
	read_options(options, argc, argv);
	if (options["help"]) {
		usage(progname);
		return 0;
	}

	char *model_filename = argv[1];
	unsigned order;
	BN *model;
	read_uai_model(model_filename, order, &model);
	if (options["verbose"]) {
		cout << ">> Model:" << endl;
		cout << *model << endl;
	}

	prompt(*model, options["verbose"]);

	delete model;

	return 0;
}

void
usage(const char *progname)
{
	cout << "usage: " << progname << " /path/to/model.uai [OPTIONS]" << endl << endl;
	cout << "OPTIONS:" << endl;
	cout << "-h\tdisplay help information" << endl;
	cout << "-v\tverbose" << endl;
}

void
read_options(unordered_map<string,bool> &options, int argc, char *argv[])
{
	// default options
	options["verbose"] = false;
	options["help"] = false;

	for (int i = 2; i < argc; ++i) {
		string option(argv[i]);
		if (option == "-h") {
			options["help"] = true;
		}
		else if (option == "-v") {
			options["verbose"] = true;
		}
	}
}

void
prompt(const BN &model, bool verbose)
{
	regex quit_regex("quit");
	regex query_regex("query ([0-9]+(\\s*,\\s*[0-9]+)*)\\s*(\\|\\s*([0-9]+(\\s*,\\s*[0-9]+)*))?");

	cout << ">> Query prompt:" << endl;
	while (cin) {
		cout << "? ";
		string line;
		getline(cin, line);

		smatch str_match_result;
		if (regex_match(line, str_match_result, query_regex)) {
			execute_query(model, str_match_result, verbose);
		}
		else if (regex_match(line, quit_regex)) {
			break;
		}
		else {
			cout << "Error: not a valid query." << endl;
		}
	}
}

void
parse_query(const BN &model,
	smatch result, string &target, string &evidence,
	unordered_set<const Variable*> &target_vars, unordered_set<const Variable*> &evidence_vars)
{
	regex whitespace_regex("\\s");

	target = result[1];
	target = regex_replace(target, whitespace_regex, "");
	evidence = result[4];
	evidence = regex_replace(evidence, whitespace_regex, "");

	string var = "";
	for (char& c: target) {
		if (c != ',') var += c;
		else {
			target_vars.insert(model.variables()[stoi(var)]);
			var = "";
		}
	}
	target_vars.insert(model.variables()[stoi(var)]);

	if (evidence != "") {
		var = "";
		for (char& c: evidence) {
			if (c != ',') var += c;
			else {
				evidence_vars.insert(model.variables()[stoi(var)]);
				var = "";
			}
		}
		evidence_vars.insert(model.variables()[stoi(var)]);
	}
}

void
execute_query(const BN &model, smatch result, bool verbose)
{
	string target;
	string evidence;
	unordered_set<const Variable*> target_vars;
	unordered_set<const Variable*> evidence_vars;
	parse_query(model, result, target, evidence, target_vars, evidence_vars);

	double uptime;
	Factor q = model.query(target_vars, evidence_vars, uptime, verbose);
	if (evidence != "") {
		cout << "P(" + target + "|" + evidence + ") =" << endl;
	}
	else {
		cout << "P(" + target + ") =" << endl;
	}
	cout << q;
	cout << ">> Executed in " << uptime << "ms." << endl << endl;
}
