#ifndef RECURSIVE_GRAPH_COLLECTIONS_H_
#define RECURSIVE_GRAPH_COLLECTIONS_H_

#ifdef __cplusplus
namespace cxx {
	extern "C" {
#endif
	long rgc_new_collection();

	void rgc_delete_collection(long id);

	bool rgc_new_graph(long id, char const *name, int n);

	void rgc_delete_graph(long id, char const *name);

	// do przetestowania
	int rgc_graph_new_node(long id, char const *name);

	// do przetestowania
	bool rgc_graph_delete_node(long id, char const *name, int x);

	// do przetestowania
	bool rgc_graph_add_edge(long id, char const *name, int x, int y);

	// do przetestowania
	bool rgc_graph_delete_edge(long id, char const *name, int x, int y);

	char const *rgc_first_graph(long id);

	char const *rgc_next_graph(long id, char const *name);

	// do przetestowania
	int rgc_first_node(long id, char const *name);
	
	// do przetestowania
	int rgc_next_node(long id, char const *name, int x);

	// do przetestowania
	int rgc_first_edge(long id, char const *name, int x);

	// do przetestowania
	int rgc_next_edge(long id, char const *name, int x, int y);

	/* START : OPERATIONS ON GRAPHS INSIDE COLLECTIONS

	// Big scary graph for edge injection.
	bool rgc_replace_edge(
		long id,
		char const *result_name,
		char const *recipient_name,
		int w1,
		int w2,
		char const *donor_name,
		int v_1,
		int v_2
	);

	END : OPERATIONS ON GRAPHS INSIDE COLLECTIONS */

	// do przetestowania
	ssize_t rgc_edge_level(long id, char const *name, int x, int y);

	// do przetestowania
	ssize_t rgc_graph_level(long id, char const *name);

	// do przetestowania
	ssize_t rgc_max_level();

	// do przetestowania
	ssize_t rgc_count_nodes(long id, char const *name);

	// do przetestowania
	ssize_t rgc_count_edges(long id, char const *name);

	ssize_t rgc_size(long id);

#ifdef __cplusplus
	} // extern "C"
} // namespace cxx
#endif

#endif
