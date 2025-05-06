def convert_to_metis(input_file, output_file):
    from collections import defaultdict

    adjacency = defaultdict(set)

    # Step 1: Read and build undirected adjacency list from the raw data
    with open(input_file, 'r') as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) < 2:
                continue  # skip malformed lines
            u, v = int(parts[0]), int(parts[1])
            if u != v:  # skip self-loops
                adjacency[u].add(v)
                adjacency[v].add(u)  # ensure undirected

    # Step 2: Relabel nodes to contiguous 1-based IDs
    all_nodes = sorted(adjacency.keys())
    node_to_id = {node: idx + 1 for idx, node in enumerate(all_nodes)}
    id_to_node = {v: k for k, v in node_to_id.items()}
    num_nodes = len(node_to_id)

    # Step 3: Count edges (each undirected edge once)
    total_edges = sum(len(adjacency[node]) for node in adjacency) // 2

    # Step 4: Write in METIS format
    with open(output_file, 'w') as f:
        f.write(f"{num_nodes} {total_edges}\n")
        for i in range(1, num_nodes + 1):
            node = id_to_node[i]
            neighbors = adjacency[node]
            neighbor_ids = sorted(node_to_id[n] for n in neighbors if n != node)
            f.write(" ".join(map(str, neighbor_ids)) + "\n")

    print(f"Converted and saved to {output_file}")

def remove_self_loops_and_convert(input_file, output_file):
    # Initialize an empty adjacency list
    adjacency = {}

    # Read the input graph file
    with open(input_file, 'r') as infile:
        for line in infile:
            # Skip comments and empty lines
            if line.startswith("#") or not line.strip():
                continue
            
            # Split the line into node connections
            nodes = list(map(int, line.strip().split()))
            
            # Ensure the adjacency list contains nodes
            u = nodes[0]
            if u not in adjacency:
                adjacency[u] = set()

            # Add connections (excluding self-loops)
            for v in nodes[1:]:
                if u != v:  # Remove self-loop
                    adjacency[u].add(v)
                    if v not in adjacency:
                        adjacency[v] = set()
                    adjacency[v].add(u)  # Add undirected edge

    # Write the output in METIS format
    with open(output_file, 'w') as outfile:
        # First line: number of nodes and edges
        num_nodes = len(adjacency)
        num_edges = sum(len(neighbors) for neighbors in adjacency.values()) // 2  # Since undirected
        outfile.write(f"{num_nodes} {num_edges}\n")
        
        # Adjacency list
        for node in range(1, num_nodes + 1):
            if node in adjacency:
                neighbors = sorted(adjacency[node])
                outfile.write(f"{' '.join(map(str, neighbors))}\n")

    print(f"Processed graph saved to {output_file}")




# Example usage
convert_to_metis("p2p-Gnutella08.txt", "p2p_dynamic_graph.txt")

#remove_self_loops_and_convert("graph1_metis.txt", "graph_metis.txt")
