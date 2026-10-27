#include <algorithm>
#include <climits>
#include <cstddef>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "recursive_graph_collections.h"

#ifndef LEVEL_MAX
#define LEVEL_MAX 4
#endif

namespace {
static_assert(LEVEL_MAX > 0, "LEVEL_MAX must be a positive number");

using node_t = int;
using level_t = std::size_t;
using edge_t = std::pair<node_t, node_t>; // {min, max}
using node_set_t = std::set<node_t>;
using adjacency_t = std::map<node_t, node_set_t>; // neighbours saved in both directions
using edge_levels_t = std::map<edge_t, level_t>;  // level of each edge, one entry per edge
using graph_t = std::tuple<node_set_t, adjacency_t, edge_levels_t>;
using graph_collection_t = std::map<std::string, graph_t, std::less<>>;
using collections_t = std::unordered_map<long, graph_collection_t>;

// indices for use with std::get on graph_t tuple
constexpr std::size_t NODES{0uz}, ADJACENCY{1uz}, EDGE_LEVELS{2uz};
constexpr level_t BASE_LEVEL{0uz};

collections_t &collections() {
    static collections_t all_collections;
    return all_collections;
}

bool is_valid_graph_name(char const *name) {
    if (name == nullptr) {
        return false;
    }
    std::string_view name_view{name};
    if (name_view.empty()) {
        return false;
    }
    return std::all_of(name_view.begin(), name_view.end(), [](char c) {
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
               ('0' <= c && c <= '9') || c == '_';
    });
}

// helper function for finding collection,
// returns nullptr on failure
graph_collection_t *find_collection(long id) {
    auto &all_collections = collections();
    auto iter = all_collections.find(id);
    if (iter == all_collections.end()) {
        return nullptr;
    }
    return &iter->second;
}

// helper functions for finding graph,
// returns nullptr on failure
graph_t *find_graph(graph_collection_t &collection, char const *name) {
    if (name == nullptr) {
        return nullptr;
    }
    auto iter = collection.find(name);
    if (iter == collection.end()) {
        return nullptr;
    }
    return &iter->second;
}

graph_t *find_graph(long id, char const *name) {
    auto collection = find_collection(id);
    if (collection == nullptr) {
        return nullptr;
    }
    return find_graph(*collection, name);
}

edge_t make_edge(node_t x, node_t y) {
    return {std::min(x, y), std::max(x, y)};
}

// inserts edge with given level, assumes x != y, and both nodes exist
// returns false if edge exists
bool insert_edge(graph_t &graph, node_t x, node_t y, level_t level) {
    auto &[nodes, adjacency, edge_levels] = graph;
    auto [iter, inserted] = edge_levels.emplace(make_edge(x, y), level);
    if (!inserted) {
        return false;
    }
    adjacency[x].emplace(y);
    adjacency[y].emplace(x);
    return true;
}

// erases y from x's neighbours
// if x's neighbours is empty after erasing, removes x's entry
void erase_neighbour(adjacency_t &adj, node_t x, node_t y) {
    auto iter = adj.find(x);
    iter->second.erase(y);
    if (iter->second.empty()) {
        adj.erase(iter);
    }
}

// deletes edge {x,y}
// returns false if edge did not exist
bool erase_edge(graph_t &graph, node_t x, node_t y) {
    auto &[nodes, adjacency, edge_levels] = graph;
    if (edge_levels.erase(make_edge(x, y)) == 0) {
        return false;
    }
    erase_neighbour(adjacency, x, y);
    erase_neighbour(adjacency, y, x);
    return true;
}

// returns biggest edge_level or BASE_LEVEL if no edges
level_t graph_level(const graph_t &graph) {
    const auto &edge_levels = std::get<EDGE_LEVELS>(graph);
    if (edge_levels.empty()) {
        return BASE_LEVEL;
    }
    return std::ranges::max(edge_levels | std::views::values);
}

// inserts smallest number >= candidate into the set that is not present in it
node_t take_free_node(node_set_t &nodes, node_t candidate) {
    auto iter = nodes.lower_bound(candidate);
    while (iter != nodes.end() && *iter == candidate) {
        ++iter;
        ++candidate;
    }
    nodes.emplace_hint(iter, candidate);
    return candidate;
}
} // namespace

// Creates a new empty graph collection and assigns it an id. Returns the
// id of the newly created collection or -1 if no more id's are available.
long cxx::rgc_new_collection() {
    static long last_collection_id{-1};

    if (last_collection_id == LONG_MAX) {
        return -1;
    }
    ++last_collection_id;
    collections().try_emplace(last_collection_id);
    return last_collection_id;
}

// Deletes the graph collection with a specified id (if one exists).
void cxx::rgc_delete_collection(long id) {
    collections().erase(id);
}

bool cxx::rgc_new_graph(long id, char const *name, int n) {
    auto collection_iter = collections().find(id);
    if (!is_valid_graph_name(name) || n < 0 || collection_iter == collections().end()) {
        return false;
    }
    auto [graph_iter, inserted] = collection_iter->second.try_emplace(name);
    if (!inserted) {
        return false;
    }
    auto &nodes = std::get<NODES>(graph_iter->second);

    for (node_t i = 0; i < n; i++) {
        // Use emplace_hint() instead of emplace() for faster sequential insertions.
        nodes.emplace_hint(nodes.end(), i);
    }

    return true;
}

// Deletes graph 'name' from the collection with specified id. If no such graph
// or no such collection exist, then the function does nothing.
void cxx::rgc_delete_graph(long id, char const *name) {
    graph_collection_t *collection = find_collection(id);
    if (collection != nullptr && name) {
        collection->erase(name);
    }
}

// Returns a pointer to the name of the first graph in a collection with
// the specified id. Returns nullptr if the collection doesn't exist or is empty.
char const *cxx::rgc_first_graph(long id) {
    graph_collection_t *collection = find_collection(id);
    if (collection == nullptr || collection->empty()) {
        return nullptr;
    }
    return collection->begin()->first.c_str();
}

// Returns a pointer to the name of the first graph after graph 'name' in
// a collection with the specified id. Returns nullptr if no such graph
// or collection exists.
char const *cxx::rgc_next_graph(long id, char const *name) {
    graph_collection_t *collection = find_collection(id);
    if (collection == nullptr || find_graph(*collection, name) == nullptr) {
        return nullptr;
    }

    auto next = collection->upper_bound(name);
    return (next == collection->end()) ? nullptr : next->first.c_str();
}

// Gives the number of graphs in the collection with specified id. Returns -1
// if no such collection exists.
ssize_t cxx::rgc_size(long id) {
    graph_collection_t *collection = find_collection(id);
    return (collection == nullptr) ? -1 : static_cast<ssize_t>(collection->size());
}

// Creates a new node with the smallest free number in the specified graph.
// Returns the number of the created node or -1 if the graph does not exist.
int cxx::rgc_graph_new_node(long id, char const *name) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    node_set_t &nodes = std::get<NODES>(*graph);

    if (nodes.size() >= INT_MAX) {
        return -1;
    }

    return take_free_node(nodes, 0);
}

// Deletes node x together with all its incident edges. Returns true iff
// the graph exists and has node x.
bool cxx::rgc_graph_delete_node(long id, char const *name, int x) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return false;
    }

    auto &[nodes, adjacency, edge_levels] = *graph;
    if (nodes.erase(x) == 0) {
        return false;
    }

    auto neighbours = adjacency.find(x);
    if (neighbours != adjacency.end()) {
        for (node_t y : neighbours->second) {
            erase_neighbour(adjacency, y, x);
            edge_levels.erase(make_edge(x, y));
        }
        adjacency.erase(neighbours);
    }

    return true;
}

// Gives the number of the first node in a specified graph. Returns -1 if
// the graph is empty or does not exist.
int cxx::rgc_first_node(long id, char const *name) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    const node_set_t &nodes = std::get<NODES>(*graph);
    return (nodes.empty()) ? -1 : *nodes.begin();
}

// Gives the first node with a number bigger than 'x' in the specified graph.
// Returns -1 if either the graph or the node don't exist.
int cxx::rgc_next_node(long id, char const *name, int x) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    const node_set_t &nodes = std::get<NODES>(*graph);
    auto next = nodes.upper_bound(x);

    return (next == nodes.end()) ? -1 : *next;
}

bool cxx::rgc_graph_add_edge(long id, char const *name, int x, int y) {
    if (x == y) {
        return false;
    }

    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return false;
    }

    node_set_t &nodes = std::get<NODES>(*graph);
    if (!nodes.contains(x) || !nodes.contains(y)) {
        return false;
    }

    return insert_edge(*graph, x, y, BASE_LEVEL);
}

bool cxx::rgc_graph_delete_edge(long id, char const *name, int x, int y) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return false;
    }

    return erase_edge(*graph, x, y);
}

// Returns the smallest neighbour of node x in the specified graph. Returns -1
// if the graph does not exist or x has no neighbours.
int cxx::rgc_first_edge(long id, char const *name, int x) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    const adjacency_t &adjacency = std::get<ADJACENCY>(*graph);
    auto neighbours = adjacency.find(x);
    return (neighbours == adjacency.end()) ? -1 : *neighbours->second.begin();
}

// Returns the smallest neighbour of node x greater than y in the specified
// graph. Returns -1 if the graph does not exist or there is no such neighbour.
int cxx::rgc_next_edge(long id, char const *name, int x, int y) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    const adjacency_t &adjacency = std::get<ADJACENCY>(*graph);
    auto neighbours = adjacency.find(x);
    if (neighbours == adjacency.end()) {
        return -1;
    }

    auto next = neighbours->second.upper_bound(y);
    return (next == neighbours->second.end()) ? -1 : *next;
}

// Returns the level of edge {x, y} in the specified graph. Returns -1 if
// the graph or the edge does not exist.
ssize_t cxx::rgc_edge_level(long id, char const *name, int x, int y) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    const edge_levels_t &edge_levels = std::get<EDGE_LEVELS>(*graph);
    auto edge = edge_levels.find(make_edge(x, y));
    return (edge == edge_levels.end()) ? -1 : static_cast<ssize_t>(edge->second);
}

// Returns the level of the specified graph (the maximal level of its edges,
// or 0 if it has no edges). Returns -1 if the graph does not exist.
ssize_t cxx::rgc_graph_level(long id, char const *name) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    return static_cast<ssize_t>(graph_level(*graph));
}

ssize_t cxx::rgc_max_level() {
    return LEVEL_MAX - 1;
}

ssize_t cxx::rgc_count_nodes(long id, char const *name) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }
    return static_cast<ssize_t>(std::get<NODES>(*graph).size());
}

// Returns the number of edges in the specified graph. Returns -1 if
// the graph does not exist.
ssize_t cxx::rgc_count_edges(long id, char const *name) {
    graph_t *graph = find_graph(id, name);
    if (graph == nullptr) {
        return -1;
    }

    return static_cast<ssize_t>(std::get<EDGE_LEVELS>(*graph).size());
}
