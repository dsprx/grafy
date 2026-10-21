#include <utility>
#include <set>
#include <map>
#include <cstring>
#include <cstddef>
#include <string>
#include <climits>
#include <cctype>
#include <algorithm>
#include <sys/types.h>

#include "recursive_graph_collections.h"

#ifndef LEVEL_MAX
# define LEVEL_MAX 4
#endif

namespace {
	using node_t = int;
	using level_t = size_t;
	using edge_t = std::pair<node_t, node_t>;
	using edge_set_t = std::pair<std::map<node_t, std::set<node_t>>, std::map<edge_t, level_t>>;
	using graph_t = std::pair<std::set<node_t>, edge_set_t>;
	using graph_collection_t = std::map<std::string, graph_t, std::less<>>;

	std::map<long, graph_collection_t> collections;

	bool is_valid_graph_name(const std::string& name) {
		return std::all_of(name.begin(), name.end(), [](char c) {
			return std::isalnum(c) || c == '_';
		});
	}

	bool collection_exists(long id) {
		return collections.contains(id);
	}

	bool graph_exists(long id, const char *name) {
		return name && collection_exists(id) && collections[id].contains(name);
	}

	bool edge_exists(const edge_set_t& edge_set, const edge_t& edge) {
		return edge_set.first.contains(edge.first) &&
					 edge_set.first[edge.first].contains(edge.second);
	}
}

// Creates a new empty graph collection and assigns it an id. Returns the
// id of the newly created collection or -1 if no more id's are available.
long cxx::rgc_new_collection() {
	static long next_collection_id = 0;

	if (LONG_MAX - next_collection_id == 0) {
		return -1;
	}

	collections.try_emplace(next_collection_id);
	return next_collection_id++;
}

// Deletes the graph collection with a specified id (if one exists).
void cxx::rgc_delete_collection(long id) {
	collections.erase(id);
}

bool cxx::rgc_new_graph(long id, char const *name, int n) {
	if (!graph_exists(id, name)) {
		return false;
	}

	graph_t new_graph;
	auto hint = new_graph.first.begin();

	for (node_t i = 0; i < n; i++) {
		// Use emplace_hint() instead of emplace() for faster sequential insertions.
		hint = new_graph.first.emplace_hint(hint, i);
	}

	collections[id].try_emplace(name, new_graph);
	return true;
}

// Deletes graph 'name' from the collection with specified id. If no such graph
// or no such collection exist, then the function does nothing.
void cxx::rgc_delete_graph(long id, char const *name) {
	if (name && collection_exists(id)) {
		collections[id].erase(name);
	}
}

// Returns a pointer to the name of the first graph in a collection with
// the specified id. Returns nullptr if the collection doesn't exist or is empty.
char const *cxx::rgc_first_graph(long id) {
	if (collection_exists(id) && !collections[id].empty()) {
		return collections[id].begin()->first.c_str();
	}

	return nullptr;
}

// Returns a pointer to the name of the first graph after graph 'name' in
// a collection with the specified id. Returns nullptr if no such graph
// or collection exists.
char const *cxx::rgc_next_graph(long id, char const *name) {
	if (!graph_exists(id, name)) {
		return nullptr;
	}

	auto next = collections[id].upper_bound(name);
	return (next == collections[id].end()) ? nullptr : next->first.c_str();
}

// Gives the number of graphs in the collection with specified id. Returns -1
// if no such collection exists.
ssize_t cxx::rgc_size(long id) {
	return (collection_exists(id)) ? collections[id].size() : -1;
}

// Creates a new node with the smallest free number in the specified graph.
// Returns the number of the created node or -1 if the graph does not exist.
int cxx::rgc_graph_new_node(long id, char const *name) {
	if (!graph_exists(id, name)) {
		return -1;
	}

	std::set<node_t>& nodes = collections[id][name].first;

	if (nodes.size() >= INT_MAX) {
		return -1;
	}

	node_t expected = 0;
	for (node_t node : nodes) {
		if (node != expected) {
			break;
		}

		expected++;
	}

	nodes.emplace(expected);
	return expected;
}

// Deletes a node in the specified graph. Returns true iff the graph
// exists and a deletion was performed.
bool cxx::rgc_graph_delete_node(long id, char const *name, int x) {
	return graph_exists(id, name) && (collections[id][name].first.erase(x) > 0);
}

// Gives the number of the first node in a specified graph. Returns -1 if
// the graph is empty or does not exist.
int cxx::rgc_first_node(long id, char const *name) {
	if (!graph_exists(id, name)) {
		return -1;
	}

	const std::set<node_t>& nodes = collections[id][name].first;
	return (nodes.empty()) ? -1 : *nodes.begin();
}

// Gives the first node with a number bigger than 'x' in the specified graph.
// Returns -1 if either the graph or the node don't exist.
int cxx::rgc_next_node(long id, char const *name, int x) {
	if (!graph_exists(id, name)) {
		return -1;
	}

	std::set<node_t>& nodes = collections[id][name].first;
	auto next = nodes.upper_bound(x);

	return (next == nodes.end()) ? -1 : *next;
}

bool cxx::rgc_graph_add_edge(long id, char const *name, int x, int y) {
	if (!graph_exists(id, name)) {
		return false;
	}

	edge_set_t& edge_set = collections[id][name].second;
	edge_t new_edge{std::min(x, y), std::max(x, y)};

	if (edge_exists(edge_set, new_edge)) {
		return false;
	}

	edge_set.first[new_edge.first].emplace(new_edge.second);
	edge_set.second.emplace(new_edge, 0);
	return true;
}

bool cxx::rgc_graph_delete_edge(long id, char const *name, int x, int y) {
	if (!graph_exists(id, name)) {
		return false;
	}

	edge_set_t& edge_set = collections[id][name].second;
	edge_t edge{std::min(x, y), std::max(x, y)};

	if (!edge_exists(edge_set, edge)) {
		return false;
	}

	edge_set.first[edge.first].erase(edge.second);
	if (edge_set.first[edge.first].empty()) {
		edge_set.first.erase(edge.first);
	}

	edge_set.second.erase(edge);
	return true;
}

int cxx::rgc_first_edge(long id, char const *name, int x) {
	if (!graph_exists(id, name)) {
		return -1;
	}

	auto& adj_map = collections[id][name].second.first;

	if (!adj_map.contains(x) || adj_map[x].empty()) {
		return -1;
	}

	return *adj_map[x].begin();
}

int cxx::rgc_next_edge(long id, char const *name, int x, int y) {
	if (!graph_exists(id, name)) {
		return -1;
	}

	auto& adj_map = collections[id][name].second.first;
	if (!adj_map.contains(x)) {
		return -1;
	}

	auto next = adj_map[x].upper_bound(y);
	return (next == adj_map[x].end()) ? -1 : *next;
}

ssize_t cxx::rgc_edge_level(long id, char const *name, int x, int y) {
	if (!graph_exists(id, name)) {
		return -1;
	}

	const edge_set_t& edge_set = collections[id][name].second;
	const edge_t& edge = {std::min(x, y), std::max(x, y)};

	if (!edge_exists(edge_set, edge)) {
		return -1;
	}

	return edge_set.second[edge];
}

ssize_t cxx::rgc_graph_level(long id, char const *name) {
	return 0; // stub
}

ssize_t cxx::rgc_max_level() {
	return LEVEL_MAX - 1;
}

ssize_t cxx::rgc_count_nodes(long id, char const *name) {
	return graph_exists(id, name) ? collections[id][name].first.size() : -1;
}

ssize_t cxx::rgc_count_edges(long id, char const *name) {
	return 0; // stub
}
